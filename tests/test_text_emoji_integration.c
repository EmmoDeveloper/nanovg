// test_text_emoji_integration.c - Text-Emoji Integration Tests
//
// Tests for Phase 6.6: Text Pipeline Integration

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_text_emoji.h"

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

// Test 1: Glyph render mode enum
static int test_glyph_render_mode() {
	printf("Test: Glyph render mode\n");

	VKNVGglyphRenderMode sdf = VKNVG_GLYPH_SDF;
	VKNVGglyphRenderMode emoji = VKNVG_GLYPH_COLOR_EMOJI;

	TEST_ASSERT(sdf == 0, "SDF mode value");
	TEST_ASSERT(emoji == 1, "emoji mode value");

	const char* sdfName = vknvg__getGlyphRenderModeName(sdf);
	TEST_ASSERT(strcmp(sdfName, "SDF") == 0, "SDF name");

	const char* emojiName = vknvg__getGlyphRenderModeName(emoji);
	TEST_ASSERT(strstr(emojiName, "Emoji") != NULL, "emoji name");

	TEST_PASS("test_glyph_render_mode");
}

// Test 2: Glyph render info structure
static int test_glyph_render_info() {
	printf("Test: Glyph render info structure\n");

	// SDF mode
	VKNVGglyphRenderInfo sdfInfo = {0};
	sdfInfo.mode = VKNVG_GLYPH_SDF;
	sdfInfo.sdfAtlasX = 100;
	sdfInfo.sdfAtlasY = 200;
	sdfInfo.sdfWidth = 32;
	sdfInfo.sdfHeight = 32;
	sdfInfo.bearingX = 2;
	sdfInfo.bearingY = 28;
	sdfInfo.advance = 30;

	TEST_ASSERT(sdfInfo.mode == VKNVG_GLYPH_SDF, "SDF mode");
	TEST_ASSERT(sdfInfo.sdfAtlasX == 100, "SDF atlas X");
	TEST_ASSERT(sdfInfo.sdfWidth == 32, "SDF width");
	TEST_ASSERT(sdfInfo.advance == 30, "SDF advance");

	// Color emoji mode
	VKNVGglyphRenderInfo emojiInfo = {0};
	emojiInfo.mode = VKNVG_GLYPH_COLOR_EMOJI;
	emojiInfo.colorAtlasIndex = 0;
	emojiInfo.colorAtlasX = 50;
	emojiInfo.colorAtlasY = 100;
	emojiInfo.colorWidth = 64;
	emojiInfo.colorHeight = 64;
	emojiInfo.bearingX = 5;
	emojiInfo.bearingY = 60;
	emojiInfo.advance = 70;

	TEST_ASSERT(emojiInfo.mode == VKNVG_GLYPH_COLOR_EMOJI, "emoji mode");
	TEST_ASSERT(emojiInfo.colorAtlasIndex == 0, "color atlas index");
	TEST_ASSERT(emojiInfo.colorAtlasX == 50, "color atlas X");
	TEST_ASSERT(emojiInfo.colorWidth == 64, "color width");
	TEST_ASSERT(emojiInfo.advance == 70, "emoji advance");

	TEST_PASS("test_glyph_render_info");
}

// Test 3: Text-emoji state structure
static int test_text_emoji_state() {
	printf("Test: Text-emoji state structure\n");

	VKNVGtextEmojiState state = {0};
	state.glyphsRenderedSDF = 100;
	state.glyphsRenderedEmoji = 50;
	state.emojiAttempts = 60;
	state.emojiFallbacks = 10;

	uint32_t sdf, emoji, attempts, fallbacks;
	vknvg__getTextEmojiStats(&state, &sdf, &emoji, &attempts, &fallbacks);

	TEST_ASSERT(sdf == 100, "SDF glyphs");
	TEST_ASSERT(emoji == 50, "emoji glyphs");
	TEST_ASSERT(attempts == 60, "emoji attempts");
	TEST_ASSERT(fallbacks == 10, "emoji fallbacks");

	// Reset
	vknvg__resetTextEmojiStats(&state);
	TEST_ASSERT(state.glyphsRenderedSDF == 0, "reset SDF");
	TEST_ASSERT(state.glyphsRenderedEmoji == 0, "reset emoji");
	TEST_ASSERT(state.emojiAttempts == 0, "reset attempts");

	TEST_PASS("test_text_emoji_state");
}

