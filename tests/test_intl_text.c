#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/nanovg_vk_intl_text.h"

// Test 1: Text run structure
static int test_text_run_structure(void)
{
	printf("Test: Text run structure\n");

	VKNVGtextRun run = {0};
	run.textStart = 0;
	run.textLength = 20;
	run.charStart = 0;
	run.charLength = 15;
	run.script = VKNVG_SCRIPT_LATIN;
	run.direction = VKNVG_TEXT_DIRECTION_LTR;
	run.bidiLevel = 0;
	run.fontID = 1;

	// Simulate shaped glyphs
	for (uint32_t i = 0; i < 15 && i < VKNVG_MAX_GLYPHS_PER_RUN; i++) {
		run.glyphs[i].glyphIndex = i + 10;
		run.glyphs[i].xAdvance = 640;	// 10 pixels (26.6 fixed point)
		run.glyphCount++;
	}

	run.width = 150.0f;
	run.height = 20.0f;
	run.ascent = 16.0f;
	run.descent = 4.0f;

	printf("  Run: text=[%u, %u), chars=[%u, %u)\n",
	       run.textStart, run.textStart + run.textLength,
	       run.charStart, run.charStart + run.charLength);
	printf("  Script: %d, Direction: %d, BiDi level: %u\n",
	       run.script, run.direction, run.bidiLevel);
	printf("  Glyphs: %u\n", run.glyphCount);
	printf("  Size: %.1fx%.1f, Ascent: %.1f, Descent: %.1f\n",
	       run.width, run.height, run.ascent, run.descent);

	assert(run.charLength == 15);
	assert(run.glyphCount == 15);
	assert(run.width == 150.0f);

	printf("  Size of VKNVGtextRun: %zu bytes\n", sizeof(VKNVGtextRun));

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 2: Shaped paragraph structure
static int test_shaped_paragraph(void)
{
	printf("Test: Shaped paragraph structure\n");

	VKNVGshapedParagraph para = {0};
	para.text = "Hello World";
	para.textLength = 11;
	para.baseDirection = VKNVG_PARA_LTR;

	// Create single run
	para.runs[0].textStart = 0;
	para.runs[0].textLength = 11;
	para.runs[0].charLength = 11;
	para.runs[0].glyphCount = 11;
	para.runs[0].width = 100.0f;
	para.runCount = 1;

	para.visualRunOrder[0] = 0;	// Only one run

	para.totalWidth = 100.0f;
	para.totalHeight = 20.0f;
	para.maxAscent = 16.0f;
	para.maxDescent = 4.0f;

	printf("  Text: \"%s\" (%u bytes)\n", para.text, para.textLength);
	printf("  Base direction: %d (LTR=%d)\n",
	       para.baseDirection, VKNVG_PARA_LTR);
	printf("  Runs: %u\n", para.runCount);
	printf("  Total size: %.1fx%.1f\n", para.totalWidth, para.totalHeight);

	assert(para.runCount == 1);
	assert(para.totalWidth == 100.0f);
	assert(strcmp(para.text, "Hello World") == 0);

	printf("  Size of VKNVGshapedParagraph: %zu bytes\n",
	       sizeof(VKNVGshapedParagraph));

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 3: Mixed LTR/RTL paragraph
static int test_mixed_direction_paragraph(void)
{
	printf("Test: Mixed LTR/RTL paragraph\n");

	VKNVGshapedParagraph para = {0};
	para.text = "Hello مرحبا World";	// English + Arabic + English
	para.textLength = strlen(para.text);
	para.baseDirection = VKNVG_PARA_LTR;

	// Run 1: "Hello " (LTR)
	para.runs[0].textStart = 0;
	para.runs[0].textLength = 6;
	para.runs[0].script = VKNVG_SCRIPT_LATIN;
	para.runs[0].direction = VKNVG_TEXT_DIRECTION_LTR;
	para.runs[0].bidiLevel = 0;

	// Run 2: "مرحبا" (RTL, Arabic)
	para.runs[1].textStart = 6;
	para.runs[1].textLength = 10;	// Arabic is multi-byte UTF-8
	para.runs[1].script = VKNVG_SCRIPT_ARABIC;
	para.runs[1].direction = VKNVG_TEXT_DIRECTION_RTL;
	para.runs[1].bidiLevel = 1;

	// Run 3: " World" (LTR)
	para.runs[2].textStart = 16;
	para.runs[2].textLength = para.textLength - 16;
	para.runs[2].script = VKNVG_SCRIPT_LATIN;
	para.runs[2].direction = VKNVG_TEXT_DIRECTION_LTR;
	para.runs[2].bidiLevel = 0;

	para.runCount = 3;

	// Visual order: [0, 1, 2] (no reordering, Arabic run is already visually correct)
	para.visualRunOrder[0] = 0;
	para.visualRunOrder[1] = 1;
	para.visualRunOrder[2] = 2;

	para.flags = VKNVG_PARA_FLAG_HAS_RTL | VKNVG_PARA_FLAG_NEEDS_BIDI;

	printf("  Text: \"%s\"\n", para.text);
	printf("  Runs: %u\n", para.runCount);
	printf("  Run 0: Latin LTR (level %u)\n", para.runs[0].bidiLevel);
	printf("  Run 1: Arabic RTL (level %u)\n", para.runs[1].bidiLevel);
	printf("  Run 2: Latin LTR (level %u)\n", para.runs[2].bidiLevel);
	printf("  Flags: 0x%X (HAS_RTL=0x%X, NEEDS_BIDI=0x%X)\n",
	       para.flags, VKNVG_PARA_FLAG_HAS_RTL, VKNVG_PARA_FLAG_NEEDS_BIDI);

	assert(para.runCount == 3);
	assert(para.runs[1].direction == VKNVG_TEXT_DIRECTION_RTL);
	assert(para.flags & VKNVG_PARA_FLAG_HAS_RTL);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 4: Complex script detection
static int test_complex_script_detection(void)
{
	printf("Test: Complex script detection\n");

	bool isComplex;

	// Latin (not complex)
	isComplex = vknvg__isComplexScript(VKNVG_SCRIPT_LATIN);
	printf("  Latin is complex: %s (expected false)\n",
	       isComplex ? "true" : "false");
	assert(!isComplex);

	// Arabic (complex)
	isComplex = vknvg__isComplexScript(VKNVG_SCRIPT_ARABIC);
	printf("  Arabic is complex: %s (expected true)\n",
	       isComplex ? "true" : "false");
	assert(isComplex);

	// Devanagari (complex)
	isComplex = vknvg__isComplexScript(VKNVG_SCRIPT_DEVANAGARI);
	printf("  Devanagari is complex: %s (expected true)\n",
	       isComplex ? "true" : "false");
	assert(isComplex);

	// Thai (complex)
	isComplex = vknvg__isComplexScript(VKNVG_SCRIPT_THAI);
	printf("  Thai is complex: %s (expected true)\n",
	       isComplex ? "true" : "false");
	assert(isComplex);

	// CJK (not complex in shaping sense)
	isComplex = vknvg__isComplexScript(VKNVG_SCRIPT_HAN);
	printf("  CJK/Han is complex: %s (expected false)\n",
	       isComplex ? "true" : "false");
	assert(!isComplex);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 5: RTL script detection
static int test_rtl_script_detection(void)
{
	printf("Test: RTL script detection\n");

	bool isRTL;

	// Latin (LTR)
	isRTL = vknvg__isRTLScript(VKNVG_SCRIPT_LATIN);
	printf("  Latin is RTL: %s (expected false)\n",
	       isRTL ? "true" : "false");
	assert(!isRTL);

	// Arabic (RTL)
	isRTL = vknvg__isRTLScript(VKNVG_SCRIPT_ARABIC);
	printf("  Arabic is RTL: %s (expected true)\n",
	       isRTL ? "true" : "false");
	assert(isRTL);

	// Hebrew (RTL)
	isRTL = vknvg__isRTLScript(VKNVG_SCRIPT_HEBREW);
	printf("  Hebrew is RTL: %s (expected true)\n",
	       isRTL ? "true" : "false");
	assert(isRTL);

	// Devanagari (LTR)
	isRTL = vknvg__isRTLScript(VKNVG_SCRIPT_DEVANAGARI);
	printf("  Devanagari is RTL: %s (expected false)\n",
	       isRTL ? "true" : "false");
	assert(!isRTL);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 6: Cursor position structure
static int test_cursor_position(void)
{
	printf("Test: Cursor position structure\n");

	VKNVGcursorPosition cursor = {0};
	cursor.byteOffset = 10;
	cursor.charOffset = 8;
	cursor.runIndex = 1;
	cursor.glyphIndex = 5;
	cursor.x = 42.5f;
	cursor.y = 10.0f;
	cursor.isTrailing = false;

	printf("  Cursor: byte=%u, char=%u, run=%u, glyph=%u\n",
	       cursor.byteOffset, cursor.charOffset,
	       cursor.runIndex, cursor.glyphIndex);
	printf("  Position: (%.1f, %.1f), trailing=%s\n",
	       cursor.x, cursor.y, cursor.isTrailing ? "true" : "false");

	assert(cursor.byteOffset == 10);
	assert(cursor.charOffset == 8);
	assert(cursor.x == 42.5f);

	printf("  Size of VKNVGcursorPosition: %zu bytes\n",
	       sizeof(VKNVGcursorPosition));

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 7: Fixed-point conversion helpers
static int test_fixed_point_conversion(void)
{
	printf("Test: Fixed-point conversion (26.6)\n");

	// Float to fixed
	float value = 10.5f;
	int32_t fixed = vknvg__toFixed26_6(value);
	printf("  %.2f → %d (26.6 fixed point)\n", value, fixed);
	assert(fixed == 672);	// 10.5 * 64 = 672

	// Fixed to float
	fixed = 640;
	float result = vknvg__fromFixed26_6(fixed);
	printf("  %d → %.2f (float)\n", fixed, result);
	assert(result == 10.0f);	// 640 / 64 = 10.0

	// Round-trip
	value = 42.25f;
	fixed = vknvg__toFixed26_6(value);
	result = vknvg__fromFixed26_6(fixed);
	printf("  Round-trip: %.2f → %d → %.2f\n", value, fixed, result);
	assert(result == value);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 8: International text context structure
static int test_intl_text_context(void)
{
	printf("Test: International text context structure\n");

	VKNVGintlTextContext ctx = {0};
	ctx.enableBidi = true;
	ctx.enableShaping = true;
	ctx.enableComplexScripts = true;

	// Add default features
	ctx.defaultFeatures[0].tag[0] = 'l';
	ctx.defaultFeatures[0].tag[1] = 'i';
	ctx.defaultFeatures[0].tag[2] = 'g';
	ctx.defaultFeatures[0].tag[3] = 'a';
	ctx.defaultFeatures[0].value = 1;
	ctx.defaultFeatureCount = 1;

	printf("  BiDi enabled: %s\n", ctx.enableBidi ? "true" : "false");
	printf("  Shaping enabled: %s\n", ctx.enableShaping ? "true" : "false");
	printf("  Complex scripts enabled: %s\n",
	       ctx.enableComplexScripts ? "true" : "false");
	printf("  Default features: %u\n", ctx.defaultFeatureCount);

	assert(ctx.enableBidi);
	assert(ctx.enableShaping);
	assert(ctx.defaultFeatureCount == 1);

	printf("  Size of VKNVGintlTextContext: %zu bytes\n",
	       sizeof(VKNVGintlTextContext));

	printf("  ✓ PASS\n\n");
	return 1;
}

int main(void)
{
	printf("==========================================\n");
	printf("  International Text Tests\n");
	printf("==========================================\n\n");

	test_text_run_structure();
	test_shaped_paragraph();
	test_mixed_direction_paragraph();
	test_complex_script_detection();
	test_rtl_script_detection();
	test_cursor_position();
	test_fixed_point_conversion();
	test_intl_text_context();

	printf("========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   8\n");
	printf("  Passed:  8\n");
	printf("========================================\n");
	printf("All international text tests passed!\n");

	return 0;
}
