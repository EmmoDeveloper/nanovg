// test_emoji_integration.c - Emoji Integration Layer Tests
//
// Tests for Phase 6.5: Emoji Integration Layer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_emoji.h"

#define TEST_ASSERT(cond, msg) do { \
	if (!(cond)) { \
		printf("  ✗ FAIL at line %d: %s\n", __LINE__, msg); \
		return 0; \
	} \
} while(0)

#define TEST_PASS(name) do { \
	printf("  ✓ PASS %s\n", name); \
	return 1; \
} while(0)

// Test 1: Emoji codepoint detection
static int test_emoji_codepoint_detection() {
	printf("Test: Emoji codepoint detection\n");

	// Common emoji
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F600), "grinning face 😀");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F4A9), "pile of poo 💩");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F525), "fire 🔥");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x2764), "red heart ❤️");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F44D), "thumbs up 👍");

	// Not emoji
	TEST_ASSERT(!vknvg__isEmojiCodepoint(0x0041), "letter A");
	TEST_ASSERT(!vknvg__isEmojiCodepoint(0x0030), "digit 0");
	TEST_ASSERT(!vknvg__isEmojiCodepoint(0x0020), "space");

	TEST_PASS("test_emoji_codepoint_detection");
}

// Test 2: Emoji modifiers
static int test_emoji_modifiers() {
	printf("Test: Emoji modifiers\n");

	// Skin tone modifiers
	TEST_ASSERT(vknvg__isEmojiModifier(0x1F3FB), "light skin tone");
	TEST_ASSERT(vknvg__isEmojiModifier(0x1F3FC), "medium-light skin tone");
	TEST_ASSERT(vknvg__isEmojiModifier(0x1F3FD), "medium skin tone");
	TEST_ASSERT(vknvg__isEmojiModifier(0x1F3FE), "medium-dark skin tone");
	TEST_ASSERT(vknvg__isEmojiModifier(0x1F3FF), "dark skin tone");

	// Not modifiers
	TEST_ASSERT(!vknvg__isEmojiModifier(0x1F600), "grinning face");
	TEST_ASSERT(!vknvg__isEmojiModifier(0x0041), "letter A");

	TEST_PASS("test_emoji_modifiers");
}

// Test 3: ZWJ detection
static int test_zwj_detection() {
	printf("Test: ZWJ detection\n");

	TEST_ASSERT(vknvg__isZWJ(0x200D), "zero-width joiner");
	TEST_ASSERT(!vknvg__isZWJ(0x200C), "zero-width non-joiner");
	TEST_ASSERT(!vknvg__isZWJ(0x0041), "letter A");

	TEST_PASS("test_zwj_detection");
}

// Test 4: Format names
static int test_format_names() {
	printf("Test: Format names\n");

	const char* none = vknvg__getEmojiFormatName(VKNVG_EMOJI_NONE);
	TEST_ASSERT(strcmp(none, "None") == 0, "none format");

	const char* sbix = vknvg__getEmojiFormatName(VKNVG_EMOJI_SBIX);
	TEST_ASSERT(strstr(sbix, "SBIX") != NULL, "sbix format");

	const char* cbdt = vknvg__getEmojiFormatName(VKNVG_EMOJI_CBDT);
	TEST_ASSERT(strstr(cbdt, "CBDT") != NULL, "cbdt format");

	const char* colr = vknvg__getEmojiFormatName(VKNVG_EMOJI_COLR);
	TEST_ASSERT(strstr(colr, "COLR") != NULL, "colr format");

	TEST_PASS("test_format_names");
}

// Test 5: Format color support
static int test_format_color_support() {
	printf("Test: Format color support\n");

	TEST_ASSERT(!vknvg__formatSupportsColor(VKNVG_EMOJI_NONE), "none");
	TEST_ASSERT(vknvg__formatSupportsColor(VKNVG_EMOJI_SBIX), "sbix");
	TEST_ASSERT(vknvg__formatSupportsColor(VKNVG_EMOJI_CBDT), "cbdt");
	TEST_ASSERT(vknvg__formatSupportsColor(VKNVG_EMOJI_COLR), "colr");
	TEST_ASSERT(vknvg__formatSupportsColor(VKNVG_EMOJI_SVG), "svg");

	TEST_PASS("test_format_color_support");
}

// Test 6: Statistics structure
static int test_statistics() {
	printf("Test: Statistics\n");

	VKNVGemojiRenderer renderer = {0};
	renderer.emojiRendered = 100;
	renderer.cacheHits = 80;
	renderer.cacheMisses = 20;
	renderer.formatSwitches = 5;

	uint32_t rendered, hits, misses, switches;
	vknvg__getEmojiStats(&renderer, &rendered, &hits, &misses, &switches);

	TEST_ASSERT(rendered == 100, "emoji rendered");
	TEST_ASSERT(hits == 80, "cache hits");
	TEST_ASSERT(misses == 20, "cache misses");
	TEST_ASSERT(switches == 5, "format switches");

	// Reset
	vknvg__resetEmojiStats(&renderer);
	TEST_ASSERT(renderer.emojiRendered == 0, "reset rendered");
	TEST_ASSERT(renderer.cacheHits == 0, "reset hits");
	TEST_ASSERT(renderer.cacheMisses == 0, "reset misses");

	TEST_PASS("test_statistics");
}

