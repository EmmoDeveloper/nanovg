// nanovg_vk_text_emoji.c - Text-Emoji Integration Implementation

#include "nanovg_vk_text_emoji.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// State Management
// ============================================================================

VKNVGtextEmojiState* vknvg__createTextEmojiState(VKNVGcolorAtlas* colorAtlas,
                                                  const uint8_t* fontData,
                                                  uint32_t fontDataSize)
{
	if (!colorAtlas || !fontData) return NULL;

	VKNVGtextEmojiState* state = (VKNVGtextEmojiState*)calloc(1, sizeof(VKNVGtextEmojiState));
	if (!state) return NULL;

	// Try to create emoji renderer (may fail if no emoji tables)
	state->emojiRenderer = vknvg__createEmojiRenderer(colorAtlas, fontData, fontDataSize);

	return state;
}

void vknvg__destroyTextEmojiState(VKNVGtextEmojiState* state)
{
	if (!state) return;

	if (state->emojiRenderer) {
		vknvg__destroyEmojiRenderer(state->emojiRenderer);
	}

	free(state);
}

bool vknvg__hasEmojiSupport(VKNVGtextEmojiState* state)
{
	if (!state || !state->emojiRenderer) return false;

	VKNVGemojiFormat format = vknvg__getEmojiFormat(state->emojiRenderer);
	return format != VKNVG_EMOJI_NONE;
}

// ============================================================================
// Glyph Rendering
// ============================================================================

bool vknvg__getGlyphRenderInfo(VKNVGtextEmojiState* state,
                               uint32_t fontID,
                               uint32_t codepoint,
                               uint32_t glyphID,
                               float size,
                               VKNVGglyphRenderInfo* outInfo)
{
	if (!state || !outInfo) return false;

	// Clear output
	memset(outInfo, 0, sizeof(VKNVGglyphRenderInfo));

	// Try emoji first if available and codepoint looks like emoji
	if (vknvg__shouldRenderAsEmoji(state, codepoint)) {
		if (vknvg__tryRenderAsEmoji(state, fontID, codepoint, glyphID, size, outInfo)) {
			state->glyphsRenderedEmoji++;
			return true;
		}
	}

	// Fallback to SDF (not implemented here - caller handles SDF rendering)
	outInfo->mode = VKNVG_GLYPH_SDF;
	state->glyphsRenderedSDF++;

	return true;
}

bool vknvg__tryRenderAsEmoji(VKNVGtextEmojiState* state,
                             uint32_t fontID,
                             uint32_t codepoint,
                             uint32_t glyphID,
                             float size,
                             VKNVGglyphRenderInfo* outInfo)
{
	if (!state || !state->emojiRenderer || !outInfo) return false;

	state->emojiAttempts++;

	// Try to render emoji
	VKNVGcolorGlyphCacheEntry* entry = vknvg__renderEmoji(state->emojiRenderer,
	                                                       fontID, glyphID, size);

	if (!entry || entry->state != VKNVG_COLOR_GLYPH_UPLOADED) {
		state->emojiFallbacks++;
		return false;
	}

	// Fill render info
	outInfo->mode = VKNVG_GLYPH_COLOR_EMOJI;
	outInfo->colorAtlasIndex = entry->atlasIndex;
	outInfo->colorAtlasX = entry->atlasX;
	outInfo->colorAtlasY = entry->atlasY;
	outInfo->colorWidth = entry->width;
	outInfo->colorHeight = entry->height;
	outInfo->bearingX = entry->bearingX;
	outInfo->bearingY = entry->bearingY;
	outInfo->advance = entry->advance;

	return true;
}

// ============================================================================
// Detection & Classification
// ============================================================================

bool vknvg__shouldRenderAsEmoji(VKNVGtextEmojiState* state,
                                uint32_t codepoint)
{
	if (!state || !state->emojiRenderer) return false;

	// Check if it's in emoji Unicode ranges
	return vknvg__isEmojiCodepoint(codepoint);
}

bool vknvg__isEmojiSequenceCodepoint(uint32_t codepoint)
{
	// ZWJ (Zero-Width Joiner)
	if (vknvg__isZWJ(codepoint)) return true;

	// Variation Selector-16 (force emoji presentation)
	if (codepoint == 0xFE0F) return true;

	// Skin tone modifiers
	if (vknvg__isEmojiModifier(codepoint)) return true;

	// Combining enclosing keycap
	if (codepoint == 0x20E3) return true;

	return false;
}

// ============================================================================
// Statistics
// ============================================================================

void vknvg__getTextEmojiStats(VKNVGtextEmojiState* state,
                              uint32_t* outGlyphsSDF,
                              uint32_t* outGlyphsEmoji,
                              uint32_t* outEmojiAttempts,
                              uint32_t* outEmojiFallbacks)
{
	if (!state) return;

	if (outGlyphsSDF) *outGlyphsSDF = state->glyphsRenderedSDF;
	if (outGlyphsEmoji) *outGlyphsEmoji = state->glyphsRenderedEmoji;
	if (outEmojiAttempts) *outEmojiAttempts = state->emojiAttempts;
	if (outEmojiFallbacks) *outEmojiFallbacks = state->emojiFallbacks;
}

void vknvg__resetTextEmojiStats(VKNVGtextEmojiState* state)
{
	if (!state) return;

	state->glyphsRenderedSDF = 0;
	state->glyphsRenderedEmoji = 0;
	state->emojiAttempts = 0;
	state->emojiFallbacks = 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* vknvg__getGlyphRenderModeName(VKNVGglyphRenderMode mode)
{
	switch (mode) {
		case VKNVG_GLYPH_SDF: return "SDF";
		case VKNVG_GLYPH_COLOR_EMOJI: return "Color Emoji";
		default: return "Unknown";
	}
}

void vknvg__printGlyphRenderInfo(const VKNVGglyphRenderInfo* info)
{
	if (!info) {
		printf("(null)\n");
		return;
	}

	printf("Glyph Render Info:\n");
	printf("  Mode: %s\n", vknvg__getGlyphRenderModeName(info->mode));

	if (info->mode == VKNVG_GLYPH_SDF) {
		printf("  SDF Atlas: (%u, %u) size: %ux%u\n",
		       info->sdfAtlasX, info->sdfAtlasY,
		       info->sdfWidth, info->sdfHeight);
	} else if (info->mode == VKNVG_GLYPH_COLOR_EMOJI) {
		printf("  Color Atlas [%u]: (%u, %u) size: %ux%u\n",
		       info->colorAtlasIndex,
		       info->colorAtlasX, info->colorAtlasY,
		       info->colorWidth, info->colorHeight);
	}

	printf("  Bearing: (%d, %d) Advance: %u\n",
	       info->bearingX, info->bearingY, info->advance);
}
