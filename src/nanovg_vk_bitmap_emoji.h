// nanovg_vk_bitmap_emoji.h - Bitmap Emoji Pipeline
//
// Phase 6.3: Bitmap Emoji Pipeline
//
// Extracts PNG-embedded bitmap emoji from SBIX/CBDT tables and uploads
// to RGBA color atlas. Handles strike selection for different font sizes.

#ifndef NANOVG_VK_BITMAP_EMOJI_H
#define NANOVG_VK_BITMAP_EMOJI_H

#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_emoji_tables.h"
#include "nanovg_vk_color_atlas.h"

// Forward declarations
typedef struct VKNVGbitmapEmoji VKNVGbitmapEmoji;
typedef struct VKNVGbitmapResult VKNVGbitmapResult;

// Bitmap decoding result
struct VKNVGbitmapResult {
	uint8_t* rgbaData;		// RGBA8 pixel data (caller must free)
	uint32_t width;
	uint32_t height;
	int16_t bearingX;		// Horizontal bearing
	int16_t bearingY;		// Vertical bearing
	uint16_t advance;		// Horizontal advance
};

// Bitmap emoji pipeline state
struct VKNVGbitmapEmoji {
	// Font tables (parsed once, reused)
	VKNVGsbixTable* sbixTable;
	VKNVGcbdtTable* cbdtTable;

	// Color atlas (for uploads)
	VKNVGcolorAtlas* colorAtlas;

	// Statistics
	uint32_t pngDecodes;
	uint32_t pngErrors;
	uint32_t cacheUploads;
};

// ============================================================================
// Pipeline Management
// ============================================================================

// Create bitmap emoji pipeline
VKNVGbitmapEmoji* vknvg__createBitmapEmoji(VKNVGcolorAtlas* colorAtlas,
                                            const uint8_t* fontData,
                                            uint32_t fontDataSize);

// Destroy bitmap emoji pipeline
void vknvg__destroyBitmapEmoji(VKNVGbitmapEmoji* bitmap);

// ============================================================================
// PNG Decoding
// ============================================================================

// Decode PNG from memory to RGBA8
// Returns NULL on failure
uint8_t* vknvg__decodePNG(const uint8_t* pngData,
                          uint32_t pngSize,
                          uint32_t* outWidth,
                          uint32_t* outHeight);

// ============================================================================
// SBIX Extraction
// ============================================================================

// Extract bitmap from SBIX strike
// Returns NULL on failure (caller must free result.rgbaData)
int vknvg__extractSbixBitmap(VKNVGsbixTable* sbixTable,
                             uint32_t glyphID,
                             float size,
                             VKNVGbitmapResult* outResult);

// Select best SBIX strike for given size
VKNVGsbixStrike* vknvg__selectBestSbixStrike(VKNVGsbixTable* sbixTable, float size);

// ============================================================================
// CBDT Extraction
// ============================================================================

// Extract bitmap from CBDT table
// Returns NULL on failure (caller must free result.rgbaData)
int vknvg__extractCbdtBitmap(VKNVGcbdtTable* cbdtTable,
                             uint32_t glyphID,
                             float size,
                             VKNVGbitmapResult* outResult);

// Select best CBDT size for given size
VKNVGcblcBitmapSize* vknvg__selectBestCbdtSize(VKNVGcbdtTable* cbdtTable, float size);

// ============================================================================
// Atlas Integration
// ============================================================================

// Load bitmap emoji into color atlas
// Returns cache entry on success, NULL on failure
VKNVGcolorGlyphCacheEntry* vknvg__loadBitmapEmoji(VKNVGbitmapEmoji* bitmap,
                                                   uint32_t fontID,
                                                   uint32_t glyphID,
                                                   float size);

// ============================================================================
// Utility Functions
// ============================================================================

// Convert BGRA to RGBA (in-place)
void vknvg__bgraToRgba(uint8_t* data, uint32_t pixelCount);

// Premultiply alpha (in-place)
void vknvg__premultiplyAlpha(uint8_t* rgbaData, uint32_t pixelCount);

// Scale bitmap to target size (simple nearest-neighbor)
uint8_t* vknvg__scaleBitmap(const uint8_t* srcData,
                            uint32_t srcWidth, uint32_t srcHeight,
                            uint32_t targetWidth, uint32_t targetHeight);

// Get pipeline statistics
void vknvg__getBitmapEmojiStats(VKNVGbitmapEmoji* bitmap,
                                uint32_t* outPngDecodes,
                                uint32_t* outPngErrors,
                                uint32_t* outCacheUploads);

#endif // NANOVG_VK_BITMAP_EMOJI_H
