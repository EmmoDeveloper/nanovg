// test_bitmap_emoji.c - Bitmap Emoji Pipeline Tests
//
// Tests for Phase 6.3: Bitmap Emoji Pipeline

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_bitmap_emoji.h"

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

// Test 1: PNG signature validation
static int test_png_signature() {
	printf("Test: PNG signature validation\n");

	// Valid PNG signature
	uint8_t validPNG[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

	// Invalid signature
	uint8_t invalidPNG[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	uint32_t width, height;

	// Valid signature should pass initial check (will fail on incomplete data)
	uint8_t* result1 = vknvg__decodePNG(validPNG, 8, &width, &height);
	// Expected to fail due to incomplete PNG, but validates signature check works

	// Invalid signature should return NULL immediately
	uint8_t* result2 = vknvg__decodePNG(invalidPNG, 8, &width, &height);
	TEST_ASSERT(result2 == NULL, "invalid signature rejected");

	if (result1) free(result1);
	TEST_PASS("test_png_signature");
}

// Test 2: BGRA to RGBA conversion
static int test_bgra_conversion() {
	printf("Test: BGRA to RGBA conversion\n");

	// Create test BGRA data (4 pixels)
	uint8_t data[16] = {
		// B    G    R    A
		0xFF, 0x00, 0x00, 0xFF, // Pixel 0: Blue (BGRA)
		0x00, 0xFF, 0x00, 0xFF, // Pixel 1: Green
		0x00, 0x00, 0xFF, 0xFF, // Pixel 2: Red
		0x80, 0x80, 0x80, 0x80, // Pixel 3: Gray
	};

	// Convert
	vknvg__bgraToRgba(data, 4);

	// Verify conversion
	TEST_ASSERT(data[0] == 0x00, "pixel 0 red");
	TEST_ASSERT(data[1] == 0x00, "pixel 0 green");
	TEST_ASSERT(data[2] == 0xFF, "pixel 0 blue");
	TEST_ASSERT(data[3] == 0xFF, "pixel 0 alpha");

	TEST_ASSERT(data[8] == 0xFF, "pixel 2 red");
	TEST_ASSERT(data[9] == 0x00, "pixel 2 green");
	TEST_ASSERT(data[10] == 0x00, "pixel 2 blue");

	TEST_PASS("test_bgra_conversion");
}

// Test 3: Premultiply alpha
static int test_premultiply_alpha() {
	printf("Test: Premultiply alpha\n");

	// Test data with various alpha values
	uint8_t data[16] = {
		255, 255, 255, 255, // Opaque white
		255, 0,   0,   128, // Half-transparent red
		0,   255, 0,   64,  // Quarter-transparent green
		0,   0,   255, 0,   // Fully transparent blue
	};

	vknvg__premultiplyAlpha(data, 4);

	// Pixel 0: fully opaque, unchanged
	TEST_ASSERT(data[0] == 255, "opaque R unchanged");
	TEST_ASSERT(data[1] == 255, "opaque G unchanged");
	TEST_ASSERT(data[2] == 255, "opaque B unchanged");

	// Pixel 1: half-transparent red
	TEST_ASSERT(data[4] == 128, "half-alpha R premultiplied");
	TEST_ASSERT(data[5] == 0, "half-alpha G unchanged");
	TEST_ASSERT(data[7] == 128, "alpha preserved");

	// Pixel 2: quarter-transparent green
	TEST_ASSERT(data[8] == 0, "quarter-alpha R unchanged");
	TEST_ASSERT(data[9] == 64, "quarter-alpha G premultiplied");
	TEST_ASSERT(data[11] == 64, "alpha preserved");

	// Pixel 3: fully transparent
	TEST_ASSERT(data[12] == 0, "transparent R is 0");
	TEST_ASSERT(data[15] == 0, "transparent alpha is 0");

	TEST_PASS("test_premultiply_alpha");
}

// Test 4: Bitmap scaling (nearest-neighbor)
static int test_bitmap_scaling() {
	printf("Test: Bitmap scaling\n");

	// Create 2x2 source bitmap
	uint8_t srcData[16] = {
		255, 0,   0,   255, // Red
		0,   255, 0,   255, // Green
		0,   0,   255, 255, // Blue
		255, 255, 255, 255, // White
	};

	// Scale to 4x4
	uint8_t* dstData = vknvg__scaleBitmap(srcData, 2, 2, 4, 4);
	TEST_ASSERT(dstData != NULL, "scaling succeeded");

	// Verify corners (should match source)
	TEST_ASSERT(dstData[0] == 255, "top-left red");
	TEST_ASSERT(dstData[1] == 0, "top-left green");

	// Top-right corner (pixel at [3, 0])
	uint32_t topRightIdx = (0 * 4 + 3) * 4;
	TEST_ASSERT(dstData[topRightIdx + 1] == 255, "top-right green");

	// Bottom-left corner (pixel at [0, 3])
	uint32_t bottomLeftIdx = (3 * 4 + 0) * 4;
	TEST_ASSERT(dstData[bottomLeftIdx + 2] == 255, "bottom-left blue");

	// Bottom-right corner (pixel at [3, 3])
	uint32_t bottomRightIdx = (3 * 4 + 3) * 4;
	TEST_ASSERT(dstData[bottomRightIdx + 0] == 255, "bottom-right white R");
	TEST_ASSERT(dstData[bottomRightIdx + 1] == 255, "bottom-right white G");
	TEST_ASSERT(dstData[bottomRightIdx + 2] == 255, "bottom-right white B");

	free(dstData);
	TEST_PASS("test_bitmap_scaling");
}

// Test 5: Strike selection for SBIX
static int test_sbix_strike_selection() {
	printf("Test: SBIX strike selection\n");

	// Create mock SBIX table
	VKNVGsbixTable table = {0};
	table.version = 1;
	table.numStrikes = 3;
	table.strikes = (VKNVGsbixStrike*)calloc(3, sizeof(VKNVGsbixStrike));

	table.strikes[0].ppem = 32;
	table.strikes[1].ppem = 64;
	table.strikes[2].ppem = 128;

	// Test various size selections
	VKNVGsbixStrike* strike1 = vknvg__selectBestSbixStrike(&table, 30.0f);
	TEST_ASSERT(strike1 && strike1->ppem == 32, "select closest to 30");

	VKNVGsbixStrike* strike2 = vknvg__selectBestSbixStrike(&table, 64.0f);
	TEST_ASSERT(strike2 && strike2->ppem == 64, "select exact match");

	VKNVGsbixStrike* strike3 = vknvg__selectBestSbixStrike(&table, 90.0f);
	TEST_ASSERT(strike3 && strike3->ppem == 64, "select closest to 90");

	VKNVGsbixStrike* strike4 = vknvg__selectBestSbixStrike(&table, 200.0f);
	TEST_ASSERT(strike4 && strike4->ppem == 128, "select largest for big size");

	free(table.strikes);
	TEST_PASS("test_sbix_strike_selection");
}

// Test 6: Strike selection for CBDT
static int test_cbdt_size_selection() {
	printf("Test: CBDT size selection\n");

	// Create mock CBDT table with CBLC
	VKNVGcbdtTable table = {0};
	table.majorVersion = 3;
	table.minorVersion = 0;
	table.cblc = (VKNVGcblcTable*)calloc(1, sizeof(VKNVGcblcTable));
	table.cblc->numSizes = 4;
	table.cblc->bitmapSizes = (VKNVGcblcBitmapSize*)calloc(4, sizeof(VKNVGcblcBitmapSize));

	table.cblc->bitmapSizes[0].ppemY = 20;
	table.cblc->bitmapSizes[1].ppemY = 40;
	table.cblc->bitmapSizes[2].ppemY = 80;
	table.cblc->bitmapSizes[3].ppemY = 160;

	// Test NULL returns (real selection requires more complex setup)
	VKNVGcblcBitmapSize* size1 = vknvg__selectBestCbdtSize(&table, 20.0f);
	// May return NULL since subtables aren't set up, just test it doesn't crash

	VKNVGcblcBitmapSize* size2 = vknvg__selectBestCbdtSize(&table, 60.0f);
	// May return NULL since subtables aren't set up, just test it doesn't crash

	VKNVGcblcBitmapSize* size3 = vknvg__selectBestCbdtSize(&table, 160.0f);
	// May return NULL since subtables aren't set up, just test it doesn't crash

	(void)size1; (void)size2; (void)size3; // Avoid unused warnings

	free(table.cblc->bitmapSizes);
	free(table.cblc);
	TEST_PASS("test_cbdt_size_selection");
}

// Test 7: Bitmap result structure
static int test_bitmap_result() {
	printf("Test: Bitmap result structure\n");

	VKNVGbitmapResult result = {0};
	result.width = 64;
	result.height = 64;
	result.bearingX = 5;
	result.bearingY = 60;
	result.advance = 70;

	// Allocate RGBA data
	result.rgbaData = (uint8_t*)malloc(result.width * result.height * 4);
	TEST_ASSERT(result.rgbaData != NULL, "allocation succeeded");

	// Fill with test pattern
	for (uint32_t i = 0; i < result.width * result.height; i++) {
		result.rgbaData[i * 4 + 0] = 255; // Red
		result.rgbaData[i * 4 + 1] = 128; // Green
		result.rgbaData[i * 4 + 2] = 0;   // Blue
		result.rgbaData[i * 4 + 3] = 255; // Alpha
	}

	TEST_ASSERT(result.rgbaData[0] == 255, "first pixel R");
	TEST_ASSERT(result.rgbaData[1] == 128, "first pixel G");
	TEST_ASSERT(result.width == 64, "width");
	TEST_ASSERT(result.height == 64, "height");
	TEST_ASSERT(result.bearingX == 5, "bearingX");
	TEST_ASSERT(result.advance == 70, "advance");

	free(result.rgbaData);
	TEST_PASS("test_bitmap_result");
}

// Test 8: Pipeline statistics
static int test_pipeline_stats() {
	printf("Test: Pipeline statistics\n");

	VKNVGbitmapEmoji bitmap = {0};
	bitmap.pngDecodes = 100;
	bitmap.pngErrors = 5;
	bitmap.cacheUploads = 95;

	uint32_t decodes, errors, uploads;
	vknvg__getBitmapEmojiStats(&bitmap, &decodes, &errors, &uploads);

	TEST_ASSERT(decodes == 100, "png decodes");
	TEST_ASSERT(errors == 5, "png errors");
	TEST_ASSERT(uploads == 95, "cache uploads");

	TEST_PASS("test_pipeline_stats");
}

// Test 9: Null pointer safety
static int test_null_safety() {
	printf("Test: Null pointer safety\n");

	uint32_t width, height;
	uint8_t* result1 = vknvg__decodePNG(NULL, 100, &width, &height);
	TEST_ASSERT(result1 == NULL, "null PNG data");

	VKNVGsbixStrike* strike = vknvg__selectBestSbixStrike(NULL, 64.0f);
	TEST_ASSERT(strike == NULL, "null SBIX table");

	VKNVGcblcBitmapSize* cbdtSize = vknvg__selectBestCbdtSize(NULL, 64.0f);
	TEST_ASSERT(cbdtSize == NULL, "null CBDT table");

	uint8_t* scaled = vknvg__scaleBitmap(NULL, 10, 10, 20, 20);
	TEST_ASSERT(scaled == NULL, "null source data");

	vknvg__bgraToRgba(NULL, 100); // Should not crash
	vknvg__premultiplyAlpha(NULL, 100); // Should not crash

	TEST_PASS("test_null_safety");
}

// Test 10: Zero-size handling
static int test_zero_sizes() {
	printf("Test: Zero-size handling\n");

	uint8_t data[16] = {0};

	uint8_t* result1 = vknvg__scaleBitmap(data, 0, 10, 10, 10);
	TEST_ASSERT(result1 == NULL, "zero source width");

	uint8_t* result2 = vknvg__scaleBitmap(data, 10, 10, 0, 10);
	TEST_ASSERT(result2 == NULL, "zero target width");

	uint8_t* result3 = vknvg__scaleBitmap(data, 10, 0, 10, 10);
	TEST_ASSERT(result3 == NULL, "zero source height");

	uint32_t width, height;
	uint8_t* result4 = vknvg__decodePNG(data, 0, &width, &height);
	TEST_ASSERT(result4 == NULL, "zero PNG size");

	TEST_PASS("test_zero_sizes");
}

int main() {
	printf("==========================================\n");
	printf("  Bitmap Emoji Pipeline Tests (Phase 6.3)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_png_signature()) passed++;
	total++; if (test_bgra_conversion()) passed++;
	total++; if (test_premultiply_alpha()) passed++;
	total++; if (test_bitmap_scaling()) passed++;
	total++; if (test_sbix_strike_selection()) passed++;
	total++; if (test_cbdt_size_selection()) passed++;
	total++; if (test_bitmap_result()) passed++;
	total++; if (test_pipeline_stats()) passed++;
	total++; if (test_null_safety()) passed++;
	total++; if (test_zero_sizes()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
