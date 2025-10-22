// nanovg_vk_harfbuzz.c - HarfBuzz Integration Implementation
//
// Real implementation using HarfBuzz library for complex text shaping

#include "nanovg_vk_harfbuzz.h"
#include <hb.h>
#include <hb-ft.h>
#include <stdlib.h>
#include <string.h>

// Create HarfBuzz context
VKNVGharfbuzzContext* vknvg__createHarfBuzzContext(void)
{
	VKNVGharfbuzzContext* ctx = (VKNVGharfbuzzContext*)calloc(1, sizeof(VKNVGharfbuzzContext));
	if (!ctx) return NULL;

	// Create reusable buffer
	ctx->buffer = hb_buffer_create();
	if (!hb_buffer_allocation_successful(ctx->buffer)) {
		free(ctx);
		return NULL;
	}

	ctx->fontCapacity = 256;
	ctx->fontCount = 0;
	ctx->fontMapCount = 0;

	return ctx;
}

// Destroy HarfBuzz context
void vknvg__destroyHarfBuzzContext(VKNVGharfbuzzContext* ctx)
{
	if (!ctx) return;

	// Destroy all HarfBuzz fonts
	for (uint32_t i = 0; i < ctx->fontMapCount; i++) {
		if (ctx->fontMap[i].hbFont) {
			hb_font_destroy(ctx->fontMap[i].hbFont);
		}
	}

	// Destroy buffer
	if (ctx->buffer) {
		hb_buffer_destroy(ctx->buffer);
	}

	free(ctx);
}

// Register font with HarfBuzz
int vknvg__registerHarfBuzzFont(VKNVGharfbuzzContext* ctx,
                                int nvgFontID,
                                FT_Face ftFace)
{
	if (!ctx || !ftFace) return -1;
	if (ctx->fontMapCount >= 256) return -1;

	// Check if font already registered
	for (uint32_t i = 0; i < ctx->fontMapCount; i++) {
		if (ctx->fontMap[i].nvgFontID == nvgFontID) {
			return 0;	// Already registered
		}
	}

	// Create HarfBuzz font from FreeType face
	hb_font_t* hbFont = hb_ft_font_create(ftFace, NULL);
	if (!hbFont) return -1;

	// Store in map
	ctx->fontMap[ctx->fontMapCount].nvgFontID = nvgFontID;
	ctx->fontMap[ctx->fontMapCount].hbFont = hbFont;
	ctx->fontMap[ctx->fontMapCount].ftFace = ftFace;
	ctx->fontMapCount++;

	return 0;
}

// Unregister font
void vknvg__unregisterHarfBuzzFont(VKNVGharfbuzzContext* ctx, int nvgFontID)
{
	if (!ctx) return;

	for (uint32_t i = 0; i < ctx->fontMapCount; i++) {
		if (ctx->fontMap[i].nvgFontID == nvgFontID) {
			// Destroy HarfBuzz font
			if (ctx->fontMap[i].hbFont) {
				hb_font_destroy(ctx->fontMap[i].hbFont);
			}

			// Remove from map (shift remaining entries)
			for (uint32_t j = i; j < ctx->fontMapCount - 1; j++) {
				ctx->fontMap[j] = ctx->fontMap[j + 1];
			}
			ctx->fontMapCount--;
			return;
		}
	}
}

// Convert VKNVGscript to HarfBuzz script
static hb_script_t vknvg__scriptToHarfBuzz(VKNVGscript script)
{
	switch (script) {
		case VKNVG_SCRIPT_LATIN:		return HB_SCRIPT_LATIN;
		case VKNVG_SCRIPT_ARABIC:		return HB_SCRIPT_ARABIC;
		case VKNVG_SCRIPT_HEBREW:		return HB_SCRIPT_HEBREW;
		case VKNVG_SCRIPT_DEVANAGARI:	return HB_SCRIPT_DEVANAGARI;
		case VKNVG_SCRIPT_BENGALI:		return HB_SCRIPT_BENGALI;
		case VKNVG_SCRIPT_THAI:			return HB_SCRIPT_THAI;
		case VKNVG_SCRIPT_KHMER:		return HB_SCRIPT_KHMER;
		case VKNVG_SCRIPT_HANGUL:		return HB_SCRIPT_HANGUL;
		case VKNVG_SCRIPT_HIRAGANA:		return HB_SCRIPT_HIRAGANA;
		case VKNVG_SCRIPT_KATAKANA:		return HB_SCRIPT_KATAKANA;
		case VKNVG_SCRIPT_HAN:			return HB_SCRIPT_HAN;
		case VKNVG_SCRIPT_MYANMAR:		return HB_SCRIPT_MYANMAR;
		case VKNVG_SCRIPT_SINHALA:		return HB_SCRIPT_SINHALA;
		default:						return HB_SCRIPT_UNKNOWN;
	}
}

