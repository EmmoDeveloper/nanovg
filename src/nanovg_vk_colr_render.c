// nanovg_vk_colr_render.c - COLR Vector Emoji Rendering Implementation

#include "nanovg_vk_colr_render.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Renderer Management
// ============================================================================

VKNVGcolrRenderer* vknvg__createColrRenderer(VKNVGcolorAtlas* colorAtlas,
                                              const uint8_t* fontData,
                                              uint32_t fontDataSize)
{
	if (!colorAtlas || !fontData) return NULL;

	VKNVGcolrRenderer* renderer = (VKNVGcolrRenderer*)calloc(1, sizeof(VKNVGcolrRenderer));
	if (!renderer) return NULL;

	renderer->colorAtlas = colorAtlas;

	// Initialize FreeType
	if (FT_Init_FreeType(&renderer->ftLibrary) != 0) {
		free(renderer);
		return NULL;
	}

	// Load font face
	if (FT_New_Memory_Face(renderer->ftLibrary, fontData, fontDataSize, 0, &renderer->ftFace) != 0) {
		FT_Done_FreeType(renderer->ftLibrary);
		free(renderer);
		return NULL;
	}

	// Parse emoji tables
	VKNVGemojiFormat format = vknvg__detectEmojiFormat(fontData, fontDataSize);

	if (format == VKNVG_EMOJI_COLR) {
		renderer->colrTable = vknvg__parseColrTable(fontData, fontDataSize);
		renderer->cpalTable = vknvg__parseCpalTable(fontData, fontDataSize);
	}

	renderer->currentPalette = 0;

	return renderer;
}

void vknvg__destroyColrRenderer(VKNVGcolrRenderer* renderer)
{
	if (!renderer) return;

	if (renderer->ftFace) {
		FT_Done_Face(renderer->ftFace);
	}
	if (renderer->ftLibrary) {
		FT_Done_FreeType(renderer->ftLibrary);
	}

	if (renderer->colrTable) {
		vknvg__freeColrTable(renderer->colrTable);
	}
	if (renderer->cpalTable) {
		vknvg__freeCpalTable(renderer->cpalTable);
	}

	free(renderer);
}

void vknvg__setColrPalette(VKNVGcolrRenderer* renderer, uint16_t paletteIndex)
{
	if (!renderer) return;
	renderer->currentPalette = paletteIndex;
}

// ============================================================================
// Layer Rasterization
// ============================================================================

uint8_t* vknvg__rasterizeGlyphLayer(VKNVGcolrRenderer* renderer,
                                     uint32_t glyphID,
                                     float size,
                                     uint32_t* outWidth,
                                     uint32_t* outHeight,
                                     int32_t* outBearingX,
                                     int32_t* outBearingY,
                                     uint32_t* outAdvance)
{
	if (!renderer || !renderer->ftFace) return NULL;

	// Set font size (size is in pixels, convert to 26.6 fixed point)
	if (FT_Set_Char_Size(renderer->ftFace, 0, (FT_F26Dot6)(size * 64), 96, 96) != 0) {
		return NULL;
	}

	// Load glyph
	if (FT_Load_Glyph(renderer->ftFace, glyphID, FT_LOAD_DEFAULT) != 0) {
		return NULL;
	}

	// Render to grayscale bitmap
	if (FT_Render_Glyph(renderer->ftFace->glyph, FT_RENDER_MODE_NORMAL) != 0) {
		return NULL;
	}

	FT_Bitmap* bitmap = &renderer->ftFace->glyph->bitmap;

	// Check for empty bitmap
	if (bitmap->width == 0 || bitmap->rows == 0) {
		return NULL;
	}

	// Copy bitmap data
	uint32_t width = bitmap->width;
	uint32_t height = bitmap->rows;
	uint8_t* grayscale = (uint8_t*)malloc(width * height);
	if (!grayscale) return NULL;

	// FreeType bitmaps may have pitch != width
	for (uint32_t y = 0; y < height; y++) {
		memcpy(grayscale + y * width,
		       bitmap->buffer + y * bitmap->pitch,
		       width);
	}

	// Output metrics
	if (outWidth) *outWidth = width;
	if (outHeight) *outHeight = height;
	if (outBearingX) *outBearingX = renderer->ftFace->glyph->bitmap_left;
	if (outBearingY) *outBearingY = renderer->ftFace->glyph->bitmap_top;
	if (outAdvance) *outAdvance = renderer->ftFace->glyph->advance.x >> 6;

	renderer->layerRenders++;

	return grayscale;
}

// ============================================================================
// Color Application
// ============================================================================

