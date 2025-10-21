#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "../src/nanovg_vk_multi_atlas.h"

// Mock Vulkan objects for testing
static VkDevice mockDevice = (VkDevice)0x1;
static VkPhysicalDevice mockPhysicalDevice = (VkPhysicalDevice)0x2;
static VkDescriptorPool mockDescriptorPool = (VkDescriptorPool)0x3;
static VkDescriptorSetLayout mockDescriptorSetLayout = (VkDescriptorSetLayout)0x4;
static VkSampler mockSampler = (VkSampler)0x5;

// Mock memory type that always returns 0
static uint32_t mockFindMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties)
{
	(void)physicalDevice;
	(void)typeFilter;
	(void)properties;
	return 0;
}

static int test_packer_init(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 512, 512);

	assert(packer.atlasWidth == 512);
	assert(packer.atlasHeight == 512);
	assert(packer.freeRectCount == 1);
	assert(packer.allocatedArea == 0);

	printf("  ✓ PASS test_packer_init\n");
	return 1;
}

static int test_multi_atlas_allocation(void)
{
	// Test allocation logic without actual Vulkan resources
	VKNVGatlasPacker packers[3];

	for (int i = 0; i < 3; i++) {
		vknvg__initAtlasPacker(&packers[i], 256, 256);
	}

	// Simulate allocations
	VKNVGrect rect;
	int success = 0;

	// Should fit in first atlas
	success = vknvg__packRect(&packers[0], 100, 100, &rect,
	                          VKNVG_PACK_BEST_AREA_FIT,
	                          VKNVG_SPLIT_SHORTER_AXIS);
	assert(success == 1);
	assert(rect.x == 0 && rect.y == 0);

	printf("    First allocation: 100x100 at (%d,%d) in atlas 0\n", rect.x, rect.y);

	// Fill first atlas
	int allocCount = 0;
	while (vknvg__packRect(&packers[0], 50, 50, &rect,
	                        VKNVG_PACK_BEST_AREA_FIT,
	                        VKNVG_SPLIT_SHORTER_AXIS)) {
		allocCount++;
	}

	printf("    Filled atlas 0 with %d additional 50x50 rects\n", allocCount);

	// Should fail on full atlas
	success = vknvg__packRect(&packers[0], 100, 100, &rect,
	                          VKNVG_PACK_BEST_AREA_FIT,
	                          VKNVG_SPLIT_SHORTER_AXIS);
	assert(success == 0);

	// Should succeed in second atlas
	success = vknvg__packRect(&packers[1], 100, 100, &rect,
	                          VKNVG_PACK_BEST_AREA_FIT,
	                          VKNVG_SPLIT_SHORTER_AXIS);
	assert(success == 1);
	assert(rect.x == 0 && rect.y == 0);

	printf("    Allocation in atlas 1: 100x100 at (%d,%d)\n", rect.x, rect.y);

	printf("    Atlas 0 efficiency: %.1f%%\n", 100.0f * vknvg__getPackingEfficiency(&packers[0]));
	printf("    Atlas 1 efficiency: %.1f%%\n", 100.0f * vknvg__getPackingEfficiency(&packers[1]));

	printf("  ✓ PASS test_multi_atlas_allocation\n");
	return 1;
}

static int test_atlas_overflow(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 128, 128);

	// Try to allocate something too large
	VKNVGrect rect;
	int success = vknvg__packRect(&packer, 200, 200, &rect,
	                               VKNVG_PACK_BEST_AREA_FIT,
	                               VKNVG_SPLIT_SHORTER_AXIS);

	assert(success == 0);	// Should fail
	printf("    Large allocation correctly failed (200x200 in 128x128 atlas)\n");

	// Allocate maximum size
	success = vknvg__packRect(&packer, 128, 128, &rect,
	                          VKNVG_PACK_BEST_AREA_FIT,
	                          VKNVG_SPLIT_SHORTER_AXIS);

	assert(success == 1);
	assert(rect.x == 0 && rect.y == 0);
	assert(rect.width == 128 && rect.height == 128);

	printf("    Maximum size allocation succeeded (128x128)\n");

	printf("  ✓ PASS test_atlas_overflow\n");
	return 1;
}