// Test 4: Emoji sequence detection
static int test_emoji_sequence_detection() {
	printf("Test: Emoji sequence detection\n");

	// ZWJ (Zero-Width Joiner)
	TEST_ASSERT(vknvg__isEmojiSequenceCodepoint(0x200D), "ZWJ");

	// Variation Selector-16
	TEST_ASSERT(vknvg__isEmojiSequenceCodepoint(0xFE0F), "VS-16");

	// Skin tone modifiers
	TEST_ASSERT(vknvg__isEmojiSequenceCodepoint(0x1F3FB), "skin tone light");
	TEST_ASSERT(vknvg__isEmojiSequenceCodepoint(0x1F3FF), "skin tone dark");

	// Combining enclosing keycap
	TEST_ASSERT(vknvg__isEmojiSequenceCodepoint(0x20E3), "keycap");

	// Not sequence codepoints
	TEST_ASSERT(!vknvg__isEmojiSequenceCodepoint(0x1F600), "grinning face");
	TEST_ASSERT(!vknvg__isEmojiSequenceCodepoint(0x0041), "letter A");

	TEST_PASS("test_emoji_sequence_detection");
}

// Test 5: Should render as emoji detection
static int test_should_render_as_emoji() {
	printf("Test: Should render as emoji\n");

	// Mock state without emoji renderer
	VKNVGtextEmojiState stateNoEmoji = {0};
	TEST_ASSERT(!vknvg__shouldRenderAsEmoji(&stateNoEmoji, 0x1F600), "no renderer");

	// Mock state with emoji renderer
	VKNVGtextEmojiState stateWithEmoji = {0};
	VKNVGemojiRenderer mockRenderer = {0};
	stateWithEmoji.emojiRenderer = &mockRenderer;

	// These should be considered emoji
	TEST_ASSERT(vknvg__shouldRenderAsEmoji(&stateWithEmoji, 0x1F600), "grinning face");
	TEST_ASSERT(vknvg__shouldRenderAsEmoji(&stateWithEmoji, 0x1F525), "fire");
	TEST_ASSERT(vknvg__shouldRenderAsEmoji(&stateWithEmoji, 0x2764), "heart");

	// These should not
	TEST_ASSERT(!vknvg__shouldRenderAsEmoji(&stateWithEmoji, 0x0041), "letter A");
	TEST_ASSERT(!vknvg__shouldRenderAsEmoji(&stateWithEmoji, 0x0030), "digit 0");

	TEST_PASS("test_should_render_as_emoji");
}

// Test 6: Emoji support check
static int test_emoji_support_check() {
	printf("Test: Emoji support check\n");

	// No state
	TEST_ASSERT(!vknvg__hasEmojiSupport(NULL), "null state");

	// State without renderer
	VKNVGtextEmojiState stateNoRenderer = {0};
	TEST_ASSERT(!vknvg__hasEmojiSupport(&stateNoRenderer), "no renderer");

	// State with renderer but no format
	VKNVGtextEmojiState stateNoFormat = {0};
	VKNVGemojiRenderer mockRenderer = {0};
	mockRenderer.format = VKNVG_EMOJI_NONE;
	stateNoFormat.emojiRenderer = &mockRenderer;
	TEST_ASSERT(!vknvg__hasEmojiSupport(&stateNoFormat), "no format");

	// State with SBIX format
	VKNVGtextEmojiState stateSbix = {0};
	VKNVGemojiRenderer mockSbix = {0};
	mockSbix.format = VKNVG_EMOJI_SBIX;
	stateSbix.emojiRenderer = &mockSbix;
	TEST_ASSERT(vknvg__hasEmojiSupport(&stateSbix), "SBIX format");

	TEST_PASS("test_emoji_support_check");
}

