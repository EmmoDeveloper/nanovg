// Atlas Defragmentation - Idle-Frame Compaction
//
// Detects fragmented atlases and compacts glyphs during idle frames
// to free up contiguous space.
//
// Strategy:
// - Track fragmentation metrics (free rect count, largest free area)
// - Identify movable glyphs that could be relocated
// - Use compute shader to copy glyph data to new locations
// - Update glyph metadata with new coordinates
// - Progressive defragmentation with time budgets

#ifndef NANOVG_VK_ATLAS_DEFRAG_H
#define NANOVG_VK_ATLAS_DEFRAG_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include "nanovg_vk_atlas_packing.h"
#include "nanovg_vk_multi_atlas.h"
#include "nanovg_vk_compute.h"

#define VKNVG_DEFRAG_TIME_BUDGET_MS 2.0f	// Max 2ms per frame for defrag
#define VKNVG_DEFRAG_THRESHOLD 0.3f			// Defrag if efficiency < 30%
#define VKNVG_MIN_FREE_RECTS_FOR_DEFRAG 50	// Defrag if > 50 free rects

// Defragmentation state
typedef enum VKNVGdefragState {
	VKNVG_DEFRAG_IDLE,				// Not defragmenting
	VKNVG_DEFRAG_ANALYZING,			// Analyzing fragmentation
	VKNVG_DEFRAG_PLANNING,			// Planning glyph moves
	VKNVG_DEFRAG_EXECUTING,			// Executing moves
	VKNVG_DEFRAG_COMPLETE			// Finished
} VKNVGdefragState;

// Glyph relocation record
typedef struct VKNVGglyphMove {
	uint16_t srcX, srcY;		// Source position
	uint16_t dstX, dstY;		// Destination position
	uint16_t width, height;		// Glyph dimensions
	uint32_t glyphId;			// Glyph identifier (for updating metadata)
} VKNVGglyphMove;

// Callback for updating glyph cache after moves
typedef void (*VKNVGdefragUpdateCallback)(void* userData, const VKNVGglyphMove* moves,
                                           uint32_t moveCount, uint32_t atlasIndex);

// Defragmentation context
typedef struct VKNVGdefragContext {
	VKNVGdefragState state;
	uint32_t atlasIndex;		// Atlas being defragmented

	// Move list
	VKNVGglyphMove moves[256];	// Up to 256 moves per pass
	uint32_t moveCount;
	uint32_t currentMove;		// Current move index

	// Time tracking
	float timeBudgetMs;			// Time budget per frame
	float timeSpentMs;			// Time spent this frame

	// Statistics
	uint32_t totalMoves;
	uint32_t bytesCopied;

	// Temporary packer for replanning
	VKNVGatlasPacker newPacker;

	// Callback for updating external glyph cache
	VKNVGdefragUpdateCallback updateCallback;
	void* callbackUserData;

	// Compute shader support
	VKNVGcomputeContext* computeContext;
	VkDescriptorSet atlasDescriptorSet;
	int useCompute;				// Use compute shader instead of vkCmdCopyImage
} VKNVGdefragContext;

// Calculate fragmentation metric for an atlas
// Returns fragmentation score (0.0 = not fragmented, 1.0 = highly fragmented)
static float vknvg__calculateFragmentation(const VKNVGatlasPacker* packer)
{
	if (packer->freeRectCount <= 1) {
		return 0.0f;	// Single free rect = not fragmented
	}

	// Fragmentation based on:
	// 1. Number of free rectangles (more = worse)
	// 2. Average size of free rectangles (smaller = worse)

	float efficiency = vknvg__getPackingEfficiency(packer);
	float freeAreaRatio = 1.0f - efficiency;

	// If mostly allocated, fragmentation doesn't matter much
	if (efficiency > 0.9f) {
		return 0.0f;
	}

	// Normalize free rect count (50 rects = significant fragmentation)
	float rectCountScore = (float)packer->freeRectCount / 50.0f;
	if (rectCountScore > 1.0f) rectCountScore = 1.0f;

	// Combine metrics: more free space + more rects = more fragmented
	float fragmentation = rectCountScore * freeAreaRatio;

	return fragmentation;
}

