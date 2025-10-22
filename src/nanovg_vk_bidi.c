// nanovg_vk_bidi.c - Unicode BiDi Algorithm Implementation
//
// Real implementation using FriBidi library for bidirectional text

#include "nanovg_vk_bidi.h"
#include <fribidi.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration
static void vknvg__buildRuns(VKNVGbidiResult* result);

// Create BiDi context
VKNVGbidiContext* vknvg__createBidiContext(void)
{
	VKNVGbidiContext* ctx = (VKNVGbidiContext*)calloc(1, sizeof(VKNVGbidiContext));
	return ctx;
}

// Destroy BiDi context
void vknvg__destroyBidiContext(VKNVGbidiContext* ctx)
{
	if (!ctx) return;
	free(ctx);
}

// Convert FriBidi char type to VKNVGbidiType
static VKNVGbidiType vknvg__fribidiTypeToVKNVG(FriBidiCharType type)
{
	switch (type) {
		case FRIBIDI_TYPE_LTR:	return VKNVG_BIDI_L;
		case FRIBIDI_TYPE_RTL:	return VKNVG_BIDI_R;
		case FRIBIDI_TYPE_AL:	return VKNVG_BIDI_AL;
		case FRIBIDI_TYPE_EN:	return VKNVG_BIDI_EN;
		case FRIBIDI_TYPE_ES:	return VKNVG_BIDI_ES;
		case FRIBIDI_TYPE_ET:	return VKNVG_BIDI_ET;
		case FRIBIDI_TYPE_AN:	return VKNVG_BIDI_AN;
		case FRIBIDI_TYPE_CS:	return VKNVG_BIDI_CS;
		case FRIBIDI_TYPE_NSM:	return VKNVG_BIDI_NSM;
		case FRIBIDI_TYPE_BN:	return VKNVG_BIDI_BN;
		case FRIBIDI_TYPE_BS:	return VKNVG_BIDI_B;
		case FRIBIDI_TYPE_SS:	return VKNVG_BIDI_S;
		case FRIBIDI_TYPE_WS:	return VKNVG_BIDI_WS;
		case FRIBIDI_TYPE_ON:	return VKNVG_BIDI_ON;
		case FRIBIDI_TYPE_LRE:	return VKNVG_BIDI_LRE;
		case FRIBIDI_TYPE_LRO:	return VKNVG_BIDI_LRO;
		case FRIBIDI_TYPE_RLE:	return VKNVG_BIDI_RLE;
		case FRIBIDI_TYPE_RLO:	return VKNVG_BIDI_RLO;
		case FRIBIDI_TYPE_PDF:	return VKNVG_BIDI_PDF;
		case FRIBIDI_TYPE_LRI:	return VKNVG_BIDI_LRI;
		case FRIBIDI_TYPE_RLI:	return VKNVG_BIDI_RLI;
		case FRIBIDI_TYPE_FSI:	return VKNVG_BIDI_FSI;
		case FRIBIDI_TYPE_PDI:	return VKNVG_BIDI_PDI;
		default:				return VKNVG_BIDI_ON;
	}
}

// Decode UTF-8 to Unicode codepoints
static uint32_t vknvg__decodeUTF8(const char* text, uint32_t textLen,
                                  uint32_t* codepoints, uint32_t maxCodepoints)
{
	uint32_t count = 0;
	uint32_t i = 0;

	while (i < textLen && count < maxCodepoints) {
		uint8_t byte = (uint8_t)text[i];
		uint32_t codepoint;

		if (byte < 0x80) {
			codepoint = byte;
			i++;
		} else if ((byte & 0xE0) == 0xC0 && i + 1 < textLen) {
			codepoint = ((byte & 0x1F) << 6) | ((uint8_t)text[i+1] & 0x3F);
			i += 2;
		} else if ((byte & 0xF0) == 0xE0 && i + 2 < textLen) {
			codepoint = ((byte & 0x0F) << 12) |
			            (((uint8_t)text[i+1] & 0x3F) << 6) |
			            ((uint8_t)text[i+2] & 0x3F);
			i += 3;
		} else if ((byte & 0xF8) == 0xF0 && i + 3 < textLen) {
			codepoint = ((byte & 0x07) << 18) |
			            (((uint8_t)text[i+1] & 0x3F) << 12) |
			            (((uint8_t)text[i+2] & 0x3F) << 6) |
			            ((uint8_t)text[i+3] & 0x3F);
			i += 4;
		} else {
			// Invalid UTF-8, skip byte
			i++;
			continue;
		}

		codepoints[count++] = codepoint;
	}

	return count;
}