void vknvg__applyPaletteColor(const uint8_t* grayscale,
                              uint32_t width, uint32_t height,
                              VKNVGcpalColor color,
                              uint8_t* outRgba)
{
	if (!grayscale || !outRgba) return;

	for (uint32_t i = 0; i < width * height; i++) {
		uint8_t alpha = grayscale[i];

		// CPAL color is BGRA, convert to RGBA and apply grayscale as alpha
		outRgba[i * 4 + 0] = color.red;
		outRgba[i * 4 + 1] = color.green;
		outRgba[i * 4 + 2] = color.blue;
		outRgba[i * 4 + 3] = (color.alpha * alpha) / 255;
	}
}

void vknvg__applyForegroundColor(const uint8_t* grayscale,
                                 uint32_t width, uint32_t height,
                                 uint32_t foregroundColor,
                                 uint8_t* outRgba)
{
	if (!grayscale || !outRgba) return;

	// foregroundColor is RGBA32
	uint8_t r = (foregroundColor >> 24) & 0xFF;
	uint8_t g = (foregroundColor >> 16) & 0xFF;
	uint8_t b = (foregroundColor >> 8) & 0xFF;
	uint8_t a = foregroundColor & 0xFF;

	for (uint32_t i = 0; i < width * height; i++) {
		uint8_t alpha = grayscale[i];

		outRgba[i * 4 + 0] = r;
		outRgba[i * 4 + 1] = g;
		outRgba[i * 4 + 2] = b;
		outRgba[i * 4 + 3] = (a * alpha) / 255;
	}
}

// ============================================================================
// Layer Compositing
// ============================================================================

void vknvg__compositeLayer(const uint8_t* srcRgba,
                           uint8_t* dstRgba,
                           uint32_t width, uint32_t height)
{
	if (!srcRgba || !dstRgba) return;

	for (uint32_t i = 0; i < width * height; i++) {
		uint32_t offset = i * 4;

		uint8_t srcR = srcRgba[offset + 0];
		uint8_t srcG = srcRgba[offset + 1];
		uint8_t srcB = srcRgba[offset + 2];
		uint8_t srcA = srcRgba[offset + 3];

		uint8_t dstR = dstRgba[offset + 0];
		uint8_t dstG = dstRgba[offset + 1];
		uint8_t dstB = dstRgba[offset + 2];
		uint8_t dstA = dstRgba[offset + 3];

		// Alpha compositing: C = Ca + Cb * (1 - Aa)
		if (srcA == 255) {
			dstRgba[offset + 0] = srcR;
			dstRgba[offset + 1] = srcG;
			dstRgba[offset + 2] = srcB;
			dstRgba[offset + 3] = 255;
		} else if (srcA > 0) {
			uint32_t invSrcA = 255 - srcA;

			dstRgba[offset + 0] = (srcR * srcA + dstR * invSrcA) / 255;
			dstRgba[offset + 1] = (srcG * srcA + dstG * invSrcA) / 255;
			dstRgba[offset + 2] = (srcB * srcA + dstB * invSrcA) / 255;
			dstRgba[offset + 3] = srcA + (dstA * invSrcA) / 255;
		}
	}
}

void vknvg__compositeLayerWithOffset(const uint8_t* srcRgba,
                                     uint32_t srcWidth, uint32_t srcHeight,
                                     int32_t srcBearingX, int32_t srcBearingY,
                                     uint8_t* dstRgba,
                                     uint32_t dstWidth, uint32_t dstHeight,
                                     int32_t dstBearingX, int32_t dstBearingY)
{
	if (!srcRgba || !dstRgba) return;

	// Calculate offset in destination
	int32_t offsetX = srcBearingX - dstBearingX;
	int32_t offsetY = dstBearingY - srcBearingY;

	for (uint32_t srcY = 0; srcY < srcHeight; srcY++) {
		int32_t dstY = (int32_t)srcY + offsetY;
		if (dstY < 0 || dstY >= (int32_t)dstHeight) continue;

		for (uint32_t srcX = 0; srcX < srcWidth; srcX++) {
			int32_t dstX = (int32_t)srcX + offsetX;
			if (dstX < 0 || dstX >= (int32_t)dstWidth) continue;

			uint32_t srcOffset = (srcY * srcWidth + srcX) * 4;
			uint32_t dstOffset = (dstY * dstWidth + dstX) * 4;

			uint8_t srcR = srcRgba[srcOffset + 0];
			uint8_t srcG = srcRgba[srcOffset + 1];
			uint8_t srcB = srcRgba[srcOffset + 2];
			uint8_t srcA = srcRgba[srcOffset + 3];

			if (srcA == 0) continue;

			uint8_t dstR = dstRgba[dstOffset + 0];
			uint8_t dstG = dstRgba[dstOffset + 1];
			uint8_t dstB = dstRgba[dstOffset + 2];
			uint8_t dstA = dstRgba[dstOffset + 3];

			if (srcA == 255) {
				dstRgba[dstOffset + 0] = srcR;
				dstRgba[dstOffset + 1] = srcG;
				dstRgba[dstOffset + 2] = srcB;
				dstRgba[dstOffset + 3] = 255;
			} else {
				uint32_t invSrcA = 255 - srcA;

				dstRgba[dstOffset + 0] = (srcR * srcA + dstR * invSrcA) / 255;
				dstRgba[dstOffset + 1] = (srcG * srcA + dstG * invSrcA) / 255;
				dstRgba[dstOffset + 2] = (srcB * srcA + dstB * invSrcA) / 255;
				dstRgba[dstOffset + 3] = srcA + (dstA * invSrcA) / 255;
			}
		}
	}
}

