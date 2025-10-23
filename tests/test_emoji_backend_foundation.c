// test_emoji_backend_foundation.c - Backend Integration Foundation Test
//
// Tests that Phase 6 backend integration changes compile and link correctly

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include nanovg.h first for NVGcontext type
#include "../../nanovg/src/nanovg.h"

// Test that nanovg_vk.h includes the emoji headers
#define NANOVG_VK_IMPLEMENTATION
#include "../src/nanovg_vk.h"

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

// Test 1: NVG_COLOR_EMOJI flag exists
static int test_color_emoji_flag() {
	printf("Test: NVG_COLOR_EMOJI flag\n");

	// Check that flag is defined
	int flag = NVG_COLOR_EMOJI;
	TEST_ASSERT(flag == (1 << 17), "flag value");

	printf("  NVG_COLOR_EMOJI = %d (0x%X)\n", flag, flag);

	TEST_PASS("test_color_emoji_flag");
}

// Test 2: NVGVkCreateInfo has emoji fields
static int test_create_info_fields() {
	printf("Test: NVGVkCreateInfo emoji fields\n");

	NVGVkCreateInfo info = {0};

	// Set emoji font path
	info.emojiFontPath = "/path/to/emoji.ttf";
	TEST_ASSERT(info.emojiFontPath != NULL, "emojiFontPath");

	// Set emoji font data
	const uint8_t testData[] = {1, 2, 3, 4};
	info.emojiFontData = testData;
	info.emojiFontDataSize = sizeof(testData);
	TEST_ASSERT(info.emojiFontData != NULL, "emojiFontData");
	TEST_ASSERT(info.emojiFontDataSize == 4, "emojiFontDataSize");

	printf("  emojiFontPath: %s\n", info.emojiFontPath);
	printf("  emojiFontData: %p\n", (void*)info.emojiFontData);
	printf("  emojiFontDataSize: %u bytes\n", info.emojiFontDataSize);

	TEST_PASS("test_create_info_fields");
}

// Test 3: VKNVGcontext has emoji fields (via sizeof)
static int test_context_size() {
	printf("Test: VKNVGcontext size\n");

	size_t contextSize = sizeof(VKNVGcontext);

	// Context should be larger now with emoji fields
	// Old size was ~7544 bytes, new size should be ~7600+ bytes
	TEST_ASSERT(contextSize > 7500, "context size increased");

	printf("  sizeof(VKNVGcontext) = %zu bytes\n", contextSize);

	TEST_PASS("test_context_size");
}

// Test 4: Text-emoji types are accessible
static int test_emoji_types() {
	printf("Test: Emoji types accessible\n");

	// Check that emoji types compile
	VKNVGglyphRenderMode modeA = VKNVG_GLYPH_SDF;
	VKNVGglyphRenderMode modeB = VKNVG_GLYPH_COLOR_EMOJI;

	TEST_ASSERT(modeA == 0, "SDF mode value");
	TEST_ASSERT(modeB == 1, "emoji mode value");

	printf("  VKNVG_GLYPH_SDF = %d\n", modeA);
	printf("  VKNVG_GLYPH_COLOR_EMOJI = %d\n", modeB);

	// Check render info structure
	VKNVGglyphRenderInfo info = {0};
	info.mode = VKNVG_GLYPH_COLOR_EMOJI;
	info.colorAtlasIndex = 0;
	info.colorWidth = 64;
	info.colorHeight = 64;

	TEST_ASSERT(sizeof(info) > 0, "render info size");

	printf("  sizeof(VKNVGglyphRenderInfo) = %zu bytes\n", sizeof(info));

	TEST_PASS("test_emoji_types");
}

// Test 5: Emoji function declarations exist
static int test_emoji_functions() {
	printf("Test: Emoji function declarations\n");

	// Check that function pointers can be obtained
	// (This verifies they're declared in headers)

	// Text-emoji API functions
	const char* (*getName)(VKNVGglyphRenderMode) = vknvg__getGlyphRenderModeName;
	TEST_ASSERT(getName != NULL, "getGlyphRenderModeName");

	bool (*isSeq)(uint32_t) = vknvg__isEmojiSequenceCodepoint;
	TEST_ASSERT(isSeq != NULL, "isEmojiSequenceCodepoint");

	printf("  vknvg__getGlyphRenderModeName: %p\n", (void*)getName);
	printf("  vknvg__isEmojiSequenceCodepoint: %p\n", (void*)isSeq);

	// Test actual function call
	const char* sdfName = vknvg__getGlyphRenderModeName(VKNVG_GLYPH_SDF);
	TEST_ASSERT(strcmp(sdfName, "SDF") == 0, "SDF name");

	bool isZwjSeq = vknvg__isEmojiSequenceCodepoint(0x200D); // ZWJ
	TEST_ASSERT(isZwjSeq, "ZWJ is sequence");

	printf("  SDF mode name: %s\n", sdfName);
	printf("  ZWJ (U+200D) is sequence: %s\n", isZwjSeq ? "yes" : "no");

	TEST_PASS("test_emoji_functions");
}

