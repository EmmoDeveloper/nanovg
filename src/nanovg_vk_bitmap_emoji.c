// nanovg_vk_bitmap_emoji.c - Bitmap Emoji Pipeline Implementation

#include "nanovg_vk_bitmap_emoji.h"
#include <stdlib.h>
#include <string.h>
#include <png.h>

// ============================================================================
// Pipeline Management
// ============================================================================

VKNVGbitmapEmoji* vknvg__createBitmapEmoji(VKNVGcolorAtlas* colorAtlas,
                                            const uint8_t* fontData,
                                            uint32_t fontDataSize)
{
	if (!colorAtlas || !fontData) return NULL;

	VKNVGbitmapEmoji* bitmap = (VKNVGbitmapEmoji*)calloc(1, sizeof(VKNVGbitmapEmoji));
	if (!bitmap) return NULL;

	bitmap->colorAtlas = colorAtlas;

	// Parse emoji tables
	VKNVGemojiFormat format = vknvg__detectEmojiFormat(fontData, fontDataSize);

	if (format == VKNVG_EMOJI_SBIX) {
		bitmap->sbixTable = vknvg__parseSbixTable(fontData, fontDataSize);
	} else if (format == VKNVG_EMOJI_CBDT) {
		bitmap->cbdtTable = vknvg__parseCbdtTable(fontData, fontDataSize);
	}

	return bitmap;
}

void vknvg__destroyBitmapEmoji(VKNVGbitmapEmoji* bitmap)
{
	if (!bitmap) return;

	if (bitmap->sbixTable) {
		vknvg__freeSbixTable(bitmap->sbixTable);
	}
	if (bitmap->cbdtTable) {
		vknvg__freeCbdtTable(bitmap->cbdtTable);
	}

	free(bitmap);
}

// ============================================================================
// PNG Decoding
// ============================================================================

// PNG read callback for memory buffer
typedef struct {
	const uint8_t* data;
	uint32_t size;
	uint32_t offset;
} VKNVGpngReadState;

static void vknvg__pngReadMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
	VKNVGpngReadState* state = (VKNVGpngReadState*)png_get_io_ptr(png_ptr);
	if (!state || state->offset + length > state->size) {
		png_error(png_ptr, "Read error");
		return;
	}
	memcpy(data, state->data + state->offset, length);
	state->offset += length;
}

uint8_t* vknvg__decodePNG(const uint8_t* pngData,
                          uint32_t pngSize,
                          uint32_t* outWidth,
                          uint32_t* outHeight)
{
	if (!pngData || !outWidth || !outHeight || pngSize < 8) {
		return NULL;
	}

	// Verify PNG signature
	if (png_sig_cmp(pngData, 0, 8) != 0) {
		return NULL;
	}

	// Create PNG structs
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) return NULL;

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, NULL, NULL);
		return NULL;
	}

	// Error handling
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		return NULL;
	}

	// Set up memory reading
	VKNVGpngReadState readState = {pngData, pngSize, 8};
	png_set_read_fn(png, &readState, vknvg__pngReadMemory);
	png_set_sig_bytes(png, 8);

	// Read PNG info
	png_read_info(png, info);

	uint32_t width = png_get_image_width(png, info);
	uint32_t height = png_get_image_height(png, info);
	png_byte colorType = png_get_color_type(png, info);
	png_byte bitDepth = png_get_bit_depth(png, info);

	// Convert to RGBA8
	if (bitDepth == 16) {
		png_set_strip_16(png);
	}
	if (colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}
	if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
		png_set_expand_gray_1_2_4_to_8(png);
	}
	if (png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png);
	}
	if (colorType == PNG_COLOR_TYPE_RGB ||
	    colorType == PNG_COLOR_TYPE_GRAY ||
	    colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}
	if (colorType == PNG_COLOR_TYPE_GRAY ||
	    colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png);
	}

	png_read_update_info(png, info);

	// Allocate RGBA buffer
	uint8_t* rgbaData = (uint8_t*)malloc(width * height * 4);
	if (!rgbaData) {
		png_destroy_read_struct(&png, &info, NULL);
		return NULL;
	}

	// Read image rows
	png_bytep* rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (uint32_t y = 0; y < height; y++) {
		rowPointers[y] = rgbaData + y * width * 4;
	}

	png_read_image(png, rowPointers);
	png_read_end(png, NULL);

	free(rowPointers);
	png_destroy_read_struct(&png, &info, NULL);

	*outWidth = width;
	*outHeight = height;
	return rgbaData;
}

// ============================================================================
// SBIX Extraction
// ============================================================================

