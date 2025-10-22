// test_colr_render.c - COLR Vector Rendering Tests
//
// Tests for Phase 6.4: COLR Vector Rendering

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_colr_render.h"

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

// Test 1: CPAL color structure
static int test_cpal_color() {
	printf("Test: CPAL color structure\n");

	VKNVGcpalColor color = {0};
	color.blue = 0xFF;
	color.green = 0x00;
	color.red = 0x00;
	color.alpha = 0xFF;

	TEST_ASSERT(color.blue == 255, "blue component");
	TEST_ASSERT(color.red == 0, "red component");
	TEST_ASSERT(color.alpha == 255, "alpha component");

	TEST_PASS("test_cpal_color");
}

// Test 2: Apply palette color to grayscale
static int test_apply_palette_color() {
	printf("Test: Apply palette color\n");

	// Create 2x2 grayscale bitmap
	uint8_t grayscale[4] = {255, 128, 64, 0};

	// Red color (BGRA in CPAL)
	VKNVGcpalColor red = {0, 0, 255, 255};

	// Output RGBA
	uint8_t rgba[16] = {0};

	vknvg__applyPaletteColor(grayscale, 2, 2, red, rgba);

	// Pixel 0: full red
	TEST_ASSERT(rgba[0] == 255, "pixel 0 red");
	TEST_ASSERT(rgba[1] == 0, "pixel 0 green");
	TEST_ASSERT(rgba[2] == 0, "pixel 0 blue");
	TEST_ASSERT(rgba[3] == 255, "pixel 0 alpha");

	// Pixel 1: half-transparent red
	TEST_ASSERT(rgba[4] == 255, "pixel 1 red");
	TEST_ASSERT(rgba[7] == 128, "pixel 1 alpha");

	// Pixel 3: fully transparent
	TEST_ASSERT(rgba[15] == 0, "pixel 3 alpha");

	TEST_PASS("test_apply_palette_color");
}

// Test 3: Apply foreground color
static int test_apply_foreground_color() {
	printf("Test: Apply foreground color\n");

	// Create 2x2 grayscale bitmap
	uint8_t grayscale[4] = {255, 128, 64, 0};

	// Blue color (RGBA32: 0x0000FFFF)
	uint32_t blue = 0x0000FFFF;

	// Output RGBA
	uint8_t rgba[16] = {0};

	vknvg__applyForegroundColor(grayscale, 2, 2, blue, rgba);

	// Pixel 0: full blue
	TEST_ASSERT(rgba[0] == 0, "pixel 0 red");
	TEST_ASSERT(rgba[1] == 0, "pixel 0 green");
	TEST_ASSERT(rgba[2] == 255, "pixel 0 blue");
	TEST_ASSERT(rgba[3] == 255, "pixel 0 alpha");

	// Pixel 1: half-transparent blue
	TEST_ASSERT(rgba[4] == 0, "pixel 1 red");
	TEST_ASSERT(rgba[6] == 255, "pixel 1 blue");
	TEST_ASSERT(rgba[7] == 128, "pixel 1 alpha");

	TEST_PASS("test_apply_foreground_color");
}

// Test 4: Simple layer compositing
static int test_composite_layer() {
	printf("Test: Layer compositing\n");

	// Destination: 2x2 opaque red
	uint8_t dst[16] = {
		255, 0, 0, 255,
		255, 0, 0, 255,
		255, 0, 0, 255,
		255, 0, 0, 255,
	};

	// Source: 2x2 semi-transparent blue
	uint8_t src[16] = {
		0, 0, 255, 128,
		0, 0, 255, 128,
		0, 0, 255, 128,
		0, 0, 255, 128,
	};

	vknvg__compositeLayer(src, dst, 2, 2);

	// Should blend to purple
	TEST_ASSERT(dst[0] > 100 && dst[0] < 150, "blended red component");
	TEST_ASSERT(dst[1] == 0, "blended green component");
	TEST_ASSERT(dst[2] > 100 && dst[2] < 150, "blended blue component");

	TEST_PASS("test_composite_layer");
}

// Test 5: Composite with full opacity
static int test_composite_opaque() {
	printf("Test: Composite opaque layer\n");

	// Destination: 2x2 red
	uint8_t dst[16] = {
		255, 0, 0, 255,
		255, 0, 0, 255,
		255, 0, 0, 255,
		255, 0, 0, 255,
	};

	// Source: 2x2 opaque blue
	uint8_t src[16] = {
		0, 0, 255, 255,
		0, 0, 255, 255,
		0, 0, 255, 255,
		0, 0, 255, 255,
	};

	vknvg__compositeLayer(src, dst, 2, 2);

	// Should completely replace with blue
	TEST_ASSERT(dst[0] == 0, "replaced red");
	TEST_ASSERT(dst[2] == 255, "replaced blue");
	TEST_ASSERT(dst[3] == 255, "replaced alpha");

	TEST_PASS("test_composite_opaque");
}