// Analyze text and determine BiDi properties
int vknvg__analyzeBidi(VKNVGbidiContext* ctx,
                       const char* text,
                       uint32_t textLen,
                       VKNVGparagraphDirection baseDirection,
                       VKNVGbidiResult* result)
{
	if (!ctx || !text || !result) return 0;

	// Decode UTF-8 to codepoints
	uint32_t cpCount = vknvg__decodeUTF8(text, textLen,
	                                     ctx->codepoints,
	                                     VKNVG_MAX_BIDI_TEXT_LENGTH);
	if (cpCount == 0) return 0;

	// Get character types
	FriBidiCharType* fribidiTypes = (FriBidiCharType*)malloc(sizeof(FriBidiCharType) * cpCount);
	for (uint32_t i = 0; i < cpCount; i++) {
		fribidiTypes[i] = fribidi_get_bidi_type(ctx->codepoints[i]);
	}

	// Determine base direction
	FriBidiParType parType;
	if (baseDirection == VKNVG_PARA_LTR) {
		parType = FRIBIDI_PAR_LTR;
	} else if (baseDirection == VKNVG_PARA_RTL) {
		parType = FRIBIDI_PAR_RTL;
	} else {
		parType = FRIBIDI_PAR_ON;	// Auto-detect
	}

	// Get embedding levels
	FriBidiLevel* levels = (FriBidiLevel*)malloc(sizeof(FriBidiLevel) * cpCount);

	fribidi_get_bidi_types(ctx->codepoints, cpCount, fribidiTypes);
	FriBidiLevel maxLevel = fribidi_get_par_embedding_levels(fribidiTypes, cpCount, &parType, levels);

	// Copy results
	result->textLength = cpCount;
	result->baseLevel = FRIBIDI_LEVEL_IS_RTL(parType) ? 1 : 0;
	result->maxLevel = maxLevel;
	result->flags = 0;

	for (uint32_t i = 0; i < cpCount; i++) {
		result->levels[i] = levels[i];
		result->types[i] = vknvg__fribidiTypeToVKNVG(fribidiTypes[i]);

		// Set flags
		if (fribidiTypes[i] == FRIBIDI_TYPE_RTL || fribidiTypes[i] == FRIBIDI_TYPE_AL) {
			result->flags |= VKNVG_BIDI_FLAG_HAS_RTL;
		}
		if (fribidiTypes[i] == FRIBIDI_TYPE_AL) {
			result->flags |= VKNVG_BIDI_FLAG_HAS_ARABIC;
		}
		if (fribidiTypes[i] == FRIBIDI_TYPE_EN || fribidiTypes[i] == FRIBIDI_TYPE_AN) {
			result->flags |= VKNVG_BIDI_FLAG_HAS_NUMBERS;
		}
	}

	free(levels);
	free(fribidiTypes);

	// Update statistics
	ctx->analysisCount++;
	if (result->flags & VKNVG_BIDI_FLAG_HAS_RTL) {
		ctx->rtlTextCount++;
	}

	return 1;
}

