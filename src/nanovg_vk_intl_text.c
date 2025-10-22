// nanovg_vk_intl_text.c - International Text Rendering Integration
//
// Combines HarfBuzz shaping and BiDi reordering

#include "nanovg_vk_intl_text.h"
#include <stdlib.h>
#include <string.h>

// Create international text context
VKNVGintlTextContext* vknvg__createIntlTextContext(void)
{
	VKNVGintlTextContext* ctx = (VKNVGintlTextContext*)calloc(1, sizeof(VKNVGintlTextContext));
	if (!ctx) return NULL;

	// Create sub-contexts
	ctx->harfbuzz = vknvg__createHarfBuzzContext();
	ctx->bidi = vknvg__createBidiContext();

	if (!ctx->harfbuzz || !ctx->bidi) {
		vknvg__destroyIntlTextContext(ctx);
		return NULL;
	}

	// Enable all features by default
	ctx->enableBidi = true;
	ctx->enableShaping = true;
	ctx->enableComplexScripts = true;

	// Set common default features
	ctx->defaultFeatureCount = 2;
	memcpy(ctx->defaultFeatures[0].tag, "liga", 4);
	ctx->defaultFeatures[0].value = 1;
	ctx->defaultFeatures[0].start = 0;
	ctx->defaultFeatures[0].end = (uint32_t)-1;

	memcpy(ctx->defaultFeatures[1].tag, "kern", 4);
	ctx->defaultFeatures[1].value = 1;
	ctx->defaultFeatures[1].start = 0;
	ctx->defaultFeatures[1].end = (uint32_t)-1;

	return ctx;
}

// Destroy international text context
void vknvg__destroyIntlTextContext(VKNVGintlTextContext* ctx)
{
	if (!ctx) return;

	if (ctx->harfbuzz) {
		vknvg__destroyHarfBuzzContext(ctx->harfbuzz);
	}
	if (ctx->bidi) {
		vknvg__destroyBidiContext(ctx->bidi);
	}

	free(ctx);
}

// Register font for international text rendering
int vknvg__registerIntlFont(VKNVGintlTextContext* ctx,
                            int nvgFontID,
                            FT_Face ftFace)
{
	if (!ctx || !ctx->harfbuzz) return -1;
	return vknvg__registerHarfBuzzFont(ctx->harfbuzz, nvgFontID, ftFace);
}

// Unregister font
void vknvg__unregisterIntlFont(VKNVGintlTextContext* ctx, int nvgFontID)
{
	if (!ctx || !ctx->harfbuzz) return;
	vknvg__unregisterHarfBuzzFont(ctx->harfbuzz, nvgFontID);
}

// Segment text into runs (by script, direction, font)
static uint32_t vknvg__segmentText(VKNVGintlTextContext* ctx,
                                   const char* text,
                                   uint32_t textLen,
                                   const VKNVGbidiResult* bidiResult,
                                   int fontID,
                                   VKNVGtextRun* runs)
{
	// Simple implementation: one run per BiDi run for now
	// More sophisticated implementation would also segment by script changes

	uint32_t runCount = 0;
	uint32_t textPos = 0;

	for (uint32_t i = 0; i < bidiResult->runCount && runCount < VKNVG_MAX_TEXT_RUNS; i++) {
		VKNVGbidiRun* bidiRun = (VKNVGbidiRun*)&bidiResult->runs[i];

		runs[runCount].textStart = textPos;
		runs[runCount].textLength = bidiRun->length;	// Approximate
		runs[runCount].charStart = bidiRun->start;
		runs[runCount].charLength = bidiRun->length;
		runs[runCount].script = VKNVG_SCRIPT_AUTO;	// Will be detected by HarfBuzz
		runs[runCount].direction = vknvg__isRTL(bidiRun->level) ?
		                           VKNVG_TEXT_DIRECTION_RTL : VKNVG_TEXT_DIRECTION_LTR;
		runs[runCount].bidiLevel = bidiRun->level;
		runs[runCount].fontID = fontID;
		runs[runCount].glyphCount = 0;

		textPos += bidiRun->length;
		runCount++;
	}

	return runCount;
}