// Test 6: Composite with offset
static int test_composite_with_offset() {
	printf("Test: Composite with offset\n");

	// Destination: 4x4 black
	uint8_t dst[64] = {0};

	// Source: 2x2 white
	uint8_t src[16] = {
		255, 255, 255, 255,
		255, 255, 255, 255,
		255, 255, 255, 255,
		255, 255, 255, 255,
	};

	// Composite at offset (1, 1)
	vknvg__compositeLayerWithOffset(src, 2, 2, 1, 2,
	                                dst, 4, 4, 0, 3);

	// Check that center 2x2 is white
	uint32_t centerOffset = (1 * 4 + 1) * 4;
	TEST_ASSERT(dst[centerOffset + 0] == 255, "center white R");
	TEST_ASSERT(dst[centerOffset + 1] == 255, "center white G");
	TEST_ASSERT(dst[centerOffset + 2] == 255, "center white B");

	// Check that corner is still black
	TEST_ASSERT(dst[0] == 0, "corner black R");
	TEST_ASSERT(dst[1] == 0, "corner black G");

	TEST_PASS("test_composite_with_offset");
}

// Test 7: COLR render result structure
static int test_render_result() {
	printf("Test: COLR render result\n");

	VKNVGcolrRenderResult result = {0};
	result.width = 64;
	result.height = 64;
	result.bearingX = 5;
	result.bearingY = 60;
	result.advance = 70;

	// Allocate RGBA data
	result.rgbaData = (uint8_t*)malloc(result.width * result.height * 4);
	TEST_ASSERT(result.rgbaData != NULL, "allocation succeeded");

	TEST_ASSERT(result.width == 64, "width");
	TEST_ASSERT(result.height == 64, "height");
	TEST_ASSERT(result.bearingX == 5, "bearingX");
	TEST_ASSERT(result.bearingY == 60, "bearingY");
	TEST_ASSERT(result.advance == 70, "advance");

	free(result.rgbaData);
	TEST_PASS("test_render_result");
}

// Test 8: Renderer statistics
static int test_renderer_stats() {
	printf("Test: Renderer statistics\n");

	VKNVGcolrRenderer renderer = {0};
	renderer.layerRenders = 50;
	renderer.composites = 40;
	renderer.cacheUploads = 10;

	uint32_t layers, composites, uploads;
	vknvg__getColrRendererStats(&renderer, &layers, &composites, &uploads);

	TEST_ASSERT(layers == 50, "layer renders");
	TEST_ASSERT(composites == 40, "composites");
	TEST_ASSERT(uploads == 10, "cache uploads");

	TEST_PASS("test_renderer_stats");
}

// Test 9: Palette index special values
static int test_palette_special_values() {
	printf("Test: Palette special values\n");

	// Create mock layer with foreground color index
	VKNVGcolrLayer layer = {0};
	layer.glyphID = 100;
	layer.paletteIndex = 0xFFFF; // Foreground color

	TEST_ASSERT(layer.paletteIndex == 0xFFFF, "foreground index");

	// Regular palette index
	VKNVGcolrLayer layer2 = {0};
	layer2.glyphID = 101;
	layer2.paletteIndex = 5;

	TEST_ASSERT(layer2.paletteIndex == 5, "regular index");

	TEST_PASS("test_palette_special_values");
}

// Test 10: Null pointer safety
static int test_null_safety() {
	printf("Test: Null pointer safety\n");

	uint8_t grayscale[4] = {255, 128, 64, 0};
	uint8_t rgba[16] = {0};
	VKNVGcpalColor color = {0, 0, 255, 255};

	// Null checks (should not crash)
	vknvg__applyPaletteColor(NULL, 2, 2, color, rgba);
	vknvg__applyPaletteColor(grayscale, 2, 2, color, NULL);

	vknvg__applyForegroundColor(NULL, 2, 2, 0xFFFFFFFF, rgba);
	vknvg__applyForegroundColor(grayscale, 2, 2, 0xFFFFFFFF, NULL);

	vknvg__compositeLayer(NULL, rgba, 2, 2);
	vknvg__compositeLayer(rgba, NULL, 2, 2);

	vknvg__compositeLayerWithOffset(NULL, 2, 2, 0, 0, rgba, 2, 2, 0, 0);
	vknvg__compositeLayerWithOffset(rgba, 2, 2, 0, 0, NULL, 2, 2, 0, 0);

	vknvg__getColrRendererStats(NULL, NULL, NULL, NULL);

	TEST_PASS("test_null_safety");
}

int main() {
	printf("==========================================\n");
	printf("  COLR Vector Rendering Tests (Phase 6.4)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_cpal_color()) passed++;
	total++; if (test_apply_palette_color()) passed++;
	total++; if (test_apply_foreground_color()) passed++;
	total++; if (test_composite_layer()) passed++;
	total++; if (test_composite_opaque()) passed++;
	total++; if (test_composite_with_offset()) passed++;
	total++; if (test_render_result()) passed++;
	total++; if (test_renderer_stats()) passed++;
	total++; if (test_palette_special_values()) passed++;
	total++; if (test_null_safety()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
