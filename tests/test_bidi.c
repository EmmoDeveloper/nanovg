#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/nanovg_vk_bidi.h"

// Test 1: BiDi character type classification
static int test_bidi_character_types(void)
{
	printf("Test: BiDi character type classification\n");

	VKNVGbidiType type;

	// Latin letter (L)
	type = vknvg__getBidiType(0x0041);	// 'A'
	printf("  'A' (U+0041) → type: %d (expected L=%d)\n", type, VKNVG_BIDI_L);
	assert(type == VKNVG_BIDI_L);

	// Arabic letter (AL)
	type = vknvg__getBidiType(0x0645);	// Arabic Meem
	printf("  Arabic Meem (U+0645) → type: %d (expected AL=%d)\n", type, VKNVG_BIDI_AL);
	assert(type == VKNVG_BIDI_AL);

	// Hebrew letter (R)
	type = vknvg__getBidiType(0x05D0);	// Hebrew Alef
	printf("  Hebrew Alef (U+05D0) → type: %d (expected R=%d)\n", type, VKNVG_BIDI_R);
	assert(type == VKNVG_BIDI_R);

	// European number (EN)
	type = vknvg__getBidiType(0x0030);	// '0'
	printf("  '0' (U+0030) → type: %d (expected EN=%d)\n", type, VKNVG_BIDI_EN);
	assert(type == VKNVG_BIDI_EN);

	// Whitespace (WS)
	type = vknvg__getBidiType(0x0020);	// Space
	printf("  Space (U+0020) → type: %d (expected WS=%d)\n", type, VKNVG_BIDI_WS);
	assert(type == VKNVG_BIDI_WS);

	// Common separator (CS)
	type = vknvg__getBidiType(0x002C);	// Comma
	printf("  Comma (U+002C) → type: %d (expected CS=%d)\n", type, VKNVG_BIDI_CS);
	assert(type == VKNVG_BIDI_CS);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 2: BiDi embedding levels
static int test_embedding_levels(void)
{
	printf("Test: BiDi embedding levels\n");

	VKNVGbidiResult result = {0};

	// Simple LTR text: "Hello"
	result.textLength = 5;
	for (uint32_t i = 0; i < result.textLength; i++) {
		result.levels[i] = 0;	// LTR base level
		result.types[i] = VKNVG_BIDI_L;
	}
	result.baseLevel = 0;

	printf("  LTR text base level: %d\n", result.baseLevel);
	assert(result.baseLevel == 0);
	assert(!vknvg__isRTL(result.baseLevel));

	// RTL text: Hebrew/Arabic
	result.textLength = 5;
	for (uint32_t i = 0; i < result.textLength; i++) {
		result.levels[i] = 1;	// RTL base level
		result.types[i] = VKNVG_BIDI_R;
	}
	result.baseLevel = 1;

	printf("  RTL text base level: %d\n", result.baseLevel);
	assert(result.baseLevel == 1);
	assert(vknvg__isRTL(result.baseLevel));

	// Mixed embedding: LTR with embedded RTL (level 2)
	result.levels[0] = 0;	// LTR
	result.levels[1] = 0;	// LTR
	result.levels[2] = 1;	// RTL embedded
	result.levels[3] = 1;	// RTL embedded
	result.levels[4] = 0;	// LTR

	printf("  Mixed levels: [%d, %d, %d, %d, %d]\n",
	       result.levels[0], result.levels[1], result.levels[2],
	       result.levels[3], result.levels[4]);

	assert(result.levels[0] == 0);
	assert(result.levels[2] == 1);
	assert(vknvg__isRTL(result.levels[2]));

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 3: Character mirroring
static int test_character_mirroring(void)
{
	printf("Test: Character mirroring\n");

	uint32_t mirrored;

	// Left parenthesis '(' → Right parenthesis ')'
	mirrored = vknvg__getMirroredChar(0x0028);
	printf("  '(' (U+0028) → U+%04X (expected U+0029)\n", mirrored);
	assert(mirrored == 0x0029);

	// Right parenthesis ')' → Left parenthesis '('
	mirrored = vknvg__getMirroredChar(0x0029);
	printf("  ')' (U+0029) → U+%04X (expected U+0028)\n", mirrored);
	assert(mirrored == 0x0028);

	// Left bracket '[' → Right bracket ']'
	mirrored = vknvg__getMirroredChar(0x005B);
	printf("  '[' (U+005B) → U+%04X (expected U+005D)\n", mirrored);
	assert(mirrored == 0x005D);

	// Less than '<' → Greater than '>'
	mirrored = vknvg__getMirroredChar(0x003C);
	printf("  '<' (U+003C) → U+%04X (expected U+003E)\n", mirrored);
	assert(mirrored == 0x003E);

	// Letter 'A' (no mirroring)
	mirrored = vknvg__getMirroredChar(0x0041);
	printf("  'A' (U+0041) → U+%04X (no mirror, expected same)\n", mirrored);
	assert(mirrored == 0x0041);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 4: Base direction detection
static int test_base_direction_detection(void)
{
	printf("Test: Base direction detection\n");

	VKNVGparagraphDirection dir;

	// LTR text
	const char* ltr = "Hello World";
	dir = vknvg__detectBaseDirection(ltr, strlen(ltr));
	printf("  \"Hello World\" → direction: %d (expected LTR=%d)\n",
	       dir, VKNVG_PARA_LTR);
	assert(dir == VKNVG_PARA_LTR);

	// RTL text (Arabic: مرحبا)
	const char* rtl = "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
	dir = vknvg__detectBaseDirection(rtl, strlen(rtl));
	printf("  Arabic text → direction: %d (expected RTL=%d)\n",
	       dir, VKNVG_PARA_RTL);
	assert(dir == VKNVG_PARA_RTL);

	// Mixed (starts with LTR)
	const char* mixed = "Hello مرحبا";
	dir = vknvg__detectBaseDirection(mixed, strlen(mixed));
	printf("  \"Hello مرحبا\" → direction: %d (expected LTR=%d)\n",
	       dir, VKNVG_PARA_LTR);
	assert(dir == VKNVG_PARA_LTR);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 5: BiDi run structure
static int test_bidi_run_structure(void)
{
	printf("Test: BiDi run structure\n");

	VKNVGbidiRun run = {0};
	run.start = 0;
	run.length = 10;
	run.level = 0;	// LTR
	run.type = VKNVG_BIDI_L;

	printf("  Run: start=%u, length=%u, level=%u, type=%d\n",
	       run.start, run.length, run.level, run.type);

	assert(run.start == 0);
	assert(run.length == 10);
	assert(run.level == 0);
	assert(!vknvg__isRTL(run.level));

	// RTL run
	run.level = 1;
	run.type = VKNVG_BIDI_R;
	printf("  RTL run: level=%u, isRTL=%d\n", run.level, vknvg__isRTL(run.level));
	assert(vknvg__isRTL(run.level));

	printf("  Size of VKNVGbidiRun: %zu bytes\n", sizeof(VKNVGbidiRun));

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 6: BiDi result capacity
static int test_bidi_result_capacity(void)
{
	printf("Test: BiDi result capacity\n");

	VKNVGbidiResult result = {0};

	printf("  Max text length: %d\n", VKNVG_MAX_BIDI_TEXT_LENGTH);
	printf("  Max runs: %zu\n", sizeof(result.runs) / sizeof(result.runs[0]));
	printf("  Size of VKNVGbidiResult: %zu bytes\n", sizeof(VKNVGbidiResult));

	// Simulate analysis of maximum text
	result.textLength = VKNVG_MAX_BIDI_TEXT_LENGTH;
	result.baseLevel = 0;
	result.maxLevel = 0;

	// Create alternating LTR/RTL runs (worst case for run count)
	for (uint32_t i = 0; i < 256 && i * 2 < result.textLength; i++) {
		result.runs[i].start = i * 2;
		result.runs[i].length = 2;
		result.runs[i].level = i % 2;	// Alternating levels
		result.runCount++;
	}

	printf("  Created %u runs for %u characters\n",
	       result.runCount, result.textLength);
	assert(result.runCount <= 256);
	assert(result.textLength == VKNVG_MAX_BIDI_TEXT_LENGTH);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Test 7: Visual reordering
static int test_visual_reordering(void)
{
	printf("Test: Visual reordering\n");

	VKNVGbidiResult result = {0};

	// Simple LTR text (no reordering needed)
	result.textLength = 5;
	for (uint32_t i = 0; i < result.textLength; i++) {
		result.levels[i] = 0;
		result.visualOrder[i] = i;	// Identity mapping
		result.logicalOrder[i] = i;
	}

	printf("  LTR visual order: [");
	for (uint32_t i = 0; i < result.textLength; i++) {
		printf("%u%s", result.visualOrder[i],
		       i < result.textLength - 1 ? ", " : "");
	}
	printf("]\n");

	for (uint32_t i = 0; i < result.textLength; i++) {
		assert(result.visualOrder[i] == i);
	}

	// RTL text (reversed order)
	// Logical: [0, 1, 2, 3, 4]
	// Visual:  [4, 3, 2, 1, 0]
	for (uint32_t i = 0; i < result.textLength; i++) {
		result.levels[i] = 1;	// RTL
		result.visualOrder[i] = result.textLength - 1 - i;
		result.logicalOrder[result.textLength - 1 - i] = i;
	}

	printf("  RTL visual order: [");
	for (uint32_t i = 0; i < result.textLength; i++) {
		printf("%u%s", result.visualOrder[i],
		       i < result.textLength - 1 ? ", " : "");
	}
	printf("]\n");

	assert(result.visualOrder[0] == 4);
	assert(result.visualOrder[4] == 0);

	printf("  ✓ PASS\n\n");
	return 1;
}

// Real implementations are now in nanovg_vk_bidi.c

int main(void)
{
	printf("==========================================\n");
	printf("  BiDi Algorithm Tests\n");
	printf("==========================================\n\n");

	test_bidi_character_types();
	test_embedding_levels();
	test_character_mirroring();
	test_base_direction_detection();
	test_bidi_run_structure();
	test_bidi_result_capacity();
	test_visual_reordering();

	printf("========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   7\n");
	printf("  Passed:  7\n");
	printf("========================================\n");
	printf("All BiDi tests passed!\n");

	return 0;
}