VKNVGsbixStrike* vknvg__selectBestSbixStrike(VKNVGsbixTable* sbixTable, float size)
{
	if (!sbixTable || sbixTable->numStrikes == 0) return NULL;

	// Find strike with closest ppem
	VKNVGsbixStrike* bestStrike = NULL;
	int bestDiff = INT32_MAX;

	for (uint16_t i = 0; i < sbixTable->numStrikes; i++) {
		VKNVGsbixStrike* strike = &sbixTable->strikes[i];
		int diff = abs((int)strike->ppem - (int)size);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestStrike = strike;
		}
	}

	return bestStrike;
}

int vknvg__extractSbixBitmap(VKNVGsbixTable* sbixTable,
                             uint32_t glyphID,
                             float size,
                             VKNVGbitmapResult* outResult)
{
	if (!sbixTable || !outResult) return 0;

	// Select best strike
	VKNVGsbixStrike* strike = vknvg__selectBestSbixStrike(sbixTable, size);
	if (!strike) return 0;

	// Get glyph data
	VKNVGsbixGlyphData* glyphData = vknvg__getSbixGlyphData(strike, glyphID);
	if (!glyphData) return 0;

	// Decode PNG (graphicType is FourCC 'png ' = 0x706E6720)
	uint32_t width, height;
	uint8_t* rgbaData = vknvg__decodePNG(glyphData->data, glyphData->dataLength,
	                                     &width, &height);

	if (!rgbaData) {
		vknvg__freeSbixGlyphData(glyphData);
		return 0;
	}

	// Fill result
	outResult->rgbaData = rgbaData;
	outResult->width = width;
	outResult->height = height;
	outResult->bearingX = glyphData->originOffsetX;
	outResult->bearingY = glyphData->originOffsetY;
	outResult->advance = (uint16_t)width;

	vknvg__freeSbixGlyphData(glyphData);
	return 1;
}

// ============================================================================
// CBDT Extraction
// ============================================================================

VKNVGcblcBitmapSize* vknvg__selectBestCbdtSize(VKNVGcbdtTable* cbdtTable, float size)
{
	if (!cbdtTable || !cbdtTable->cblc) return NULL;

	// Use the existing function
	return vknvg__selectCbdtSize(cbdtTable, size);
}

int vknvg__extractCbdtBitmap(VKNVGcbdtTable* cbdtTable,
                             uint32_t glyphID,
                             float size,
                             VKNVGbitmapResult* outResult)
{
	if (!cbdtTable || !outResult) return 0;

	// Select best size
	VKNVGcblcBitmapSize* bitmapSize = vknvg__selectBestCbdtSize(cbdtTable, size);
	if (!bitmapSize) return 0;

	// Get glyph data
	VKNVGcbdtGlyphData* glyphData = vknvg__getCbdtGlyphData(cbdtTable, bitmapSize, glyphID);
	if (!glyphData) return 0;

	uint8_t* rgbaData = NULL;
	uint32_t width = 0;
	uint32_t height = 0;
	int16_t bearingX = 0;
	int16_t bearingY = 0;
	uint16_t advance = 0;

	// Extract metrics based on format
	if (glyphData->format == VKNVG_CBDT_FORMAT_17) {
		// Small metrics
		width = glyphData->metrics.small.width;
		height = glyphData->metrics.small.height;
		bearingX = glyphData->metrics.small.bearingX;
		bearingY = glyphData->metrics.small.bearingY;
		advance = glyphData->metrics.small.advance;
	} else if (glyphData->format == VKNVG_CBDT_FORMAT_18) {
		// Big metrics
		width = glyphData->metrics.big.width;
		height = glyphData->metrics.big.height;
		bearingX = glyphData->metrics.big.horiBearingX;
		bearingY = glyphData->metrics.big.horiBearingY;
		advance = glyphData->metrics.big.horiAdvance;
	}

	// Decode PNG data
	if (glyphData->format == VKNVG_CBDT_FORMAT_17 || glyphData->format == VKNVG_CBDT_FORMAT_18) {
		// PNG format
		uint32_t decodedWidth, decodedHeight;
		rgbaData = vknvg__decodePNG(glyphData->data, glyphData->dataLength,
		                            &decodedWidth, &decodedHeight);
		if (rgbaData) {
			width = decodedWidth;
			height = decodedHeight;
		}
	}

	if (!rgbaData) {
		vknvg__freeCbdtGlyphData(glyphData);
		return 0;
	}

	// Fill result
	outResult->rgbaData = rgbaData;
	outResult->width = width;
	outResult->height = height;
	outResult->bearingX = bearingX;
	outResult->bearingY = bearingY;
	outResult->advance = advance;

	vknvg__freeCbdtGlyphData(glyphData);
	return 1;
}

// ============================================================================
// Atlas Integration
// ============================================================================