// ============================================================================
// COLR Rendering
// ============================================================================

void vknvg__calculateColrBounds(VKNVGcolrRenderer* renderer,
                                VKNVGcolrGlyph* colrGlyph,
                                float size,
                                uint32_t* outWidth,
                                uint32_t* outHeight,
                                int32_t* outBearingX,
                                int32_t* outBearingY)
{
	if (!renderer || !colrGlyph) return;

	int32_t minX = INT32_MAX;
	int32_t minY = INT32_MAX;
	int32_t maxX = INT32_MIN;
	int32_t maxY = INT32_MIN;

	// Calculate bounding box across all layers
	for (uint16_t i = 0; i < colrGlyph->numLayers; i++) {
		uint32_t layerGlyphID = colrGlyph->layers[i].glyphID;

		uint32_t width, height;
		int32_t bearingX, bearingY;
		uint32_t advance;

		uint8_t* grayscale = vknvg__rasterizeGlyphLayer(renderer, layerGlyphID, size,
		                                                 &width, &height,
		                                                 &bearingX, &bearingY,
		                                                 &advance);
		if (!grayscale) continue;

		int32_t layerMinX = bearingX;
		int32_t layerMaxX = bearingX + (int32_t)width;
		int32_t layerMinY = bearingY - (int32_t)height;
		int32_t layerMaxY = bearingY;

		if (layerMinX < minX) minX = layerMinX;
		if (layerMaxX > maxX) maxX = layerMaxX;
		if (layerMinY < minY) minY = layerMinY;
		if (layerMaxY > maxY) maxY = layerMaxY;

		free(grayscale);
	}

	if (minX == INT32_MAX) {
		// No layers rendered
		if (outWidth) *outWidth = 0;
		if (outHeight) *outHeight = 0;
		if (outBearingX) *outBearingX = 0;
		if (outBearingY) *outBearingY = 0;
		return;
	}

	if (outWidth) *outWidth = maxX - minX;
	if (outHeight) *outHeight = maxY - minY;
	if (outBearingX) *outBearingX = minX;
	if (outBearingY) *outBearingY = maxY;
}