// Convert VKNVGtextDirection to HarfBuzz direction
static hb_direction_t vknvg__directionToHarfBuzz(VKNVGtextDirection direction)
{
	switch (direction) {
		case VKNVG_TEXT_DIRECTION_LTR:	return HB_DIRECTION_LTR;
		case VKNVG_TEXT_DIRECTION_RTL:	return HB_DIRECTION_RTL;
		case VKNVG_TEXT_DIRECTION_TTB:	return HB_DIRECTION_TTB;
		case VKNVG_TEXT_DIRECTION_BTT:	return HB_DIRECTION_BTT;
		default:						return HB_DIRECTION_INVALID;
	}
}

// Convert HarfBuzz direction to VKNVGtextDirection
static VKNVGtextDirection vknvg__hbDirectionToVKNVG(hb_direction_t hbDirection)
{
	if (hbDirection == HB_DIRECTION_LTR) return VKNVG_TEXT_DIRECTION_LTR;
	if (hbDirection == HB_DIRECTION_RTL) return VKNVG_TEXT_DIRECTION_RTL;
	if (hbDirection == HB_DIRECTION_TTB) return VKNVG_TEXT_DIRECTION_TTB;
	if (hbDirection == HB_DIRECTION_BTT) return VKNVG_TEXT_DIRECTION_BTT;
	return VKNVG_TEXT_DIRECTION_LTR;
}

// Shape text using HarfBuzz
int vknvg__shapeText(VKNVGharfbuzzContext* ctx,
                     const char* text,
                     uint32_t textLen,
                     int fontID,
                     VKNVGtextDirection direction,
                     VKNVGscript script,
                     const VKNVGlanguage* language,
                     const VKNVGfeature* features,
                     uint32_t featureCount,
                     VKNVGshapingResult* result)
{
	if (!ctx || !text || !result) return 0;

	// Find HarfBuzz font
	hb_font_t* hbFont = NULL;
	for (uint32_t i = 0; i < ctx->fontMapCount; i++) {
		if (ctx->fontMap[i].nvgFontID == fontID) {
			hbFont = ctx->fontMap[i].hbFont;
			break;
		}
	}
	if (!hbFont) return 0;

	// Clear buffer
	hb_buffer_clear_contents(ctx->buffer);

	// Add text to buffer
	hb_buffer_add_utf8(ctx->buffer, text, (int)textLen, 0, (int)textLen);

	// Set buffer properties
	if (direction == VKNVG_TEXT_DIRECTION_AUTO) {
		hb_buffer_guess_segment_properties(ctx->buffer);
	} else {
		hb_buffer_set_direction(ctx->buffer, vknvg__directionToHarfBuzz(direction));
		if (script != VKNVG_SCRIPT_AUTO) {
			hb_buffer_set_script(ctx->buffer, vknvg__scriptToHarfBuzz(script));
		}
		if (language) {
			hb_buffer_set_language(ctx->buffer, hb_language_from_string(language->tag, -1));
		}
	}

	// Convert features to HarfBuzz format
	hb_feature_t* hbFeatures = NULL;
	if (features && featureCount > 0) {
		hbFeatures = (hb_feature_t*)malloc(sizeof(hb_feature_t) * featureCount);
		for (uint32_t i = 0; i < featureCount; i++) {
			hbFeatures[i].tag = HB_TAG(features[i].tag[0], features[i].tag[1],
			                            features[i].tag[2], features[i].tag[3]);
			hbFeatures[i].value = features[i].value;
			hbFeatures[i].start = features[i].start;
			hbFeatures[i].end = features[i].end;
		}
	}

	// Shape the text
	hb_shape(hbFont, ctx->buffer, hbFeatures, featureCount);

	// Free features
	if (hbFeatures) {
		free(hbFeatures);
	}

	// Get shaped glyphs
	unsigned int glyphCount;
	hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(ctx->buffer, &glyphCount);
	hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions(ctx->buffer, &glyphCount);

	if (glyphCount > VKNVG_MAX_SHAPED_GLYPHS) {
		glyphCount = VKNVG_MAX_SHAPED_GLYPHS;
	}

	// Copy results
	result->glyphCount = glyphCount;
	for (unsigned int i = 0; i < glyphCount; i++) {
		result->glyphs[i].glyphIndex = glyphInfo[i].codepoint;
		result->glyphs[i].cluster = glyphInfo[i].cluster;
		result->glyphs[i].xOffset = glyphPos[i].x_offset;
		result->glyphs[i].yOffset = glyphPos[i].y_offset;
		result->glyphs[i].xAdvance = glyphPos[i].x_advance;
		result->glyphs[i].yAdvance = glyphPos[i].y_advance;
	}

	// Get result direction and script
	result->direction = vknvg__hbDirectionToVKNVG(hb_buffer_get_direction(ctx->buffer));
	hb_script_t hbScript = hb_buffer_get_script(ctx->buffer);
	result->script = script;	// Use input script as approximation

	// Update statistics
	ctx->shapingCalls++;
	ctx->totalGlyphsShaped += glyphCount;
	if (script >= VKNVG_SCRIPT_ARABIC && script <= VKNVG_SCRIPT_SINHALA) {
		ctx->complexScriptRuns++;
	}

	return 1;
}