VKNVGcolorGlyphCacheEntry* vknvg__loadBitmapEmoji(VKNVGbitmapEmoji* bitmap,
                                                   uint32_t fontID,
                                                   uint32_t glyphID,
                                                   float size)
{
	if (!bitmap || !bitmap->colorAtlas) return NULL;

	// Check cache first
	VKNVGcolorGlyphCacheEntry* cached = vknvg__lookupColorGlyph(bitmap->colorAtlas,
	                                                             fontID, glyphID, (uint32_t)size);
	if (cached && cached->state == VKNVG_COLOR_GLYPH_UPLOADED) {
		vknvg__touchColorGlyph(bitmap->colorAtlas, cached);
		return cached;
	}

	// Extract bitmap
	VKNVGbitmapResult result = {0};
	int success = 0;

	if (bitmap->sbixTable) {
		success = vknvg__extractSbixBitmap(bitmap->sbixTable, glyphID, size, &result);
		if (success) bitmap->pngDecodes++;
	} else if (bitmap->cbdtTable) {
		success = vknvg__extractCbdtBitmap(bitmap->cbdtTable, glyphID, size, &result);
		if (success) bitmap->pngDecodes++;
	}

	if (!success) {
		bitmap->pngErrors++;
		return NULL;
	}

	// Allocate atlas space
	uint16_t atlasX, atlasY;
	uint32_t atlasIndex;
	if (!vknvg__allocColorGlyph(bitmap->colorAtlas, result.width, result.height,
	                            &atlasX, &atlasY, &atlasIndex)) {
		free(result.rgbaData);
		return NULL;
	}

	// Allocate cache entry
	VKNVGcolorGlyphCacheEntry* entry = vknvg__allocColorGlyphEntry(bitmap->colorAtlas,
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
	vknvg__queueColorUpload(bitmap->colorAtlas, atlasIndex, atlasX, atlasY,
	                        result.width, result.height, result.rgbaData, entry);

	bitmap->cacheUploads++;

	free(result.rgbaData);
	return entry;
}

// ============================================================================
// Utility Functions
// ============================================================================

void vknvg__bgraToRgba(uint8_t* data, uint32_t pixelCount)
{
	if (!data) return;

	for (uint32_t i = 0; i < pixelCount; i++) {
		uint8_t b = data[i * 4 + 0];
		uint8_t r = data[i * 4 + 2];
		data[i * 4 + 0] = r;
		data[i * 4 + 2] = b;
		// G and A stay in same positions
	}
}

void vknvg__premultiplyAlpha(uint8_t* rgbaData, uint32_t pixelCount)
{
	if (!rgbaData) return;

	for (uint32_t i = 0; i < pixelCount; i++) {
		uint8_t alpha = rgbaData[i * 4 + 3];
		if (alpha == 255) continue;

		rgbaData[i * 4 + 0] = (rgbaData[i * 4 + 0] * alpha) / 255;
		rgbaData[i * 4 + 1] = (rgbaData[i * 4 + 1] * alpha) / 255;
		rgbaData[i * 4 + 2] = (rgbaData[i * 4 + 2] * alpha) / 255;
	}
}

uint8_t* vknvg__scaleBitmap(const uint8_t* srcData,
                            uint32_t srcWidth, uint32_t srcHeight,
                            uint32_t targetWidth, uint32_t targetHeight)
{
	if (!srcData || srcWidth == 0 || srcHeight == 0 ||
	    targetWidth == 0 || targetHeight == 0) {
		return NULL;
	}

	uint8_t* dstData = (uint8_t*)malloc(targetWidth * targetHeight * 4);
	if (!dstData) return NULL;

	// Nearest-neighbor scaling
	for (uint32_t y = 0; y < targetHeight; y++) {
		for (uint32_t x = 0; x < targetWidth; x++) {
			uint32_t srcX = (x * srcWidth) / targetWidth;
			uint32_t srcY = (y * srcHeight) / targetHeight;

			uint32_t srcIdx = (srcY * srcWidth + srcX) * 4;
			uint32_t dstIdx = (y * targetWidth + x) * 4;

			dstData[dstIdx + 0] = srcData[srcIdx + 0];
			dstData[dstIdx + 1] = srcData[srcIdx + 1];
			dstData[dstIdx + 2] = srcData[srcIdx + 2];
			dstData[dstIdx + 3] = srcData[srcIdx + 3];
		}
	}

	return dstData;
}

void vknvg__getBitmapEmojiStats(VKNVGbitmapEmoji* bitmap,
                                uint32_t* outPngDecodes,
                                uint32_t* outPngErrors,
                                uint32_t* outCacheUploads)
{
	if (!bitmap) return;

	if (outPngDecodes) *outPngDecodes = bitmap->pngDecodes;
	if (outPngErrors) *outPngErrors = bitmap->pngErrors;
	if (outCacheUploads) *outCacheUploads = bitmap->cacheUploads;
}
