// nanovg_vk_emoji.h - Unified Emoji Rendering System
//
// Phase 6.5: Emoji Integration Layer
//
// Combines all emoji rendering components (SBIX, CBDT, COLR) into a unified
// API with automatic format detection and routing.

#ifndef NANOVG_VK_EMOJI_H
#define NANOVG_VK_EMOJI_H

#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_emoji_tables.h"
#include "nanovg_vk_color_atlas.h"
#include "nanovg_vk_bitmap_emoji.h"
#include "nanovg_vk_colr_render.h"

// Forward declarations
typedef struct VKNVGemojiRenderer VKNVGemojiRenderer;
typedef struct VKNVGemojiMetrics VKNVGemojiMetrics;

// Emoji glyph metrics
struct VKNVGemojiMetrics {
	uint32_t width;
	uint32_t height;
	int16_t bearingX;
	int16_t bearingY;
	uint16_t advance;

	// Atlas location (if cached)
	uint32_t atlasIndex;
	uint16_t atlasX;
	uint16_t atlasY;
};

// Unified emoji renderer
struct VKNVGemojiRenderer {
	// Color atlas (shared by all formats)
	VKNVGcolorAtlas* colorAtlas;

	// Font data
	const uint8_t* fontData;
	uint32_t fontDataSize;

	// Format-specific renderers
	VKNVGemojiFormat format;
	VKNVGbitmapEmoji* bitmapRenderer;	// For SBIX/CBDT
	VKNVGcolrRenderer* colrRenderer;	// For COLR

	// Statistics
	uint32_t emojiRendered;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t formatSwitches;
};

// ============================================================================
// Renderer Management
// ============================================================================

// Create unified emoji renderer
// Automatically detects available emoji format and initializes appropriate renderer
VKNVGemojiRenderer* vknvg__createEmojiRenderer(VKNVGcolorAtlas* colorAtlas,
                                                const uint8_t* fontData,
                                                uint32_t fontDataSize);

// Destroy emoji renderer
void vknvg__destroyEmojiRenderer(VKNVGemojiRenderer* renderer);

// Get detected emoji format
VKNVGemojiFormat vknvg__getEmojiFormat(VKNVGemojiRenderer* renderer);

// ============================================================================
// Emoji Rendering
// ============================================================================

// Render emoji and upload to color atlas
// Returns cache entry on success, NULL on failure
// Automatically routes to SBIX, CBDT, or COLR renderer based on format
VKNVGcolorGlyphCacheEntry* vknvg__renderEmoji(VKNVGemojiRenderer* renderer,
                                               uint32_t fontID,
                                               uint32_t glyphID,
                                               float size);

// Get emoji metrics without rendering
// Returns true if emoji exists and metrics are valid
bool vknvg__getEmojiMetrics(VKNVGemojiRenderer* renderer,
                            uint32_t glyphID,
                            float size,
                            VKNVGemojiMetrics* outMetrics);

// Check if glyph is a color emoji
bool vknvg__isColorEmoji(VKNVGemojiRenderer* renderer, uint32_t glyphID);

// ============================================================================
// Codepoint Detection
// ============================================================================

// Check if Unicode codepoint is typically an emoji
// Uses Unicode emoji ranges for detection
bool vknvg__isEmojiCodepoint(uint32_t codepoint);

// Check if codepoint is an emoji modifier
bool vknvg__isEmojiModifier(uint32_t codepoint);

// Check if codepoint is ZWJ (Zero-Width Joiner)
bool vknvg__isZWJ(uint32_t codepoint);

// ============================================================================
// Statistics
// ============================================================================

// Get rendering statistics
void vknvg__getEmojiStats(VKNVGemojiRenderer* renderer,
                          uint32_t* outEmojiRendered,
                          uint32_t* outCacheHits,
                          uint32_t* outCacheMisses,
                          uint32_t* outFormatSwitches);

// Reset statistics
void vknvg__resetEmojiStats(VKNVGemojiRenderer* renderer);

// ============================================================================
// Utility Functions
// ============================================================================

// Get format name as string
const char* vknvg__getEmojiFormatName(VKNVGemojiFormat format);

// Check if format supports color
bool vknvg__formatSupportsColor(VKNVGemojiFormat format);

// Get recommended size for emoji format
// Returns size in pixels for best quality
float vknvg__getRecommendedEmojiSize(VKNVGemojiRenderer* renderer, float requestedSize);

#endif // NANOVG_VK_EMOJI_H
