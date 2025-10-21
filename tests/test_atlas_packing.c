#include <stdio.h>
#include <assert.h>
#include "../src/nanovg_vk_atlas_packing.h"

static int test_init_packer(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 1024, 1024);

	assert(packer.atlasWidth == 1024);
	assert(packer.atlasHeight == 1024);
	assert(packer.totalArea == 1024 * 1024);
	assert(packer.freeRectCount == 1);
	assert(packer.freeRects[0].x == 0);
	assert(packer.freeRects[0].y == 0);
	assert(packer.freeRects[0].width == 1024);
	assert(packer.freeRects[0].height == 1024);

	printf("  ✓ PASS test_init_packer\n");
	return 1;
}

static int test_single_allocation(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 1024, 1024);

	VKNVGrect rect;
	int result = vknvg__packRect(&packer, 100, 200, &rect,
	                              VKNVG_PACK_BEST_AREA_FIT,
	                              VKNVG_SPLIT_SHORTER_AXIS);

	assert(result == 1);
	assert(rect.x == 0);
	assert(rect.y == 0);
	assert(rect.width == 100);
	assert(rect.height == 200);
	assert(packer.allocatedArea == 100 * 200);
	assert(packer.allocationCount == 1);

	printf("    Allocated 100x200 at (%d,%d)\n", rect.x, rect.y);
	printf("    Free rects: %d\n", packer.freeRectCount);
	printf("    Allocated area: %d / %d (%.1f%%)\n",
	       packer.allocatedArea, packer.totalArea,
	       100.0f * packer.allocatedArea / packer.totalArea);

	printf("  ✓ PASS test_single_allocation\n");
	return 1;
}

static int test_multiple_allocations(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 512, 512);

	VKNVGrect rects[10];
	const uint16_t sizes[][2] = {
		{64, 64}, {32, 128}, {128, 32}, {96, 96}, {48, 72},
		{80, 40}, {60, 100}, {50, 50}, {40, 80}, {70, 70}
	};

	printf("    Allocating 10 rectangles...\n");
	for (int i = 0; i < 10; i++) {
		int result = vknvg__packRect(&packer, sizes[i][0], sizes[i][1], &rects[i],
		                              VKNVG_PACK_BEST_AREA_FIT,
		                              VKNVG_SPLIT_SHORTER_AXIS);
		assert(result == 1);
		printf("      Rect %d: %dx%d at (%d,%d)\n",
		       i, sizes[i][0], sizes[i][1], rects[i].x, rects[i].y);
	}

	// Verify no overlaps
	for (int i = 0; i < 10; i++) {
		for (int j = i + 1; j < 10; j++) {
			int overlapX = !(rects[i].x + rects[i].width <= rects[j].x ||
			                 rects[j].x + rects[j].width <= rects[i].x);
			int overlapY = !(rects[i].y + rects[i].height <= rects[j].y ||
			                 rects[j].y + rects[j].height <= rects[i].y);
			assert(!(overlapX && overlapY));	// No overlap
		}
	}

	printf("    No overlaps detected\n");
	printf("    Free rects: %d\n", packer.freeRectCount);
	printf("    Efficiency: %.1f%%\n", 100.0f * vknvg__getPackingEfficiency(&packer));

	printf("  ✓ PASS test_multiple_allocations\n");
	return 1;
}

static int test_fill_atlas(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 256, 256);

	int allocCount = 0;
	VKNVGrect rect;

	// Try to pack many small rectangles
	printf("    Packing 20x20 rectangles until full...\n");
	while (vknvg__packRect(&packer, 20, 20, &rect,
	                        VKNVG_PACK_BEST_AREA_FIT,
	                        VKNVG_SPLIT_SHORTER_AXIS)) {
		allocCount++;
	}

	printf("    Packed %d rectangles before full\n", allocCount);
	printf("    Allocated area: %d / %d\n", packer.allocatedArea, packer.totalArea);
	printf("    Efficiency: %.1f%%\n", 100.0f * vknvg__getPackingEfficiency(&packer));

	// Theoretical maximum: (256/20)^2 = 163.84
	// With guillotine overhead, should achieve at least 140+ (85% efficiency)
	assert(allocCount >= 140);
	assert(vknvg__getPackingEfficiency(&packer) >= 0.85f);

	printf("  ✓ PASS test_fill_atlas\n");
	return 1;
}

static int test_varied_sizes(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 512, 512);

	int allocCount = 0;
	VKNVGrect rect;

	// Mix of small, medium, and large rectangles (simulates real glyph sizes)
	const uint16_t sizes[][2] = {
		{12, 14}, {24, 28}, {10, 16}, {32, 32}, {48, 48},
		{16, 20}, {8, 12}, {64, 64}, {20, 24}, {28, 32}
	};

	printf("    Packing varied sizes (simulating glyphs)...\n");
	for (int i = 0; i < 100; i++) {
		int sizeIdx = i % 10;
		if (vknvg__packRect(&packer, sizes[sizeIdx][0], sizes[sizeIdx][1], &rect,
		                     VKNVG_PACK_BEST_SHORT_SIDE_FIT,
		                     VKNVG_SPLIT_SHORTER_AXIS)) {
			allocCount++;
		}
	}

	printf("    Packed %d / 100 rectangles\n", allocCount);
	printf("    Efficiency: %.1f%%\n", 100.0f * vknvg__getPackingEfficiency(&packer));

	// Should be able to pack most of them
	assert(allocCount >= 80);

	printf("  ✓ PASS test_varied_sizes\n");
	return 1;
}

static int test_reset_packer(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 256, 256);

	VKNVGrect rect;
	vknvg__packRect(&packer, 100, 100, &rect,
	                 VKNVG_PACK_BEST_AREA_FIT,
	                 VKNVG_SPLIT_SHORTER_AXIS);

	assert(packer.allocatedArea > 0);
	assert(packer.allocationCount > 0);

	vknvg__resetAtlasPacker(&packer);

	assert(packer.allocatedArea == 0);
	assert(packer.allocationCount == 0);
	assert(packer.freeRectCount == 1);
	assert(packer.freeRects[0].width == 256);
	assert(packer.freeRects[0].height == 256);

	printf("    Packer reset successfully\n");
	printf("  ✓ PASS test_reset_packer\n");
	return 1;
}

int main(void)
{
	printf("==========================================\n");
	printf("  Atlas Packing Tests\n");
	printf("==========================================\n\n");

	test_init_packer();
	test_single_allocation();
	test_multiple_allocations();
	test_fill_atlas();
	test_varied_sizes();
	test_reset_packer();

	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   6\n");
	printf("  Passed:  6\n");
	printf("========================================\n");
	printf("All tests passed!\n");

	return 0;
}
