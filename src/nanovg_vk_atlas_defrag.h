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

// Function declarations

// Calculate fragmentation metric for an atlas
// Returns fragmentation score (0.0 = not fragmented, 1.0 = highly fragmented)
float vknvg__calculateFragmentation(const VKNVGatlasPacker* packer);

// Check if atlas should be defragmented
int vknvg__shouldDefragmentAtlas(const VKNVGatlasInstance* atlas);

// Initialize defragmentation context
void vknvg__initDefragContext(VKNVGdefragContext* ctx,
                                      uint32_t atlasIndex,
                                      float timeBudgetMs);

// Start defragmentation for an atlas
void vknvg__startDefragmentation(VKNVGdefragContext* ctx,
                                         VKNVGatlasManager* manager);

// Analyze and plan glyph moves for defragmentation
// This is a simplified approach that would need integration with actual glyph tracking
int vknvg__planDefragMoves(VKNVGdefragContext* ctx,
                                   VKNVGatlasManager* manager);

// Execute a single glyph move using compute shader or vkCmdCopyImage
void vknvg__executeSingleMove(VKNVGdefragContext* ctx,
                                      VkCommandBuffer cmdBuffer,
                                      VkImage atlasImage);

// Execute defragmentation moves (time-budgeted)
int vknvg__executeDefragMoves(VKNVGdefragContext* ctx,
                                      VKNVGatlasManager* manager,
                                      VkCommandBuffer cmdBuffer,
                                      float deltaTimeMs);

// Complete defragmentation and update packer
void vknvg__completeDefragmentation(VKNVGdefragContext* ctx,
                                            VKNVGatlasManager* manager);

// Single-frame defragmentation update
// Returns 1 if complete, 0 if more work needed
int vknvg__updateDefragmentation(VKNVGdefragContext* ctx,
                                          VKNVGatlasManager* manager,
                                          VkCommandBuffer cmdBuffer,
                                          float deltaTimeMs);

// Get defragmentation statistics
void vknvg__getDefragStats(const VKNVGdefragContext* ctx,
                                   uint32_t* outTotalMoves,
                                   uint32_t* outBytesCopied,
                                   float* outFragmentation);

// Enable compute shader defragmentation
void vknvg__enableComputeDefrag(VKNVGdefragContext* ctx,
                                        VKNVGcomputeContext* computeContext,
                                        VkDescriptorSet atlasDescriptorSet);

// Disable compute shader defragmentation (fall back to vkCmdCopyImage)
void vknvg__disableComputeDefrag(VKNVGdefragContext* ctx);

#endif // NANOVG_VK_ATLAS_DEFRAG_H