// Shape single run with HarfBuzz
static int vknvg__shapeRun(VKNVGintlTextContext* ctx,
                           const char* text,
                           VKNVGtextRun* run,
                           float fontSize)
{
	if (!ctx || !ctx->harfbuzz || !run) return 0;

	VKNVGshapingResult shapingResult = {0};

	// Shape the run's text segment
	int success = vknvg__shapeText(ctx->harfbuzz,
	                                text + run->textStart,
	                                run->textLength,
	                                run->fontID,
	                                run->direction,
	                                run->script,
	                                NULL,	// Language (auto-detect)
	                                ctx->defaultFeatures,
	                                ctx->defaultFeatureCount,
	                                &shapingResult);

	if (!success) return 0;

	// Copy shaped glyphs to run
	uint32_t glyphCount = shapingResult.glyphCount;
	if (glyphCount > VKNVG_MAX_GLYPHS_PER_RUN) {
		glyphCount = VKNVG_MAX_GLYPHS_PER_RUN;
	}

	for (uint32_t i = 0; i < glyphCount; i++) {
		run->glyphs[i] = shapingResult.glyphs[i];
	}
	run->glyphCount = glyphCount;

	// Update run's detected script
	run->script = shapingResult.script;

	// Calculate metrics
	vknvg__calculateRunMetrics(run, fontSize);

	return 1;
}

// Calculate run metrics
static void vknvg__calculateRunMetrics(VKNVGtextRun* run, float fontSize)
{
	if (!run || run->glyphCount == 0) return;

	// Calculate width from glyph advances
	float width = 0.0f;
	for (uint32_t i = 0; i < run->glyphCount; i++) {
		width += vknvg__fromFixed26_6(run->glyphs[i].xAdvance);
	}

	run->width = width;
	run->height = fontSize;
	run->ascent = fontSize * 0.8f;		// Approximate
	run->descent = fontSize * 0.2f;		// Approximate
}

// Calculate paragraph metrics
static void vknvg__calculateParagraphMetrics(VKNVGshapedParagraph* paragraph)
{
	if (!paragraph || paragraph->runCount == 0) return;

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float maxAscent = 0.0f;
	float maxDescent = 0.0f;

	for (uint32_t i = 0; i < paragraph->runCount; i++) {
		VKNVGtextRun* run = &paragraph->runs[i];
		totalWidth += run->width;
		if (run->height > maxHeight) maxHeight = run->height;
		if (run->ascent > maxAscent) maxAscent = run->ascent;
		if (run->descent > maxDescent) maxDescent = run->descent;
	}

	paragraph->totalWidth = totalWidth;
	paragraph->totalHeight = maxHeight;
	paragraph->maxAscent = maxAscent;
	paragraph->maxDescent = maxDescent;
}

// Reorder runs for visual display
static void vknvg__reorderRuns(VKNVGshapedParagraph* paragraph)
{
	if (!paragraph || paragraph->runCount == 0) return;

	// Simple visual ordering based on BiDi levels
	// For more sophisticated layout, would need to consider line breaking

	for (uint32_t i = 0; i < paragraph->runCount; i++) {
		paragraph->visualRunOrder[i] = i;
	}

	// If base direction is RTL, reverse run order
	if (paragraph->baseDirection == VKNVG_PARA_RTL) {
		for (uint32_t i = 0; i < paragraph->runCount / 2; i++) {
			uint32_t temp = paragraph->visualRunOrder[i];
			paragraph->visualRunOrder[i] = paragraph->visualRunOrder[paragraph->runCount - 1 - i];
			paragraph->visualRunOrder[paragraph->runCount - 1 - i] = temp;
		}
	}
}