// Test 7: Render info printing (should not crash)
static int test_render_info_printing() {
	printf("Test: Render info printing\n");

	VKNVGglyphRenderInfo info = {0};
	info.mode = VKNVG_GLYPH_SDF;
	info.sdfAtlasX = 100;
	info.sdfAtlasY = 200;
	info.advance = 30;

	// Should not crash
	vknvg__printGlyphRenderInfo(&info);
	vknvg__printGlyphRenderInfo(NULL);

	TEST_PASS("test_render_info_printing");
}

// Test 8: Statistics accuracy
static int test_statistics_accuracy() {
	printf("Test: Statistics accuracy\n");

	VKNVGtextEmojiState state = {0};

	// Simulate rendering
	state.glyphsRenderedSDF = 100;
	state.glyphsRenderedEmoji = 25;
	state.emojiAttempts = 30;
	state.emojiFallbacks = 5;

	// Calculate derived metrics
	uint32_t totalGlyphs = state.glyphsRenderedSDF + state.glyphsRenderedEmoji;
	TEST_ASSERT(totalGlyphs == 125, "total glyphs");

	uint32_t successfulEmoji = state.emojiAttempts - state.emojiFallbacks;
	TEST_ASSERT(successfulEmoji == 25, "successful emoji");

	float fallbackRate = (float)state.emojiFallbacks / state.emojiAttempts;
	TEST_ASSERT(fallbackRate < 0.2f, "fallback rate under 20%");

	TEST_PASS("test_statistics_accuracy");
}

// Test 9: Null pointer safety
static int test_null_safety() {
	printf("Test: Null pointer safety\n");

	// Should not crash
	vknvg__destroyTextEmojiState(NULL);

	bool hasSupport = vknvg__hasEmojiSupport(NULL);
	TEST_ASSERT(!hasSupport, "null has no support");

	VKNVGglyphRenderInfo info = {0};
	bool result = vknvg__getGlyphRenderInfo(NULL, 0, 0, 0, 0.0f, &info);
	TEST_ASSERT(!result, "null get render info");

	result = vknvg__tryRenderAsEmoji(NULL, 0, 0, 0, 0.0f, &info);
	TEST_ASSERT(!result, "null try render");

	bool shouldRender = vknvg__shouldRenderAsEmoji(NULL, 0x1F600);
	TEST_ASSERT(!shouldRender, "null should render");

	vknvg__getTextEmojiStats(NULL, NULL, NULL, NULL, NULL);
	vknvg__resetTextEmojiStats(NULL);

	TEST_PASS("test_null_safety");
}

// Test 10: Mode names
static int test_mode_names() {
	printf("Test: Mode names\n");

	const char* sdf = vknvg__getGlyphRenderModeName(VKNVG_GLYPH_SDF);
	TEST_ASSERT(sdf != NULL, "SDF name not null");
	TEST_ASSERT(strlen(sdf) > 0, "SDF name not empty");

	const char* emoji = vknvg__getGlyphRenderModeName(VKNVG_GLYPH_COLOR_EMOJI);
	TEST_ASSERT(emoji != NULL, "emoji name not null");
	TEST_ASSERT(strlen(emoji) > 0, "emoji name not empty");

	const char* unknown = vknvg__getGlyphRenderModeName((VKNVGglyphRenderMode)999);
	TEST_ASSERT(unknown != NULL, "unknown name not null");

	TEST_PASS("test_mode_names");
}

int main() {
	printf("==========================================\n");
	printf("  Text-Emoji Integration Tests (Phase 6.6)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_glyph_render_mode()) passed++;
	total++; if (test_glyph_render_info()) passed++;
	total++; if (test_text_emoji_state()) passed++;
	total++; if (test_emoji_sequence_detection()) passed++;
	total++; if (test_should_render_as_emoji()) passed++;
	total++; if (test_emoji_support_check()) passed++;
	total++; if (test_render_info_printing()) passed++;
	total++; if (test_statistics_accuracy()) passed++;
	total++; if (test_null_safety()) passed++;
	total++; if (test_mode_names()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
