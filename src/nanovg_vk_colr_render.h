// nanovg_vk_colr_render.h - COLR Vector Emoji Rendering
//
// Phase 6.4: COLR Vector Rendering
//
// Rasterizes layered COLR glyphs with CPAL palette colors and composites
// them into RGBA bitmaps for upload to the color atlas.

#ifndef NANOVG_VK_COLR_RENDER_H
#define NANOVG_VK_COLR_RENDER_H

#include <stdint.h>
#include <stdbool.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "nanovg_vk_emoji_tables.h"
#include "nanovg_vk_color_atlas.h"

// Forward declarations
typedef struct VKNVGcolrRenderer VKNVGcolrRenderer;
typedef struct VKNVGcolrRenderResult VKNVGcolrRenderResult;

// COLR rendering result
struct VKNVGcolrRenderResult {
	uint8_t* rgbaData;		// RGBA8 pixel data (caller must free)
	uint32_t width;
	uint32_t height;
	int16_t bearingX;		// Horizontal bearing
	int16_t bearingY;		// Vertical bearing
	uint16_t advance;		// Horizontal advance
};

// COLR renderer state
struct VKNVGcolrRenderer {
	// FreeType context
	FT_Library ftLibrary;
	FT_Face ftFace;

	// Font tables
	VKNVGcolrTable* colrTable;
	VKNVGcpalTable* cpalTable;

	// Color atlas (for uploads)
	VKNVGcolorAtlas* colorAtlas;

	// Rendering parameters
	uint16_t currentPalette;	// Active palette index

	// Statistics
	uint32_t layerRenders;
	uint32_t composites;
	uint32_t cacheUploads;
};

// ============================================================================
// Renderer Management
// ============================================================================

// Create COLR renderer
VKNVGcolrRenderer* vknvg__createColrRenderer(VKNVGcolorAtlas* colorAtlas,
                                              const uint8_t* fontData,
                                              uint32_t fontDataSize);

// Destroy COLR renderer
void vknvg__destroyColrRenderer(VKNVGcolrRenderer* renderer);

// Set active palette
void vknvg__setColrPalette(VKNVGcolrRenderer* renderer, uint16_t paletteIndex);

// ============================================================================
// Layer Rasterization
// ============================================================================

// Rasterize single glyph layer as grayscale
// Returns NULL on failure (caller must free)
uint8_t* vknvg__rasterizeGlyphLayer(VKNVGcolrRenderer* renderer,
                                     uint32_t glyphID,
                                     float size,
                                     uint32_t* outWidth,
                                     uint32_t* outHeight,
                                     int32_t* outBearingX,
                                     int32_t* outBearingY,
                                     uint32_t* outAdvance);

// ============================================================================
// Color Application
// ============================================================================

// Apply palette color to grayscale bitmap
// Converts grayscale to RGBA with palette color
// outRgba must be allocated (width * height * 4 bytes)
void vknvg__applyPaletteColor(const uint8_t* grayscale,
                              uint32_t width, uint32_t height,
                              VKNVGcpalColor color,
                              uint8_t* outRgba);

// Apply foreground color (special paletteIndex 0xFFFF)
void vknvg__applyForegroundColor(const uint8_t* grayscale,
                                 uint32_t width, uint32_t height,
                                 uint32_t foregroundColor,
                                 uint8_t* outRgba);

// ============================================================================
// Layer Compositing
// ============================================================================

// Composite RGBA layer onto destination using alpha blending
void vknvg__compositeLayer(const uint8_t* srcRgba,
                           uint8_t* dstRgba,
                           uint32_t width, uint32_t height);

// Composite RGBA layer with offset (for different layer sizes)
void vknvg__compositeLayerWithOffset(const uint8_t* srcRgba,
                                     uint32_t srcWidth, uint32_t srcHeight,
                                     int32_t srcBearingX, int32_t srcBearingY,
                                     uint8_t* dstRgba,
                                     uint32_t dstWidth, uint32_t dstHeight,
                                     int32_t dstBearingX, int32_t dstBearingY);

// ============================================================================
// COLR Rendering
// ============================================================================

// Render COLR glyph to RGBA bitmap
// Returns 0 on failure (caller must free result.rgbaData)
int vknvg__renderColrGlyph(VKNVGcolrRenderer* renderer,
                           uint32_t glyphID,
                           float size,
                           VKNVGcolrRenderResult* outResult);

// ============================================================================
// Atlas Integration
// ============================================================================

// Load COLR emoji into color atlas
// Returns cache entry on success, NULL on failure
VKNVGcolorGlyphCacheEntry* vknvg__loadColrEmoji(VKNVGcolrRenderer* renderer,
                                                 uint32_t fontID,
                                                 uint32_t glyphID,
                                                 float size);

// ============================================================================
// Utility Functions
// ============================================================================

// Calculate bounding box for COLR glyph layers
void vknvg__calculateColrBounds(VKNVGcolrRenderer* renderer,
                                VKNVGcolrGlyph* colrGlyph,
                                float size,
                                uint32_t* outWidth,
                                uint32_t* outHeight,
                                int32_t* outBearingX,
                                int32_t* outBearingY);

// Get renderer statistics
void vknvg__getColrRendererStats(VKNVGcolrRenderer* renderer,
                                 uint32_t* outLayerRenders,
                                 uint32_t* outComposites,
                                 uint32_t* outCacheUploads);

#endif // NANOVG_VK_COLR_RENDER_H