// Auto-detect script from UTF-8 text
VKNVGscript vknvg__detectScript(const char* text, uint32_t textLen)
{
	if (!text || textLen == 0) return VKNVG_SCRIPT_INVALID;

	// Decode first UTF-8 character to get codepoint
	uint32_t codepoint = 0;
	uint8_t byte = (uint8_t)text[0];

	if (byte < 0x80) {
		codepoint = byte;
	} else if ((byte & 0xE0) == 0xC0 && textLen >= 2) {
		codepoint = ((byte & 0x1F) << 6) | ((uint8_t)text[1] & 0x3F);
	} else if ((byte & 0xF0) == 0xE0 && textLen >= 3) {
		codepoint = ((byte & 0x0F) << 12) |
		            (((uint8_t)text[1] & 0x3F) << 6) |
		            ((uint8_t)text[2] & 0x3F);
	} else if ((byte & 0xF8) == 0xF0 && textLen >= 4) {
		codepoint = ((byte & 0x07) << 18) |
		            (((uint8_t)text[1] & 0x3F) << 12) |
		            (((uint8_t)text[2] & 0x3F) << 6) |
		            ((uint8_t)text[3] & 0x3F);
	}

	// Detect script from codepoint range
	if (codepoint >= 0x0600 && codepoint <= 0x06FF) return VKNVG_SCRIPT_ARABIC;
	if (codepoint >= 0x0590 && codepoint <= 0x05FF) return VKNVG_SCRIPT_HEBREW;
	if (codepoint >= 0x0900 && codepoint <= 0x097F) return VKNVG_SCRIPT_DEVANAGARI;
	if (codepoint >= 0x0980 && codepoint <= 0x09FF) return VKNVG_SCRIPT_BENGALI;
	if (codepoint >= 0x0E00 && codepoint <= 0x0E7F) return VKNVG_SCRIPT_THAI;
	if (codepoint >= 0x1780 && codepoint <= 0x17FF) return VKNVG_SCRIPT_KHMER;
	if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return VKNVG_SCRIPT_HANGUL;
	if (codepoint >= 0x3040 && codepoint <= 0x309F) return VKNVG_SCRIPT_HIRAGANA;
	if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return VKNVG_SCRIPT_KATAKANA;
	if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) return VKNVG_SCRIPT_HAN;
	if (codepoint >= 0x1000 && codepoint <= 0x109F) return VKNVG_SCRIPT_MYANMAR;
	if (codepoint >= 0x0D80 && codepoint <= 0x0DFF) return VKNVG_SCRIPT_SINHALA;

	return VKNVG_SCRIPT_LATIN;
}

// Get default text direction for script
VKNVGtextDirection vknvg__getScriptDirection(VKNVGscript script)
{
	switch (script) {
		case VKNVG_SCRIPT_ARABIC:
		case VKNVG_SCRIPT_HEBREW:
			return VKNVG_TEXT_DIRECTION_RTL;
		default:
			return VKNVG_TEXT_DIRECTION_LTR;
	}
}

// Get statistics
void vknvg__getHarfBuzzStats(VKNVGharfbuzzContext* ctx,
                             uint32_t* shapingCalls,
                             uint32_t* totalGlyphs,
                             uint32_t* complexRuns)
{
	if (!ctx) return;
	if (shapingCalls) *shapingCalls = ctx->shapingCalls;
	if (totalGlyphs) *totalGlyphs = ctx->totalGlyphsShaped;
	if (complexRuns) *complexRuns = ctx->complexScriptRuns;
}

// Reset statistics
void vknvg__resetHarfBuzzStats(VKNVGharfbuzzContext* ctx)
{
	if (!ctx) return;
	ctx->shapingCalls = 0;
	ctx->totalGlyphsShaped = 0;
	ctx->complexScriptRuns = 0;
}
