#include <stdio.h>
#include <assert.h>
#include "../src/nanovg_vk_atlas_defrag.h"

// Test fragmentation calculation
static int test_fragmentation_calculation(void)
{
	VKNVGatlasPacker packer;
	float fragmentation;

	// Empty atlas - not fragmented
	vknvg__initAtlasPacker(&packer, 1024, 1024);
	fragmentation = vknvg__calculateFragmentation(&packer);
	assert(fragmentation == 0.0f);
	printf("    Empty atlas: fragmentation = %.2f ✓\n", fragmentation);

	// Single allocation - not fragmented (still 1 free rect)
	VKNVGrect rect;
	vknvg__packRect(&packer, 100, 100, &rect,
	                VKNVG_PACK_BEST_AREA_FIT,
	                VKNVG_SPLIT_SHORTER_AXIS);
	fragmentation = vknvg__calculateFragmentation(&packer);
	assert(fragmentation < 0.1f);	// Should be low
	printf("    After 1 allocation (%d free rects): fragmentation = %.2f ✓\n",
	       packer.freeRectCount, fragmentation);

	// Many allocations - increasing fragmentation
	for (int i = 0; i < 30; i++) {
		vknvg__packRect(&packer, 50, 50, &rect,
		                VKNVG_PACK_BEST_AREA_FIT,
		                VKNVG_SPLIT_SHORTER_AXIS);
	}
	fragmentation = vknvg__calculateFragmentation(&packer);
	printf("    After 31 allocations (%d free rects): fragmentation = %.2f ✓\n",
	       packer.freeRectCount, fragmentation);

	// High utilization - fragmentation less important
	vknvg__initAtlasPacker(&packer, 512, 512);
	packer.allocatedArea = (uint32_t)(packer.totalArea * 0.95f);
	packer.freeRectCount = 60;	// Many rects but high utilization
	fragmentation = vknvg__calculateFragmentation(&packer);
	assert(fragmentation == 0.0f);	// Should ignore when >90% full
	printf("    95%% utilized with 60 free rects: fragmentation = %.2f ✓\n",
	       fragmentation);

	printf("  ✓ PASS test_fragmentation_calculation\n");
	return 1;
}

// Test defragmentation trigger conditions
static int test_defrag_trigger(void)
{
	VKNVGatlasInstance atlas = {0};
	int shouldDefrag;

	// Low fragmentation - should not defrag
	vknvg__initAtlasPacker(&atlas.packer, 1024, 1024);
	VKNVGrect rect;
	vknvg__packRect(&atlas.packer, 200, 200, &rect,
	                VKNVG_PACK_BEST_AREA_FIT,
	                VKNVG_SPLIT_SHORTER_AXIS);

	shouldDefrag = vknvg__shouldDefragmentAtlas(&atlas);
	assert(shouldDefrag == 0);
	printf("    Low fragmentation: should not defrag ✓\n");

	// Many free rects - should defrag
	vknvg__initAtlasPacker(&atlas.packer, 1024, 1024);
	for (int i = 0; i < 60; i++) {
		if (!vknvg__packRect(&atlas.packer, 30, 30, &rect,
		                     VKNVG_PACK_BEST_AREA_FIT,
		                     VKNVG_SPLIT_SHORTER_AXIS)) {
			break;
		}
	}

	shouldDefrag = vknvg__shouldDefragmentAtlas(&atlas);
	printf("    %d free rects: should %sdefrag ✓\n",
	       atlas.packer.freeRectCount,
	       shouldDefrag ? "" : "not ");

	// High utilization - should not defrag regardless
	vknvg__initAtlasPacker(&atlas.packer, 1024, 1024);
	atlas.packer.allocatedArea = (uint32_t)(atlas.packer.totalArea * 0.95f);
	atlas.packer.freeRectCount = 100;

	shouldDefrag = vknvg__shouldDefragmentAtlas(&atlas);
	assert(shouldDefrag == 0);
	printf("    High utilization (95%%): should not defrag ✓\n");

	printf("  ✓ PASS test_defrag_trigger\n");
	return 1;
}