// Reorder text for visual display
void vknvg__reorderBidi(VKNVGbidiContext* ctx, VKNVGbidiResult* result)
{
	if (!ctx || !result || result->textLength == 0) return;

	// Create simple linear logical order
	for (uint32_t i = 0; i < result->textLength; i++) {
		result->visualOrder[i] = i;
		result->logicalOrder[i] = i;
	}

	// Reverse RTL runs
	for (uint32_t i = 0; i < result->textLength; ) {
		if (vknvg__isRTL(result->levels[i])) {
			// Find end of RTL run
			uint32_t j = i + 1;
			while (j < result->textLength && result->levels[j] == result->levels[i]) {
				j++;
			}

			// Reverse this run
			for (uint32_t k = 0; k < (j - i) / 2; k++) {
				uint32_t temp = result->visualOrder[i + k];
				result->visualOrder[i + k] = result->visualOrder[j - 1 - k];
				result->visualOrder[j - 1 - k] = temp;
			}

			i = j;
		} else {
			i++;
		}
	}

	// Update logical to visual mapping
	for (uint32_t i = 0; i < result->textLength; i++) {
		result->logicalOrder[result->visualOrder[i]] = i;
	}

	// Build runs
	vknvg__buildRuns(result);

	ctx->reorderingCount++;
}

// Get character type for Unicode codepoint
VKNVGbidiType vknvg__getBidiType(uint32_t codepoint)
{
	FriBidiCharType type = fribidi_get_bidi_type(codepoint);
	return vknvg__fribidiTypeToVKNVG(type);
}

// Check if character should be mirrored in RTL context
uint32_t vknvg__getMirroredChar(uint32_t codepoint)
{
	FriBidiChar mirrored;
	if (fribidi_get_mirror_char(codepoint, &mirrored)) {
		return mirrored;
	}
	return codepoint;
}

// Detect paragraph base direction from text
VKNVGparagraphDirection vknvg__detectBaseDirection(const char* text, uint32_t textLen)
{
	if (!text || textLen == 0) return VKNVG_PARA_LTR;

	// Decode first few characters to determine direction
	uint32_t codepoints[32];
	uint32_t cpCount = vknvg__decodeUTF8(text, textLen < 128 ? textLen : 128,
	                                     codepoints, 32);

	// Find first strong character
	for (uint32_t i = 0; i < cpCount; i++) {
		FriBidiCharType type = fribidi_get_bidi_type(codepoints[i]);
		if (type == FRIBIDI_TYPE_RTL || type == FRIBIDI_TYPE_AL) {
			return VKNVG_PARA_RTL;
		}
		if (type == FRIBIDI_TYPE_LTR) {
			return VKNVG_PARA_LTR;
		}
	}

	return VKNVG_PARA_LTR;
}

// Build run list from levels
static void vknvg__buildRuns(VKNVGbidiResult* result)
{
	if (!result || result->textLength == 0) return;

	result->runCount = 0;
	uint32_t runStart = 0;
	uint8_t currentLevel = result->levels[0];

	for (uint32_t i = 1; i <= result->textLength; i++) {
		if (i == result->textLength || result->levels[i] != currentLevel) {
			// End of run
			if (result->runCount < 256) {
				result->runs[result->runCount].start = runStart;
				result->runs[result->runCount].length = i - runStart;
				result->runs[result->runCount].level = currentLevel;
				result->runs[result->runCount].type = result->types[runStart];
				result->runCount++;
			}

			if (i < result->textLength) {
				runStart = i;
				currentLevel = result->levels[i];
			}
		}
	}
}

// Get statistics
void vknvg__getBidiStats(VKNVGbidiContext* ctx,
                         uint32_t* analysisCount,
                         uint32_t* reorderingCount,
                         uint32_t* rtlTextCount)
{
	if (!ctx) return;
	if (analysisCount) *analysisCount = ctx->analysisCount;
	if (reorderingCount) *reorderingCount = ctx->reorderingCount;
	if (rtlTextCount) *rtlTextCount = ctx->rtlTextCount;
}

// Reset statistics
void vknvg__resetBidiStats(VKNVGbidiContext* ctx)
{
	if (!ctx) return;
	ctx->analysisCount = 0;
	ctx->reorderingCount = 0;
	ctx->rtlTextCount = 0;
}
