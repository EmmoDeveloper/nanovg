// nanovg_vk_emoji.c - Unified Emoji Rendering System Implementation

#include "nanovg_vk_emoji.h"
#include <stdlib.h>
#include <string.h>

// Unicode emoji ranges (simplified)
#define EMOJI_RANGE_1_START 0x1F300
#define EMOJI_RANGE_1_END   0x1F6FF
#define EMOJI_RANGE_2_START 0x1F900
#define EMOJI_RANGE_2_END   0x1FAFF
#define EMOJI_RANGE_3_START 0x2600
#define EMOJI_RANGE_3_END   0x27BF

// Special emoji codepoints
#define ZWJ_CODEPOINT 0x200D
#define VARIATION_SELECTOR_16 0xFE0F

// Emoji modifiers (skin tone)
#define EMOJI_MODIFIER_START 0x1F3FB
#define EMOJI_MODIFIER_END   0x1F3FF

// ============================================================================
// Renderer Management
// ============================================================================

VKNVGemojiRenderer* vknvg__createEmojiRenderer(VKNVGcolorAtlas* colorAtlas,
                                                const uint8_t* fontData,
                                                uint32_t fontDataSize)
{
	if (!colorAtlas || !fontData) return NULL;

	VKNVGemojiRenderer* renderer = (VKNVGemojiRenderer*)calloc(1, sizeof(VKNVGemojiRenderer));
	if (!renderer) return NULL;

	renderer->colorAtlas = colorAtlas;
	renderer->fontData = fontData;
	renderer->fontDataSize = fontDataSize;

	// Detect emoji format
	renderer->format = vknvg__detectEmojiFormat(fontData, fontDataSize);

	// Initialize appropriate renderer
	if (renderer->format == VKNVG_EMOJI_SBIX || renderer->format == VKNVG_EMOJI_CBDT) {
		renderer->bitmapRenderer = vknvg__createBitmapEmoji(colorAtlas, fontData, fontDataSize);
	} else if (renderer->format == VKNVG_EMOJI_COLR) {
		renderer->colrRenderer = vknvg__createColrRenderer(colorAtlas, fontData, fontDataSize);
	}

	return renderer;
}

void vknvg__destroyEmojiRenderer(VKNVGemojiRenderer* renderer)
{
	if (!renderer) return;

	if (renderer->bitmapRenderer) {
		vknvg__destroyBitmapEmoji(renderer->bitmapRenderer);
	}
	if (renderer->colrRenderer) {
		vknvg__destroyColrRenderer(renderer->colrRenderer);
	}

	free(renderer);
}

VKNVGemojiFormat vknvg__getEmojiFormat(VKNVGemojiRenderer* renderer)
{
	if (!renderer) return VKNVG_EMOJI_NONE;
	return renderer->format;
}

// ============================================================================
// Emoji Rendering
// ============================================================================

VKNVGcolorGlyphCacheEntry* vknvg__renderEmoji(VKNVGemojiRenderer* renderer,
                                               uint32_t fontID,
                                               uint32_t glyphID,
                                               float size)
{
	if (!renderer) return NULL;

	// Check cache first
	VKNVGcolorGlyphCacheEntry* cached = vknvg__lookupColorGlyph(renderer->colorAtlas,
	                                                             fontID, glyphID, (uint32_t)size);
	if (cached && cached->state == VKNVG_COLOR_GLYPH_UPLOADED) {
		vknvg__touchColorGlyph(renderer->colorAtlas, cached);
		renderer->cacheHits++;
		return cached;
	}

	renderer->cacheMisses++;

	// Route to appropriate renderer
	VKNVGcolorGlyphCacheEntry* entry = NULL;

	if (renderer->format == VKNVG_EMOJI_SBIX || renderer->format == VKNVG_EMOJI_CBDT) {
		if (renderer->bitmapRenderer) {
			entry = vknvg__loadBitmapEmoji(renderer->bitmapRenderer, fontID, glyphID, size);
		}
	} else if (renderer->format == VKNVG_EMOJI_COLR) {
		if (renderer->colrRenderer) {
			entry = vknvg__loadColrEmoji(renderer->colrRenderer, fontID, glyphID, size);
		}
	}

	if (entry) {
		renderer->emojiRendered++;
	}

	return entry;
}

bool vknvg__getEmojiMetrics(VKNVGemojiRenderer* renderer,
                            uint32_t glyphID,
                            float size,
                            VKNVGemojiMetrics* outMetrics)
{
	if (!renderer || !outMetrics) return false;

	// Check if already cached
	VKNVGcolorGlyphCacheEntry* cached = vknvg__lookupColorGlyph(renderer->colorAtlas,
	                                                             0, glyphID, (uint32_t)size);
	if (cached && cached->state == VKNVG_COLOR_GLYPH_UPLOADED) {
		outMetrics->width = cached->width;
		outMetrics->height = cached->height;
		outMetrics->bearingX = cached->bearingX;
		outMetrics->bearingY = cached->bearingY;
		outMetrics->advance = cached->advance;
		outMetrics->atlasIndex = cached->atlasIndex;
		outMetrics->atlasX = cached->atlasX;
		outMetrics->atlasY = cached->atlasY;
		return true;
	}

	// Would need to render to get metrics - not implemented for now
	return false;
}