// Test 7: Emoji metrics structure
static int test_emoji_metrics() {
	printf("Test: Emoji metrics\n");

	VKNVGemojiMetrics metrics = {0};
	metrics.width = 64;
	metrics.height = 64;
	metrics.bearingX = 5;
	metrics.bearingY = 60;
	metrics.advance = 70;
	metrics.atlasIndex = 0;
	metrics.atlasX = 100;
	metrics.atlasY = 200;

	TEST_ASSERT(metrics.width == 64, "width");
	TEST_ASSERT(metrics.height == 64, "height");
	TEST_ASSERT(metrics.bearingX == 5, "bearingX");
	TEST_ASSERT(metrics.bearingY == 60, "bearingY");
	TEST_ASSERT(metrics.advance == 70, "advance");
	TEST_ASSERT(metrics.atlasIndex == 0, "atlasIndex");
	TEST_ASSERT(metrics.atlasX == 100, "atlasX");
	TEST_ASSERT(metrics.atlasY == 200, "atlasY");

	TEST_PASS("test_emoji_metrics");
}

// Test 8: Unicode ranges
static int test_unicode_ranges() {
	printf("Test: Unicode ranges\n");

	// Emoji range 1 (Miscellaneous Symbols and Pictographs)
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F300), "start of range 1");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F500), "middle of range 1");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F6FF), "end of range 1");

	// Emoji range 2 (Supplemental Symbols and Pictographs)
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F900), "start of range 2");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1F9C0), "middle of range 2");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x1FAFF), "end of range 2");

	// Emoji range 3 (Dingbats and Miscellaneous Symbols)
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x2600), "start of range 3");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x2700), "middle of range 3");
	TEST_ASSERT(vknvg__isEmojiCodepoint(0x27BF), "end of range 3");

	TEST_PASS("test_unicode_ranges");
}

// Test 9: Null pointer safety
static int test_null_safety() {
	printf("Test: Null pointer safety\n");

	// Should not crash
	vknvg__destroyEmojiRenderer(NULL);

	VKNVGemojiFormat format = vknvg__getEmojiFormat(NULL);
	TEST_ASSERT(format == VKNVG_EMOJI_NONE, "null format");

	VKNVGcolorGlyphCacheEntry* entry = vknvg__renderEmoji(NULL, 0, 0, 0.0f);
	TEST_ASSERT(entry == NULL, "null render");

	VKNVGemojiMetrics metrics = {0};
	bool result = vknvg__getEmojiMetrics(NULL, 0, 0.0f, &metrics);
	TEST_ASSERT(!result, "null metrics");

	bool isEmoji = vknvg__isColorEmoji(NULL, 0);
	TEST_ASSERT(!isEmoji, "null emoji check");

	vknvg__getEmojiStats(NULL, NULL, NULL, NULL, NULL);
	vknvg__resetEmojiStats(NULL);

	float size = vknvg__getRecommendedEmojiSize(NULL, 48.0f);
	TEST_ASSERT(size == 48.0f, "null size fallback");

	TEST_PASS("test_null_safety");
}

// Test 10: Format detection
static int test_format_detection() {
	printf("Test: Format detection\n");

	// Mock renderer structure
	VKNVGemojiRenderer renderer = {0};

	// Test SBIX format
	renderer.format = VKNVG_EMOJI_SBIX;
	TEST_ASSERT(vknvg__getEmojiFormat(&renderer) == VKNVG_EMOJI_SBIX, "sbix format");

	// Test CBDT format
	renderer.format = VKNVG_EMOJI_CBDT;
	TEST_ASSERT(vknvg__getEmojiFormat(&renderer) == VKNVG_EMOJI_CBDT, "cbdt format");

	// Test COLR format
	renderer.format = VKNVG_EMOJI_COLR;
	TEST_ASSERT(vknvg__getEmojiFormat(&renderer) == VKNVG_EMOJI_COLR, "colr format");

	// Test None format
	renderer.format = VKNVG_EMOJI_NONE;
	TEST_ASSERT(vknvg__getEmojiFormat(&renderer) == VKNVG_EMOJI_NONE, "none format");

	TEST_PASS("test_format_detection");
}

int main() {
	printf("==========================================\n");
	printf("  Emoji Integration Tests (Phase 6.5)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_emoji_codepoint_detection()) passed++;
	total++; if (test_emoji_modifiers()) passed++;
	total++; if (test_zwj_detection()) passed++;
	total++; if (test_format_names()) passed++;
	total++; if (test_format_color_support()) passed++;
	total++; if (test_statistics()) passed++;
	total++; if (test_emoji_metrics()) passed++;
	total++; if (test_unicode_ranges()) passed++;
	total++; if (test_null_safety()) passed++;
	total++; if (test_format_detection()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
