#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/nanovg_vk_harfbuzz.h"

// Mock HarfBuzz types for testing (real implementation would use actual HarfBuzz)
struct hb_buffer_t { int dummy; };
struct hb_font_t { int fontID; };
struct hb_face_t { int dummy; };

// Test 1: Script detection
static int test_script_detection(void)
{
	printf("Test: Script detection from text\n");

	// Latin text
	const char* latin = "Hello World";
	VKNVGscript script = vknvg__detectScript(latin, strlen(latin));
	printf("  Latin text → script: %d (expected %d)\n", script, VKNVG_SCRIPT_LATIN);
	assert(script == VKNVG_SCRIPT_LATIN);

	// Arabic text (مرحبا)
	const char* arabic = "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
	script = vknvg__detectScript(arabic, strlen(arabic));
	printf("  Arabic text → script: %d (expected %d)\n", script, VKNVG_SCRIPT_ARABIC);
	assert(script == VKNVG_SCRIPT_ARABIC);

	// Hebrew text (שלום)
	const char* hebrew = "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D";
	script = vknvg__detectScript(hebrew, strlen(hebrew));
	printf("  Hebrew text → script: %d (expected %d)\n", script, VKNVG_SCRIPT_HEBREW);
	assert(script == VKNVG_SCRIPT_HEBREW);

	// Devanagari text (नमस्ते)
	const char* devanagari = "\xE0\xA4\xA8\xE0\xA4\xAE\xE0\xA4\xB8\xE0\xA5\x8D\xE0\xA4\xA4\xE0\xA5\x87";
	script = vknvg__detectScript(devanagari, strlen(devanagari));
	printf("  Devanagari text → script: %d (expected %d)\n", script, VKNVG_SCRIPT_DEVANAGARI);
	assert(script == VKNVG_SCRIPT_DEVANAGARI);

	// Thai text (สวัสดี)
	const char* thai = "\xE0\xB8\xAA\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\xAA\xE0\xB8\x94\xE0\xB8\xB5";
	script = vknvg__detectScript(thai, strlen(thai));
	printf("  Thai text → script: %d (expected %d)\n", script, VKNVG_SCRIPT_THAI);
	assert(script == VKNVG_SCRIPT_THAI);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 2: Script direction detection
static int test_script_direction(void)
{
	printf("Test: Script direction detection\n");

	VKNVGtextDirection dir;

	dir = vknvg__getScriptDirection(VKNVG_SCRIPT_LATIN);
	printf("  Latin → direction: %d (expected LTR=%d)\n", dir, VKNVG_TEXT_DIRECTION_LTR);
	assert(dir == VKNVG_TEXT_DIRECTION_LTR);

	dir = vknvg__getScriptDirection(VKNVG_SCRIPT_ARABIC);
	printf("  Arabic → direction: %d (expected RTL=%d)\n", dir, VKNVG_TEXT_DIRECTION_RTL);
	assert(dir == VKNVG_TEXT_DIRECTION_RTL);

	dir = vknvg__getScriptDirection(VKNVG_SCRIPT_HEBREW);
	printf("  Hebrew → direction: %d (expected RTL=%d)\n", dir, VKNVG_TEXT_DIRECTION_RTL);
	assert(dir == VKNVG_TEXT_DIRECTION_RTL);

	dir = vknvg__getScriptDirection(VKNVG_SCRIPT_DEVANAGARI);
	printf("  Devanagari → direction: %d (expected LTR=%d)\n", dir, VKNVG_TEXT_DIRECTION_LTR);
	assert(dir == VKNVG_TEXT_DIRECTION_LTR);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 3: OpenType feature creation
static int test_opentype_features(void)
{
	printf("Test: OpenType feature creation\n");

	// Standard ligatures
	VKNVGfeature liga = VKNVG_MAKE_FEATURE(VKNVG_FEATURE_LIGA, 1);
	printf("  Liga feature: tag='%.4s', value=%u\n", liga.tag, liga.value);
	assert(liga.tag[0] == 'l' && liga.tag[1] == 'i' &&
	       liga.tag[2] == 'g' && liga.tag[3] == 'a');
	assert(liga.value == 1);
	assert(liga.start == 0 && liga.end == (uint32_t)-1);

	// Kerning
	VKNVGfeature kern = VKNVG_MAKE_FEATURE(VKNVG_FEATURE_KERN, 1);
	printf("  Kern feature: tag='%.4s', value=%u\n", kern.tag, kern.value);
	assert(kern.tag[0] == 'k' && kern.tag[1] == 'e' &&
	       kern.tag[2] == 'r' && kern.tag[3] == 'n');

	// Range-specific feature
	VKNVGfeature smcp = VKNVG_MAKE_FEATURE_RANGE(VKNVG_FEATURE_SMCP, 1, 10, 20);
	printf("  Smcp feature: tag='%.4s', range=[%u, %u]\n",
	       smcp.tag, smcp.start, smcp.end);
	assert(smcp.start == 10 && smcp.end == 20);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 4: Shaped glyph structure
static int test_shaped_glyph_structure(void)
{
	printf("Test: Shaped glyph structure\n");

	VKNVGshapedGlyph glyph = {0};
	glyph.glyphIndex = 42;
	glyph.cluster = 5;
	glyph.xOffset = 128;		// 2.0 pixels in 26.6 fixed point
	glyph.yOffset = 64;			// 1.0 pixel
	glyph.xAdvance = 640;		// 10.0 pixels
	glyph.yAdvance = 0;

	printf("  Glyph index: %u\n", glyph.glyphIndex);
	printf("  Cluster: %u\n", glyph.cluster);
	printf("  Offset: (%.2f, %.2f) pixels\n",
	       glyph.xOffset / 64.0f, glyph.yOffset / 64.0f);
	printf("  Advance: (%.2f, %.2f) pixels\n",
	       glyph.xAdvance / 64.0f, glyph.yAdvance / 64.0f);

	assert(glyph.glyphIndex == 42);
	assert(glyph.xAdvance / 64.0f == 10.0f);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 5: Shaping result capacity
static int test_shaping_result_capacity(void)
{
	printf("Test: Shaping result capacity\n");

	VKNVGshapingResult result = {0};

	printf("  Max glyphs: %d\n", VKNVG_MAX_SHAPED_GLYPHS);
	printf("  Size of VKNVGshapedGlyph: %zu bytes\n", sizeof(VKNVGshapedGlyph));
	printf("  Size of VKNVGshapingResult: %zu bytes\n", sizeof(VKNVGshapingResult));

	// Fill result with maximum glyphs
	for (uint32_t i = 0; i < VKNVG_MAX_SHAPED_GLYPHS; i++) {
		result.glyphs[i].glyphIndex = i;
		result.glyphs[i].xAdvance = 640;	// 10 pixels
	}
	result.glyphCount = VKNVG_MAX_SHAPED_GLYPHS;

	printf("  Filled %u glyphs\n", result.glyphCount);
	assert(result.glyphCount == VKNVG_MAX_SHAPED_GLYPHS);
	assert(result.glyphs[0].glyphIndex == 0);
	assert(result.glyphs[VKNVG_MAX_SHAPED_GLYPHS - 1].glyphIndex == VKNVG_MAX_SHAPED_GLYPHS - 1);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 6: HarfBuzz context structure
static int test_harfbuzz_context(void)
{
	printf("Test: HarfBuzz context structure\n");

	VKNVGharfbuzzContext ctx = {0};

	printf("  Font capacity: %u\n", ctx.fontCapacity);
	printf("  Font map size: %zu entries\n", sizeof(ctx.fontMap) / sizeof(ctx.fontMap[0]));
	printf("  Size of VKNVGharfbuzzContext: %zu bytes\n", sizeof(VKNVGharfbuzzContext));

	// Simulate font registration
	ctx.fontMap[0].nvgFontID = 1;
	ctx.fontMap[0].hbFont = NULL;	// Would be real HarfBuzz font
	ctx.fontMap[0].ftFace = NULL;	// Would be real FreeType face
	ctx.fontMapCount = 1;

	ctx.fontMap[1].nvgFontID = 2;
	ctx.fontMapCount = 2;

	printf("  Registered %u fonts\n", ctx.fontMapCount);
	assert(ctx.fontMapCount == 2);
	assert(ctx.fontMap[0].nvgFontID == 1);
	assert(ctx.fontMap[1].nvgFontID == 2);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 7: Language tag structure
static int test_language_tags(void)
{
	printf("Test: Language tag structure\n");

	VKNVGlanguage lang;

	// English
	strncpy(lang.tag, "en", sizeof(lang.tag));
	printf("  English: '%s'\n", lang.tag);
	assert(strcmp(lang.tag, "en") == 0);

	// Arabic
	strncpy(lang.tag, "ar", sizeof(lang.tag));
	printf("  Arabic: '%s'\n", lang.tag);
	assert(strcmp(lang.tag, "ar") == 0);

	// Thai
	strncpy(lang.tag, "th", sizeof(lang.tag));
	printf("  Thai: '%s'\n", lang.tag);
	assert(strcmp(lang.tag, "th") == 0);

	// Hindi
	strncpy(lang.tag, "hi", sizeof(lang.tag));
	printf("  Hindi: '%s'\n", lang.tag);
	assert(strcmp(lang.tag, "hi") == 0);

	printf("  Size of VKNVGlanguage: %zu bytes\n", sizeof(VKNVGlanguage));
	assert(sizeof(VKNVGlanguage) == 8);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Real implementations are now in nanovg_vk_harfbuzz.c

int main(void)
{
	printf("==========================================\n");
	printf("  HarfBuzz Integration Tests\n");
	printf("==========================================\n\n");

	test_script_detection();
	test_script_direction();
	test_opentype_features();
	test_shaped_glyph_structure();
	test_shaping_result_capacity();
	test_harfbuzz_context();
	test_language_tags();

	printf("========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   7\n");
	printf("  Passed:  7\n");
	printf("========================================\n");
	printf("All HarfBuzz tests passed!\n");

	return 0;
}
