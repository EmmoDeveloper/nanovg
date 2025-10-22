// nanovg_vk_text_emoji.h - Text-Emoji Integration
//
// Phase 6.6: Text Pipeline Integration
//
// Integrates emoji rendering into the text pipeline with automatic detection,
// format selection, and fallback to grayscale SDF when color is unavailable.

#ifndef NANOVG_VK_TEXT_EMOJI_H
#define NANOVG_VK_TEXT_EMOJI_H

#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_emoji.h"

// Forward declarations
typedef struct VKNVGtextEmojiState VKNVGtextEmojiState;
typedef struct VKNVGglyphRenderInfo VKNVGglyphRenderInfo;

// Glyph rendering mode
typedef enum VKNVGglyphRenderMode {
	VKNVG_GLYPH_SDF = 0,		// Render as grayscale SDF
	VKNVG_GLYPH_COLOR_EMOJI,	// Render as color emoji (RGBA)
} VKNVGglyphRenderMode;

// Glyph rendering information
struct VKNVGglyphRenderInfo {
	VKNVGglyphRenderMode mode;

	// For SDF mode
	uint16_t sdfAtlasX;
	uint16_t sdfAtlasY;
	uint16_t sdfWidth;
	uint16_t sdfHeight;

	// For color emoji mode
	uint32_t colorAtlasIndex;
	uint16_t colorAtlasX;
	uint16_t colorAtlasY;
	uint16_t colorWidth;
	uint16_t colorHeight;

	// Common metrics
	int16_t bearingX;
	int16_t bearingY;
	uint16_t advance;
};

// Text-emoji integration state
struct VKNVGtextEmojiState {
	// Emoji renderer (may be NULL if no emoji font)
	VKNVGemojiRenderer* emojiRenderer;

	// Statistics
	uint32_t glyphsRenderedSDF;
	uint32_t glyphsRenderedEmoji;
	uint32_t emojiAttempts;
	uint32_t emojiFallbacks;
};

// ============================================================================
// State Management
// ============================================================================

// Create text-emoji state
VKNVGtextEmojiState* vknvg__createTextEmojiState(VKNVGcolorAtlas* colorAtlas,
                                                  const uint8_t* fontData,
                                                  uint32_t fontDataSize);

// Destroy text-emoji state
void vknvg__destroyTextEmojiState(VKNVGtextEmojiState* state);

// Check if emoji rendering is available
bool vknvg__hasEmojiSupport(VKNVGtextEmojiState* state);

// ============================================================================
// Glyph Rendering
// ============================================================================

// Get rendering info for a glyph
// Automatically detects if glyph should be rendered as emoji or SDF
// Returns true if glyph should be rendered, false if not found
bool vknvg__getGlyphRenderInfo(VKNVGtextEmojiState* state,
                               uint32_t fontID,
                               uint32_t codepoint,
                               uint32_t glyphID,
                               float size,
                               VKNVGglyphRenderInfo* outInfo);

// Try to render glyph as color emoji
// Returns true if successful, false if should fall back to SDF
bool vknvg__tryRenderAsEmoji(VKNVGtextEmojiState* state,
                             uint32_t fontID,
                             uint32_t codepoint,
                             uint32_t glyphID,
                             float size,
                             VKNVGglyphRenderInfo* outInfo);

// ============================================================================
// Detection & Classification
// ============================================================================

// Classify codepoint for rendering
// Returns true if should attempt emoji rendering
bool vknvg__shouldRenderAsEmoji(VKNVGtextEmojiState* state,
                                uint32_t codepoint);

// Check if codepoint is part of emoji sequence
// (ZWJ, variation selector, skin tone modifier, etc.)
bool vknvg__isEmojiSequenceCodepoint(uint32_t codepoint);

// ============================================================================
// Statistics
// ============================================================================

// Get rendering statistics
void vknvg__getTextEmojiStats(VKNVGtextEmojiState* state,
                              uint32_t* outGlyphsSDF,
                              uint32_t* outGlyphsEmoji,
                              uint32_t* outEmojiAttempts,
                              uint32_t* outEmojiFallbacks);

// Reset statistics
void vknvg__resetTextEmojiStats(VKNVGtextEmojiState* state);

// ============================================================================
// Utility Functions
// ============================================================================

// Get glyph render mode name
const char* vknvg__getGlyphRenderModeName(VKNVGglyphRenderMode mode);

// Print glyph render info (for debugging)
void vknvg__printGlyphRenderInfo(const VKNVGglyphRenderInfo* info);

#endif // NANOVG_VK_TEXT_EMOJI_H