bool vknvg__isColorEmoji(VKNVGemojiRenderer* renderer, uint32_t glyphID)
{
	if (!renderer) return false;

	// Check format-specific tables
	if (renderer->format == VKNVG_EMOJI_SBIX && renderer->bitmapRenderer) {
		// Check if glyph exists in SBIX table
		return renderer->bitmapRenderer->sbixTable != NULL;
	} else if (renderer->format == VKNVG_EMOJI_CBDT && renderer->bitmapRenderer) {
		// Check if glyph exists in CBDT table
		return renderer->bitmapRenderer->cbdtTable != NULL;
	} else if (renderer->format == VKNVG_EMOJI_COLR && renderer->colrRenderer) {
		// Check if glyph exists in COLR table
		VKNVGcolrGlyph* colrGlyph = vknvg__getColrGlyph(renderer->colrRenderer->colrTable, glyphID);
		return colrGlyph != NULL && colrGlyph->numLayers > 0;
	}

	return false;
}

// ============================================================================
// Codepoint Detection
// ============================================================================

bool vknvg__isEmojiCodepoint(uint32_t codepoint)
{
	// Check emoji ranges
	if (codepoint >= EMOJI_RANGE_1_START && codepoint <= EMOJI_RANGE_1_END) return true;
	if (codepoint >= EMOJI_RANGE_2_START && codepoint <= EMOJI_RANGE_2_END) return true;
	if (codepoint >= EMOJI_RANGE_3_START && codepoint <= EMOJI_RANGE_3_END) return true;

	// Common single emoji
	if (codepoint == 0x203C || codepoint == 0x2049) return true; // ‼️ ⁉️
	if (codepoint >= 0x2194 && codepoint <= 0x2199) return true; // Arrows
	if (codepoint >= 0x21A9 && codepoint <= 0x21AA) return true; // More arrows
	if (codepoint >= 0x231A && codepoint <= 0x231B) return true; // ⌚ ⌛
	if (codepoint >= 0x2328 && codepoint <= 0x2328) return true; // ⌨️
	if (codepoint >= 0x23CF && codepoint <= 0x23CF) return true; // ⏏️
	if (codepoint >= 0x23E9 && codepoint <= 0x23F3) return true; // Media controls
	if (codepoint >= 0x23F8 && codepoint <= 0x23FA) return true; // More media

	return false;
}

bool vknvg__isEmojiModifier(uint32_t codepoint)
{
	return codepoint >= EMOJI_MODIFIER_START && codepoint <= EMOJI_MODIFIER_END;
}

bool vknvg__isZWJ(uint32_t codepoint)
{
	return codepoint == ZWJ_CODEPOINT;
}

// ============================================================================
// Statistics
// ============================================================================

void vknvg__getEmojiStats(VKNVGemojiRenderer* renderer,
                          uint32_t* outEmojiRendered,
                          uint32_t* outCacheHits,
                          uint32_t* outCacheMisses,
                          uint32_t* outFormatSwitches)
{
	if (!renderer) return;

	if (outEmojiRendered) *outEmojiRendered = renderer->emojiRendered;
	if (outCacheHits) *outCacheHits = renderer->cacheHits;
	if (outCacheMisses) *outCacheMisses = renderer->cacheMisses;
	if (outFormatSwitches) *outFormatSwitches = renderer->formatSwitches;
}

void vknvg__resetEmojiStats(VKNVGemojiRenderer* renderer)
{
	if (!renderer) return;

	renderer->emojiRendered = 0;
	renderer->cacheHits = 0;
	renderer->cacheMisses = 0;
	renderer->formatSwitches = 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* vknvg__getEmojiFormatName(VKNVGemojiFormat format)
{
	switch (format) {
		case VKNVG_EMOJI_NONE: return "None";
		case VKNVG_EMOJI_SBIX: return "SBIX (Apple Bitmap)";
		case VKNVG_EMOJI_CBDT: return "CBDT (Google Bitmap)";
		case VKNVG_EMOJI_COLR: return "COLR (Vector)";
		case VKNVG_EMOJI_SVG:  return "SVG (Not Supported)";
		default: return "Unknown";
	}
}

bool vknvg__formatSupportsColor(VKNVGemojiFormat format)
{
	return format == VKNVG_EMOJI_SBIX ||
	       format == VKNVG_EMOJI_CBDT ||
	       format == VKNVG_EMOJI_COLR ||
	       format == VKNVG_EMOJI_SVG;
}

float vknvg__getRecommendedEmojiSize(VKNVGemojiRenderer* renderer, float requestedSize)
{
	if (!renderer) return requestedSize;

	// For bitmap formats, snap to available strikes
	if (renderer->format == VKNVG_EMOJI_SBIX && renderer->bitmapRenderer && renderer->bitmapRenderer->sbixTable) {
		VKNVGsbixStrike* strike = vknvg__selectBestSbixStrike(renderer->bitmapRenderer->sbixTable, requestedSize);
		if (strike) {
			return (float)strike->ppem;
		}
	} else if (renderer->format == VKNVG_EMOJI_CBDT && renderer->bitmapRenderer && renderer->bitmapRenderer->cbdtTable) {
		VKNVGcblcBitmapSize* size = vknvg__selectBestCbdtSize(renderer->bitmapRenderer->cbdtTable, requestedSize);
		if (size) {
			return (float)size->ppemY;
		}
	}

	// For vector format, any size is fine
	return requestedSize;
}