// Shape paragraph (full analysis + shaping + reordering)
int vknvg__shapeParagraph(VKNVGintlTextContext* ctx,
                          const char* text,
                          uint32_t textLen,
                          int fontID,
                          VKNVGparagraphDirection baseDirection,
                          float fontSize,
                          VKNVGshapedParagraph* paragraph)
{
	if (!ctx || !text || !paragraph) return 0;

	memset(paragraph, 0, sizeof(VKNVGshapedParagraph));

	paragraph->text = (char*)text;	// Not owned
	paragraph->textLength = textLen;
	paragraph->baseDirection = baseDirection;

	// BiDi analysis
	if (ctx->enableBidi) {
		if (!vknvg__analyzeBidi(ctx->bidi, text, textLen, baseDirection, &paragraph->bidiResult)) {
			return 0;
		}

		// Reorder for visual display
		vknvg__reorderBidi(ctx->bidi, &paragraph->bidiResult);

		// Set paragraph flags
		paragraph->flags = paragraph->bidiResult.flags;
	} else {
		// No BiDi processing - treat as single LTR run
		paragraph->bidiResult.textLength = textLen;
		paragraph->bidiResult.baseLevel = 0;
		paragraph->bidiResult.runCount = 1;
		paragraph->bidiResult.runs[0].start = 0;
		paragraph->bidiResult.runs[0].length = textLen;
		paragraph->bidiResult.runs[0].level = 0;
	}

	// Segment into text runs
	paragraph->runCount = vknvg__segmentText(ctx, text, textLen,
	                                          &paragraph->bidiResult,
	                                          fontID,
	                                          paragraph->runs);

	// Shape each run
	if (ctx->enableShaping) {
		for (uint32_t i = 0; i < paragraph->runCount; i++) {
			vknvg__shapeRun(ctx, text, &paragraph->runs[i], fontSize);

			// Update flags
			if (vknvg__isComplexScript(paragraph->runs[i].script)) {
				paragraph->flags |= VKNVG_PARA_FLAG_HAS_COMPLEX;
			}
		}
	}

	// Reorder runs for visual display
	vknvg__reorderRuns(paragraph);

	// Calculate metrics
	vknvg__calculateParagraphMetrics(paragraph);

	// Update statistics
	ctx->paragraphsProcessed++;
	ctx->runsProcessed += paragraph->runCount;
	for (uint32_t i = 0; i < paragraph->runCount; i++) {
		ctx->glyphsShaped += paragraph->runs[i].glyphCount;
	}
	if (paragraph->flags & VKNVG_PARA_FLAG_HAS_RTL) {
		ctx->bidiReorderings++;
	}

	return 1;
}

// Free shaped paragraph resources
void vknvg__freeParagraph(VKNVGshapedParagraph* paragraph)
{
	// Currently no dynamic allocations in paragraph
	// Just clear it
	if (paragraph) {
		memset(paragraph, 0, sizeof(VKNVGshapedParagraph));
	}
}

// Set default OpenType features
void vknvg__setDefaultFeatures(VKNVGintlTextContext* ctx,
                               const VKNVGfeature* features,
                               uint32_t featureCount)
{
	if (!ctx || !features) return;

	if (featureCount > VKNVG_MAX_FEATURES) {
		featureCount = VKNVG_MAX_FEATURES;
	}

	for (uint32_t i = 0; i < featureCount; i++) {
		ctx->defaultFeatures[i] = features[i];
	}
	ctx->defaultFeatureCount = featureCount;
}

// Enable/disable BiDi processing
void vknvg__setBidiEnabled(VKNVGintlTextContext* ctx, bool enabled)
{
	if (ctx) ctx->enableBidi = enabled;
}

// Enable/disable complex script shaping
void vknvg__setComplexScriptsEnabled(VKNVGintlTextContext* ctx, bool enabled)
{
	if (ctx) ctx->enableComplexScripts = enabled;
}

// Get statistics
void vknvg__getIntlTextStats(VKNVGintlTextContext* ctx,
                             uint32_t* paragraphs,
                             uint32_t* runs,
                             uint32_t* glyphs,
                             uint32_t* bidiReorderings)
{
	if (!ctx) return;
	if (paragraphs) *paragraphs = ctx->paragraphsProcessed;
	if (runs) *runs = ctx->runsProcessed;
	if (glyphs) *glyphs = ctx->glyphsShaped;
	if (bidiReorderings) *bidiReorderings = ctx->bidiReorderings;
}

// Reset statistics
void vknvg__resetIntlTextStats(VKNVGintlTextContext* ctx)
{
	if (!ctx) return;
	ctx->paragraphsProcessed = 0;
	ctx->runsProcessed = 0;
	ctx->glyphsShaped = 0;
	ctx->bidiReorderings = 0;
}

// Get cursor position from pixel coordinates (stub for now)
int vknvg__getCursorPosition(const VKNVGshapedParagraph* paragraph,
                             float x, float y,
                             VKNVGcursorPosition* cursor)
{
	if (!paragraph || !cursor) return 0;
	// TODO: Implement hit testing
	return 0;
}

// Get pixel position from character offset (stub for now)
int vknvg__getCharacterPosition(const VKNVGshapedParagraph* paragraph,
                                uint32_t charOffset,
                                VKNVGcursorPosition* cursor)
{
	if (!paragraph || !cursor) return 0;
	// TODO: Implement caret positioning
	return 0;
}
