#include <stdio.h>
#include <assert.h>
#include "../src/nanovg_vk_multi_atlas.h"

// Test next size calculation
static int test_next_size_progression(void)
{
	uint16_t size;

	// Normal progression
	size = vknvg__getNextAtlasSize(512, 4096);
	assert(size == 1024);

	size = vknvg__getNextAtlasSize(1024, 4096);
	assert(size == 2048);

	size = vknvg__getNextAtlasSize(2048, 4096);
	assert(size == 4096);

	// Already at max
	size = vknvg__getNextAtlasSize(4096, 4096);
	assert(size == 0);

	// Over max (shouldn't happen but handle it)
	size = vknvg__getNextAtlasSize(8192, 4096);
	assert(size == 0);

	printf("  ✓ PASS test_next_size_progression\n");
	return 1;
}

// Test resize threshold logic
static int test_resize_threshold(void)
{
	// Create a mock manager (NULL pointers for Vulkan resources)
	VKNVGatlasManager manager = {0};
	manager.enableDynamicGrowth = 1;
	manager.resizeThreshold = 0.85f;
	manager.maxAtlasSize = 4096;
	manager.atlasCount = 1;

	// Create a mock atlas instance
	VKNVGatlasInstance atlas = {0};
	vknvg__initAtlasPacker(&atlas.packer, 512, 512);
	manager.atlases[0] = atlas;

	// Low utilization - should not resize
	atlas.packer.allocatedArea = 100000;	// ~38% of 512x512
	manager.atlases[0] = atlas;
	int shouldResize = vknvg__shouldResizeAtlas(&manager, 0);
	assert(shouldResize == 0);
	printf("    Low utilization (38%%): should not resize ✓\n");

	// High utilization - should resize
	atlas.packer.allocatedArea = 230000;	// ~88% of 512x512
	manager.atlases[0] = atlas;
	shouldResize = vknvg__shouldResizeAtlas(&manager, 0);
	assert(shouldResize == 1);
	printf("    High utilization (88%%): should resize ✓\n");

	// Disabled growth - should not resize
	manager.enableDynamicGrowth = 0;
	shouldResize = vknvg__shouldResizeAtlas(&manager, 0);
	assert(shouldResize == 0);
	printf("    Growth disabled: should not resize ✓\n");

	// Already at max size - should not resize
	manager.enableDynamicGrowth = 1;
	vknvg__initAtlasPacker(&atlas.packer, 4096, 4096);
	atlas.packer.allocatedArea = 15000000;	// ~90% of 4096x4096
	manager.atlases[0] = atlas;
	shouldResize = vknvg__shouldResizeAtlas(&manager, 0);
	assert(shouldResize == 0);
	printf("    At max size (4096x4096): should not resize ✓\n");

	printf("  ✓ PASS test_resize_threshold\n");
	return 1;
}

// Test size progression with custom limits
static int test_custom_size_limits(void)
{
	uint16_t size;

	// Custom max of 2048
	size = vknvg__getNextAtlasSize(512, 2048);
	assert(size == 1024);

	size = vknvg__getNextAtlasSize(1024, 2048);
	assert(size == 2048);

	size = vknvg__getNextAtlasSize(2048, 2048);
	assert(size == 0);

	// Starting larger
	size = vknvg__getNextAtlasSize(1024, 4096);
	assert(size == 2048);

	printf("  ✓ PASS test_custom_size_limits\n");
	return 1;
}

// Test configuration defaults
static int test_resize_config(void)
{
	// Verify default configuration values
	assert(VKNVG_MIN_ATLAS_SIZE == 512);
	assert(VKNVG_DEFAULT_ATLAS_SIZE == 4096);
	assert(VKNVG_RESIZE_THRESHOLD == 0.85f);

	printf("  ✓ PASS test_resize_config\n");
	return 1;
}

// Test utilization calculation at different sizes
static int test_utilization_at_sizes(void)
{
	VKNVGatlasPacker packer;

	// 512x512 atlas
	vknvg__initAtlasPacker(&packer, 512, 512);
	uint32_t totalArea512 = packer.totalArea;
	assert(totalArea512 == 512 * 512);

	packer.allocatedArea = (uint32_t)(totalArea512 * 0.9f);
	float efficiency = vknvg__getPackingEfficiency(&packer);
	assert(efficiency >= 0.89f && efficiency <= 0.91f);
	printf("    512x512 at 90%% allocated: efficiency = %.2f%% ✓\n", efficiency * 100);

	// 2048x2048 atlas
	vknvg__initAtlasPacker(&packer, 2048, 2048);
	uint32_t totalArea2048 = packer.totalArea;
	assert(totalArea2048 == 2048 * 2048);

	packer.allocatedArea = (uint32_t)(totalArea2048 * 0.5f);
	efficiency = vknvg__getPackingEfficiency(&packer);
	assert(efficiency >= 0.49f && efficiency <= 0.51f);
	printf("    2048x2048 at 50%% allocated: efficiency = %.2f%% ✓\n", efficiency * 100);

	printf("  ✓ PASS test_utilization_at_sizes\n");
	return 1;
}

int main(void)
{
	printf("==========================================\n");
	printf("  Atlas Resize Tests\n");
	printf("==========================================\n\n");

	test_next_size_progression();
	test_resize_threshold();
	test_custom_size_limits();
	test_resize_config();
	test_utilization_at_sizes();

	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   5\n");
	printf("  Passed:  5\n");
	printf("========================================\n");
	printf("All tests passed!\n");

	printf("\nNote: This test validates resize logic.\n");
	printf("Full Vulkan resize tests require a graphics context.\n");

	return 0;
}