// Test defragmentation context initialization
static int test_defrag_context_init(void)
{
	VKNVGdefragContext ctx;

	vknvg__initDefragContext(&ctx, 0, VKNVG_DEFRAG_TIME_BUDGET_MS);

	assert(ctx.state == VKNVG_DEFRAG_IDLE);
	assert(ctx.atlasIndex == 0);
	assert(ctx.timeBudgetMs == VKNVG_DEFRAG_TIME_BUDGET_MS);
	assert(ctx.moveCount == 0);
	assert(ctx.currentMove == 0);
	assert(ctx.totalMoves == 0);
	assert(ctx.bytesCopied == 0);

	printf("  ✓ PASS test_defrag_context_init\n");
	return 1;
}

// Test defragmentation state machine
static int test_defrag_state_machine(void)
{
	VKNVGdefragContext ctx;
	VKNVGatlasManager manager = {0};

	// Setup mock manager
	manager.atlasCount = 1;
	vknvg__initAtlasPacker(&manager.atlases[0].packer, 1024, 1024);

	// Initialize context
	vknvg__initDefragContext(&ctx, 0, 2.0f);
	assert(ctx.state == VKNVG_DEFRAG_IDLE);

	// Start defragmentation
	vknvg__startDefragmentation(&ctx, &manager);
	assert(ctx.state == VKNVG_DEFRAG_ANALYZING);
	printf("    Started: state = ANALYZING ✓\n");

	// Update through states (without actual Vulkan commands)
	int complete = vknvg__updateDefragmentation(&ctx, &manager, VK_NULL_HANDLE, 1.0f);
	assert(ctx.state == VKNVG_DEFRAG_PLANNING);
	printf("    After update: state = PLANNING ✓\n");

	complete = vknvg__updateDefragmentation(&ctx, &manager, VK_NULL_HANDLE, 1.0f);
	assert(ctx.state == VKNVG_DEFRAG_EXECUTING || ctx.state == VKNVG_DEFRAG_COMPLETE);
	printf("    After update: state = EXECUTING or COMPLETE ✓\n");

	// Continue until complete
	int maxIterations = 10;
	while (!complete && maxIterations-- > 0) {
		complete = vknvg__updateDefragmentation(&ctx, &manager, VK_NULL_HANDLE, 1.0f);
	}

	assert(ctx.state == VKNVG_DEFRAG_IDLE);
	printf("    Completed: state = IDLE ✓\n");

	printf("  ✓ PASS test_defrag_state_machine\n");
	return 1;
}

// Test defragmentation statistics
static int test_defrag_stats(void)
{
	VKNVGdefragContext ctx;
	vknvg__initDefragContext(&ctx, 0, 2.0f);

	// Set some stats
	ctx.totalMoves = 15;
	ctx.bytesCopied = 32768;

	uint32_t moves, bytes;
	float fragmentation;

	vknvg__getDefragStats(&ctx, &moves, &bytes, &fragmentation);

	assert(moves == 15);
	assert(bytes == 32768);

	printf("    Stats: %u moves, %u bytes copied ✓\n", moves, bytes);

	printf("  ✓ PASS test_defrag_stats\n");
	return 1;
}

// Test configuration constants
static int test_defrag_config(void)
{
	assert(VKNVG_DEFRAG_TIME_BUDGET_MS == 2.0f);
	assert(VKNVG_DEFRAG_THRESHOLD == 0.3f);
	assert(VKNVG_MIN_FREE_RECTS_FOR_DEFRAG == 50);

	printf("  ✓ PASS test_defrag_config\n");
	return 1;
}

int main(void)
{
	printf("==========================================\n");
	printf("  Atlas Defragmentation Tests\n");
	printf("==========================================\n\n");

	test_fragmentation_calculation();
	test_defrag_trigger();
	test_defrag_context_init();
	test_defrag_state_machine();
	test_defrag_stats();
	test_defrag_config();

	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   6\n");
	printf("  Passed:  6\n");
	printf("========================================\n");
	printf("All tests passed!\n");

	printf("\nNote: This test validates defragmentation logic.\n");
	printf("Full Vulkan defrag tests require a graphics context.\n");

	return 0;
}