// Check if atlas should be defragmented
static int vknvg__shouldDefragmentAtlas(const VKNVGatlasInstance* atlas)
{
	// Don't defrag if atlas is mostly full (>90%)
	float efficiency = vknvg__getPackingEfficiency(&atlas->packer);
	if (efficiency > 0.9f) {
		return 0;
	}

	float fragmentation = vknvg__calculateFragmentation(&atlas->packer);

	// Defragment if fragmentation is high OR if too many free rects
	if (fragmentation > VKNVG_DEFRAG_THRESHOLD) {
		return 1;
	}

	if (atlas->packer.freeRectCount > VKNVG_MIN_FREE_RECTS_FOR_DEFRAG) {
		return 1;
	}

	return 0;
}

// Initialize defragmentation context
static void vknvg__initDefragContext(VKNVGdefragContext* ctx,
                                      uint32_t atlasIndex,
                                      float timeBudgetMs)
{
	memset(ctx, 0, sizeof(VKNVGdefragContext));
	ctx->state = VKNVG_DEFRAG_IDLE;
	ctx->atlasIndex = atlasIndex;
	ctx->timeBudgetMs = timeBudgetMs;
}

// Start defragmentation for an atlas
static void vknvg__startDefragmentation(VKNVGdefragContext* ctx,
                                         VKNVGatlasManager* manager)
{
	if (ctx->atlasIndex >= manager->atlasCount) {
		return;
	}

	ctx->state = VKNVG_DEFRAG_ANALYZING;
	ctx->moveCount = 0;
	ctx->currentMove = 0;
	ctx->timeSpentMs = 0.0f;
}