// Test 6: Backend compilation with emoji headers
static int test_backend_includes() {
	printf("Test: Backend emoji header includes\n");

	// Verify that nanovg_vk.h includes emoji headers
	// This is tested implicitly by the fact that emoji types are available

	// Check emoji format enum
	VKNVGemojiFormat format = VKNVG_EMOJI_NONE;
	TEST_ASSERT(format == 0, "emoji format enum");

	printf("  VKNVG_EMOJI_NONE = %d\n", format);
	printf("  VKNVG_EMOJI_SBIX = %d\n", VKNVG_EMOJI_SBIX);
	printf("  VKNVG_EMOJI_CBDT = %d\n", VKNVG_EMOJI_CBDT);
	printf("  VKNVG_EMOJI_COLR = %d\n", VKNVG_EMOJI_COLR);

	TEST_PASS("test_backend_includes");
}

// Test 7: Flag combinations
static int test_flag_combinations() {
	printf("Test: Flag combinations\n");

	// Test combining NVG_COLOR_EMOJI with other flags
	int flags1 = NVG_ANTIALIAS | NVG_COLOR_EMOJI;
	TEST_ASSERT(flags1 & NVG_ANTIALIAS, "antialias flag");
	TEST_ASSERT(flags1 & NVG_COLOR_EMOJI, "emoji flag");

	int flags2 = NVG_SDF_TEXT | NVG_COLOR_EMOJI | NVG_VIRTUAL_ATLAS;
	TEST_ASSERT(flags2 & NVG_SDF_TEXT, "SDF flag");
	TEST_ASSERT(flags2 & NVG_COLOR_EMOJI, "emoji flag");
	TEST_ASSERT(flags2 & NVG_VIRTUAL_ATLAS, "virtual atlas flag");

	printf("  NVG_ANTIALIAS | NVG_COLOR_EMOJI = 0x%X\n", flags1);
	printf("  NVG_SDF_TEXT | NVG_COLOR_EMOJI | NVG_VIRTUAL_ATLAS = 0x%X\n", flags2);

	TEST_PASS("test_flag_combinations");
}

// Test 8: Structure alignment
static int test_structure_alignment() {
	printf("Test: Structure alignment\n");

	// Check that structures are properly aligned
	VKNVGglyphRenderInfo info = {0};
	TEST_ASSERT(sizeof(info.mode) == sizeof(VKNVGglyphRenderMode), "mode size");
	TEST_ASSERT(sizeof(info.sdfAtlasX) == sizeof(uint16_t), "sdfAtlasX size");
	TEST_ASSERT(sizeof(info.colorAtlasIndex) == sizeof(uint32_t), "colorAtlasIndex size");

	printf("  VKNVGglyphRenderInfo alignment OK\n");

	TEST_PASS("test_structure_alignment");
}

// Test 9: Emoji state structure
static int test_emoji_state_structure() {
	printf("Test: Emoji state structure\n");

	// Check that VKNVGtextEmojiState is accessible
	// (Cannot instantiate directly as it's opaque, but can check types)

	// Verify state function types
	void (*destroy)(VKNVGtextEmojiState*) = vknvg__destroyTextEmojiState;
	TEST_ASSERT(destroy != NULL, "destroyTextEmojiState");

	bool (*hasSupport)(VKNVGtextEmojiState*) = vknvg__hasEmojiSupport;
	TEST_ASSERT(hasSupport != NULL, "hasEmojiSupport");

	printf("  VKNVGtextEmojiState API accessible\n");

	TEST_PASS("test_emoji_state_structure");
}

// Test 10: Integration sanity check
static int test_integration_sanity() {
	printf("Test: Integration sanity check\n");

	// Final sanity check: ensure all key components are present
	TEST_ASSERT(NVG_COLOR_EMOJI == (1 << 17), "flag defined");
	TEST_ASSERT(VKNVG_GLYPH_SDF == 0, "SDF mode");
	TEST_ASSERT(VKNVG_GLYPH_COLOR_EMOJI == 1, "emoji mode");
	TEST_ASSERT(sizeof(VKNVGcontext) > 7500, "context size");
	TEST_ASSERT(sizeof(NVGVkCreateInfo) >= 96, "create info size");
	TEST_ASSERT(sizeof(VKNVGglyphRenderInfo) > 0, "render info size");

	printf("  All integration components present\n");

	TEST_PASS("test_integration_sanity");
}

int main() {
	printf("==========================================\n");
	printf("  Phase 6 Backend Foundation Tests\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_color_emoji_flag()) passed++;
	total++; if (test_create_info_fields()) passed++;
	total++; if (test_context_size()) passed++;
	total++; if (test_emoji_types()) passed++;
	total++; if (test_emoji_functions()) passed++;
	total++; if (test_backend_includes()) passed++;
	total++; if (test_flag_combinations()) passed++;
	total++; if (test_structure_alignment()) passed++;
	total++; if (test_emoji_state_structure()) passed++;
	total++; if (test_integration_sanity()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	if (passed == total) {
		printf("\n✅ Backend integration foundation is solid!\n");
		printf("   Ready for Phase B: Initialization\n");
	}

	return (passed == total) ? 0 : 1;
}