int vknvg__renderColrGlyph(VKNVGcolrRenderer* renderer,
                           uint32_t glyphID,
                           float size,
                           VKNVGcolrRenderResult* outResult)
{
	if (!renderer || !renderer->colrTable || !renderer->cpalTable || !outResult) return 0;

	// Get COLR glyph
	VKNVGcolrGlyph* colrGlyph = vknvg__getColrGlyph(renderer->colrTable, glyphID);
	if (!colrGlyph || colrGlyph->numLayers == 0) return 0;

	// Calculate bounding box
	uint32_t width, height;
	int32_t bearingX, bearingY;
	vknvg__calculateColrBounds(renderer, colrGlyph, size, &width, &height, &bearingX, &bearingY);

	if (width == 0 || height == 0) return 0;

	// Allocate destination RGBA buffer
	uint8_t* dstRgba = (uint8_t*)calloc(width * height, 4);
	if (!dstRgba) return 0;

	// Render and composite each layer
	for (uint16_t i = 0; i < colrGlyph->numLayers; i++) {
		VKNVGcolrLayer* layer = &colrGlyph->layers[i];

		// Rasterize layer
		uint32_t layerWidth, layerHeight;
		int32_t layerBearingX, layerBearingY;
		uint32_t layerAdvance;

		uint8_t* grayscale = vknvg__rasterizeGlyphLayer(renderer, layer->glyphID, size,
		                                                 &layerWidth, &layerHeight,
		                                                 &layerBearingX, &layerBearingY,
		                                                 &layerAdvance);
		if (!grayscale) continue;

		// Allocate layer RGBA buffer
		uint8_t* layerRgba = (uint8_t*)malloc(layerWidth * layerHeight * 4);
		if (!layerRgba) {
			free(grayscale);
			continue;
		}

		// Apply palette color
		if (layer->paletteIndex == 0xFFFF) {
			// Foreground color (use default black for now)
			vknvg__applyForegroundColor(grayscale, layerWidth, layerHeight, 0x000000FF, layerRgba);
		} else {
			VKNVGcpalColor color = vknvg__getCpalColor(renderer->cpalTable,
			                                            renderer->currentPalette,
			                                            layer->paletteIndex);
			vknvg__applyPaletteColor(grayscale, layerWidth, layerHeight, color, layerRgba);
		}

		free(grayscale);

		// Composite layer onto destination
		vknvg__compositeLayerWithOffset(layerRgba, layerWidth, layerHeight,
		                                layerBearingX, layerBearingY,
		                                dstRgba, width, height,
		                                bearingX, bearingY);

		free(layerRgba);
		renderer->composites++;
	}

	// Get advance from base glyph
	FT_Set_Char_Size(renderer->ftFace, 0, (FT_F26Dot6)(size * 64), 96, 96);
	FT_Load_Glyph(renderer->ftFace, glyphID, FT_LOAD_DEFAULT);
	uint32_t advance = renderer->ftFace->glyph->advance.x >> 6;

	// Fill result
	outResult->rgbaData = dstRgba;
	outResult->width = width;
	outResult->height = height;
	outResult->bearingX = bearingX;
	outResult->bearingY = bearingY;
	outResult->advance = advance;

	return 1;
}

// ============================================================================
// Atlas Integration
// ============================================================================

VKNVGcolorGlyphCacheEntry* vknvg__loadColrEmoji(VKNVGcolrRenderer* renderer,
                                                 uint32_t fontID,
                                                 uint32_t glyphID,
                                                 float size)
{
	if (!renderer || !renderer->colorAtlas) return NULL;

	// Check cache first
	VKNVGcolorGlyphCacheEntry* cached = vknvg__lookupColorGlyph(renderer->colorAtlas,
	                                                             fontID, glyphID, (uint32_t)size);
	if (cached && cached->state == VKNVG_COLOR_GLYPH_UPLOADED) {
		vknvg__touchColorGlyph(renderer->colorAtlas, cached);
		return cached;
	}

	// Render COLR glyph
	VKNVGcolrRenderResult result = {0};
	if (!vknvg__renderColrGlyph(renderer, glyphID, size, &result)) {
		return NULL;
	}

	// Allocate atlas space
	uint16_t atlasX, atlasY;
	uint32_t atlasIndex;
	if (!vknvg__allocColorGlyph(renderer->colorAtlas, result.width, result.height,
	                            &atlasX, &atlasY, &atlasIndex)) {
		free(result.rgbaData);
		return NULL;
	}

	// Allocate cache entry
	VKNVGcolorGlyphCacheEntry* entry = vknvg__allocColorGlyphEntry(renderer->colorAtlas,
	                                                                fontID, glyphID, (uint32_t)size);
	if (!entry) {
		free(result.rgbaData);
		return NULL;
	}

	// Fill entry
	entry->atlasIndex = atlasIndex;
	entry->atlasX = atlasX;
	entry->atlasY = atlasY;
	entry->width = result.width;
	entry->height = result.height;
	entry->bearingX = result.bearingX;
	entry->bearingY = result.bearingY;
	entry->advance = result.advance;

	// Queue upload
	vknvg__queueColorUpload(renderer->colorAtlas, atlasIndex, atlasX, atlasY,
	                        result.width, result.height, result.rgbaData, entry);

	renderer->cacheUploads++;

	free(result.rgbaData);
	return entry;
}

// ============================================================================
// Utility Functions
// ============================================================================

void vknvg__getColrRendererStats(VKNVGcolrRenderer* renderer,
                                 uint32_t* outLayerRenders,
                                 uint32_t* outComposites,
                                 uint32_t* outCacheUploads)
{
	if (!renderer) return;

	if (outLayerRenders) *outLayerRenders = renderer->layerRenders;
	if (outComposites) *outComposites = renderer->composites;
	if (outCacheUploads) *outCacheUploads = renderer->cacheUploads;
}