// Analyze and plan glyph moves for defragmentation
// This is a simplified approach that would need integration with actual glyph tracking
static int vknvg__planDefragMoves(VKNVGdefragContext* ctx,
                                   VKNVGatlasManager* manager)
{
	if (ctx->atlasIndex >= manager->atlasCount) {
		return 0;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[ctx->atlasIndex];

	// Create new packer with same dimensions
	vknvg__initAtlasPacker(&ctx->newPacker,
	                       atlas->packer.atlasWidth,
	                       atlas->packer.atlasHeight);

	// In a real implementation, would iterate over all allocated glyphs
	// and repack them optimally. For now, this is a placeholder that
	// demonstrates the structure.

	// Example: Plan to consolidate glyphs to top-left
	// (Real implementation would track actual glyph positions)

	ctx->moveCount = 0;
	ctx->state = VKNVG_DEFRAG_PLANNING;

	return 1;
}

// Execute a single glyph move using compute shader or vkCmdCopyImage
static void vknvg__executeSingleMove(VKNVGdefragContext* ctx,
                                      VkCommandBuffer cmdBuffer,
                                      VkImage atlasImage)
{
	if (ctx->currentMove >= ctx->moveCount) {
		return;
	}

	VKNVGglyphMove* move = &ctx->moves[ctx->currentMove];

	if (ctx->useCompute && ctx->computeContext && ctx->atlasDescriptorSet) {
		// Use compute shader for GPU-accelerated copy
		static int first_compute_move = 1;
		if (first_compute_move) {
			printf("[DEFRAG] Using COMPUTE SHADER path for defragmentation\n");
			printf("[DEFRAG] Move %u: (%u,%u)->(%u,%u) size %ux%u\n",
			       ctx->currentMove, move->srcX, move->srcY,
			       move->dstX, move->dstY, move->width, move->height);
			first_compute_move = 0;
		}

		VKNVGdefragPushConstants pushConstants = {0};
		pushConstants.srcOffsetX = move->srcX;
		pushConstants.srcOffsetY = move->srcY;
		pushConstants.dstOffsetX = move->dstX;
		pushConstants.dstOffsetY = move->dstY;
		pushConstants.extentWidth = move->width;
		pushConstants.extentHeight = move->height;

		vknvg__dispatchDefragCompute(ctx->computeContext,
		                              ctx->atlasDescriptorSet,
		                              &pushConstants);
	} else {
		// Fall back to vkCmdCopyImage (CPU-initiated)
		VkImageCopy copyRegion = {0};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset.x = move->srcX;
		copyRegion.srcOffset.y = move->srcY;

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset.x = move->dstX;
		copyRegion.dstOffset.y = move->dstY;

		copyRegion.extent.width = move->width;
		copyRegion.extent.height = move->height;
		copyRegion.extent.depth = 1;

		// Self-copy within same image
		vkCmdCopyImage(cmdBuffer,
		               atlasImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               atlasImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &copyRegion);
	}

	ctx->bytesCopied += move->width * move->height;
	ctx->currentMove++;
}

// Execute defragmentation moves (time-budgeted)
static int vknvg__executeDefragMoves(VKNVGdefragContext* ctx,
                                      VKNVGatlasManager* manager,
                                      VkCommandBuffer cmdBuffer,
                                      float deltaTimeMs)
{
	if (ctx->state != VKNVG_DEFRAG_EXECUTING) {
		return 0;
	}

	if (ctx->atlasIndex >= manager->atlasCount) {
		return 0;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[ctx->atlasIndex];
	float timeRemaining = ctx->timeBudgetMs - ctx->timeSpentMs;

	// Estimate time per move (simplified)
	float estimatedTimePerMove = 0.1f;	// 0.1ms per move estimate

	// Execute moves within time budget
	while (ctx->currentMove < ctx->moveCount && timeRemaining > estimatedTimePerMove) {
		vknvg__executeSingleMove(ctx, cmdBuffer, atlas->image);

		ctx->timeSpentMs += estimatedTimePerMove;
		timeRemaining -= estimatedTimePerMove;
		ctx->totalMoves++;
	}

	// Check if complete
	if (ctx->currentMove >= ctx->moveCount) {
		ctx->state = VKNVG_DEFRAG_COMPLETE;
		return 1;	// Complete
	}

	return 0;	// More work to do
}

// Complete defragmentation and update packer
static void vknvg__completeDefragmentation(VKNVGdefragContext* ctx,
                                            VKNVGatlasManager* manager)
{
	if (ctx->state != VKNVG_DEFRAG_COMPLETE) {
		return;
	}

	if (ctx->atlasIndex >= manager->atlasCount) {
		return;
	}

	// Update atlas packer with new layout
	VKNVGatlasInstance* atlas = &manager->atlases[ctx->atlasIndex];
	atlas->packer = ctx->newPacker;

	// Call glyph cache update callback if registered
	if (ctx->updateCallback) {
		ctx->updateCallback(ctx->callbackUserData, ctx->moves, ctx->moveCount, ctx->atlasIndex);
	}

	ctx->state = VKNVG_DEFRAG_IDLE;
}

// Single-frame defragmentation update
// Returns 1 if complete, 0 if more work needed
static int vknvg__updateDefragmentation(VKNVGdefragContext* ctx,
                                          VKNVGatlasManager* manager,
                                          VkCommandBuffer cmdBuffer,
                                          float deltaTimeMs)
{
	ctx->timeSpentMs = 0.0f;

	switch (ctx->state) {
		case VKNVG_DEFRAG_IDLE:
			return 1;	// Nothing to do

		case VKNVG_DEFRAG_ANALYZING:
			// Quick analysis
			ctx->state = VKNVG_DEFRAG_PLANNING;
			return 0;

		case VKNVG_DEFRAG_PLANNING:
			// Plan moves
			vknvg__planDefragMoves(ctx, manager);
			ctx->state = VKNVG_DEFRAG_EXECUTING;
			return 0;

		case VKNVG_DEFRAG_EXECUTING:
			// Execute moves with time budget
			if (vknvg__executeDefragMoves(ctx, manager, cmdBuffer, deltaTimeMs)) {
				vknvg__completeDefragmentation(ctx, manager);
				return 1;
			}
			return 0;

		case VKNVG_DEFRAG_COMPLETE:
			ctx->state = VKNVG_DEFRAG_IDLE;
			return 1;
	}

	return 1;
}

// Get defragmentation statistics
static void vknvg__getDefragStats(const VKNVGdefragContext* ctx,
                                   uint32_t* outTotalMoves,
                                   uint32_t* outBytesCopied,
                                   float* outFragmentation)
{
	if (outTotalMoves) *outTotalMoves = ctx->totalMoves;
	if (outBytesCopied) *outBytesCopied = ctx->bytesCopied;
	if (outFragmentation) *outFragmentation = 0.0f;	// Would calculate from current state
}

// Enable compute shader defragmentation
static void vknvg__enableComputeDefrag(VKNVGdefragContext* ctx,
                                        VKNVGcomputeContext* computeContext,
                                        VkDescriptorSet atlasDescriptorSet)
{
	ctx->computeContext = computeContext;
	ctx->atlasDescriptorSet = atlasDescriptorSet;
	ctx->useCompute = 1;
}

// Disable compute shader defragmentation (fall back to vkCmdCopyImage)
static void vknvg__disableComputeDefrag(VKNVGdefragContext* ctx)
{
	ctx->computeContext = NULL;
	ctx->atlasDescriptorSet = VK_NULL_HANDLE;
	ctx->useCompute = 0;
}

#endif // NANOVG_VK_ATLAS_DEFRAG_H