static int test_packing_heuristics(void)
{
	VKNVGatlasPacker packer;
	VKNVGrect rect;

	// Test BEST_SHORT_SIDE_FIT
	vknvg__initAtlasPacker(&packer, 512, 512);
	vknvg__packRect(&packer, 100, 200, &rect,
	                VKNVG_PACK_BEST_SHORT_SIDE_FIT,
	                VKNVG_SPLIT_SHORTER_AXIS);
	printf("    BEST_SHORT_SIDE_FIT: 100x200 at (%d,%d), %d free rects\n",
	       rect.x, rect.y, packer.freeRectCount);

	// Test BEST_AREA_FIT
	vknvg__initAtlasPacker(&packer, 512, 512);
	vknvg__packRect(&packer, 100, 200, &rect,
	                VKNVG_PACK_BEST_AREA_FIT,
	                VKNVG_SPLIT_SHORTER_AXIS);
	printf("    BEST_AREA_FIT: 100x200 at (%d,%d), %d free rects\n",
	       rect.x, rect.y, packer.freeRectCount);

	// Test BOTTOM_LEFT
	vknvg__initAtlasPacker(&packer, 512, 512);
	vknvg__packRect(&packer, 100, 200, &rect,
	                VKNVG_PACK_BOTTOM_LEFT,
	                VKNVG_SPLIT_SHORTER_AXIS);
	printf("    BOTTOM_LEFT: 100x200 at (%d,%d), %d free rects\n",
	       rect.x, rect.y, packer.freeRectCount);

	assert(rect.x == 0 && rect.y == 0);	// Should always be at origin

	printf("  ✓ PASS test_packing_heuristics\n");
	return 1;
}

static int test_fragmentation_analysis(void)
{
	VKNVGatlasPacker packer;
	vknvg__initAtlasPacker(&packer, 512, 512);

	VKNVGrect rect;
	int initialFreeRects = packer.freeRectCount;

	// Make several allocations
	vknvg__packRect(&packer, 100, 100, &rect, VKNVG_PACK_BEST_AREA_FIT, VKNVG_SPLIT_SHORTER_AXIS);
	vknvg__packRect(&packer, 50, 150, &rect, VKNVG_PACK_BEST_AREA_FIT, VKNVG_SPLIT_SHORTER_AXIS);
	vknvg__packRect(&packer, 200, 50, &rect, VKNVG_PACK_BEST_AREA_FIT, VKNVG_SPLIT_SHORTER_AXIS);

	printf("    Initial free rects: %d\n", initialFreeRects);
	printf("    After 3 allocations: %d free rects\n", packer.freeRectCount);
	printf("    Allocated area: %d / %d (%.1f%%)\n",
	       packer.allocatedArea, packer.totalArea,
	       100.0f * vknvg__getPackingEfficiency(&packer));

	// More allocations increase fragmentation
	assert(packer.freeRectCount > initialFreeRects);

	printf("  ✓ PASS test_fragmentation_analysis\n");
	return 1;
}

int main(void)
{
	printf("==========================================\n");
	printf("  Multi-Atlas Tests\n");
	printf("==========================================\n\n");

	test_packer_init();
	test_multi_atlas_allocation();
	test_atlas_overflow();
	test_packing_heuristics();
	test_fragmentation_analysis();

	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   5\n");
	printf("  Passed:  5\n");
	printf("========================================\n");
	printf("All tests passed!\n");

	printf("\nNote: This test validates the packing algorithm.\n");
	printf("Full Vulkan integration tests require a Vulkan instance.\n");

	return 0;
}
