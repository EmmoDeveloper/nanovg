#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include "../src/nanovg_vk_virtual_atlas.h"
#include "window_utils.h"

// Full compute shader defragmentation test with measurements

// Helper to get current time in microseconds
static uint64_t get_time_us(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

// Helper to create fragmentation
static void create_fragmentation(VKNVGvirtualAtlas* atlas, uint32_t* glyphCount)
{
	printf("Creating fragmentation scenario...\n");

	// Phase 1: Add many small glyphs (16x16)
	for (uint32_t i = 0; i < 100; i++) {
		VKNVGglyphKey key = {0};
		key.fontID = 1;
		key.codepoint = 0x4E00 + i;
		key.size = 16 << 16;

		uint8_t* pixelData = (uint8_t*)malloc(16 * 16);
		if (pixelData) {
			// Fill with pattern
			for (int j = 0; j < 16 * 16; j++) {
				pixelData[j] = (uint8_t)(i + j);
			}

			VKNVGglyphCacheEntry* entry = vknvg__addGlyphDirect(atlas, key, pixelData,
			                                                     16, 16, 0, 16, 16);
			if (entry) {
				(*glyphCount)++;
			}
		}
	}

	// Phase 2: Add medium glyphs (32x32)
	for (uint32_t i = 0; i < 50; i++) {
		VKNVGglyphKey key = {0};
		key.fontID = 1;
		key.codepoint = 0x5000 + i;
		key.size = 32 << 16;

		uint8_t* pixelData = (uint8_t*)malloc(32 * 32);
		if (pixelData) {
			for (int j = 0; j < 32 * 32; j++) {
				pixelData[j] = (uint8_t)(i * 2 + j);
			}

			VKNVGglyphCacheEntry* entry = vknvg__addGlyphDirect(atlas, key, pixelData,
			                                                     32, 32, 0, 32, 32);
			if (entry) {
				(*glyphCount)++;
			}
		}
	}

	// Phase 3: Add large glyphs (64x64)
	for (uint32_t i = 0; i < 30; i++) {
		VKNVGglyphKey key = {0};
		key.fontID = 1;
		key.codepoint = 0x6000 + i;
		key.size = 64 << 16;

		uint8_t* pixelData = (uint8_t*)malloc(64 * 64);
		if (pixelData) {
			for (int j = 0; j < 64 * 64; j++) {
				pixelData[j] = (uint8_t)(i * 3 + j);
			}

			VKNVGglyphCacheEntry* entry = vknvg__addGlyphDirect(atlas, key, pixelData,
			                                                     64, 64, 0, 64, 64);
			if (entry) {
				(*glyphCount)++;
			}
		}
	}

	printf("Added %u glyphs (100 small, 50 medium, 30 large)\n", *glyphCount);
}

// Helper to print atlas packing stats
static void print_packing_stats(VKNVGvirtualAtlas* atlas)
{
	if (atlas->atlasManager && atlas->atlasManager->atlasCount > 0) {
		VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[0];
		VKNVGatlasPacker* packer = &atlasInst->packer;

		float efficiency = vknvg__getPackingEfficiency(packer);
		float fragmentation = vknvg__calculateFragmentation(packer);

		printf("  Atlas 0: %ux%u\n", packer->atlasWidth, packer->atlasHeight);
		printf("  Packing efficiency: %.1f%%\n", efficiency * 100.0f);
		printf("  Fragmentation: %.1f%%\n", fragmentation * 100.0f);
		printf("  Free rectangles: %u\n", packer->freeRectCount);
		printf("  Should defrag: %s\n",
		       vknvg__shouldDefragmentAtlas(atlasInst) ? "YES" : "NO");
	}
}

int main(void)
{
	printf("========================================\n");
	printf("Full Compute Shader Defragmentation Test\n");
	printf("========================================\n\n");

	// Create window context
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Compute Defrag Full Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("✓ Vulkan context created\n");
	printf("  Device: %p\n", (void*)winCtx->device);
	printf("  Queue: %p (family %u)\n\n", (void*)winCtx->graphicsQueue,
	       winCtx->graphicsQueueFamilyIndex);

	// Create virtual atlas
	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(winCtx->device,
	                                                      winCtx->physicalDevice,
	                                                      NULL, NULL);
	if (!atlas) {
		fprintf(stderr, "Failed to create virtual atlas\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Virtual atlas created\n\n");

	// Enable compute defragmentation
	uint64_t t0 = get_time_us();
	VkResult result = vknvg__enableComputeDefragmentation(atlas,
	                                                       winCtx->graphicsQueue,
	                                                       winCtx->graphicsQueueFamilyIndex);
	uint64_t t1 = get_time_us();

	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to enable compute defragmentation: %d\n", result);
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("✓ Compute shader defragmentation enabled\n");
	printf("  Initialization time: %lu μs\n", (unsigned long)(t1 - t0));
	printf("  Compute context: %p\n", (void*)atlas->computeContext);
	printf("  Pipeline: %p\n", (void*)atlas->computeContext->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG].pipeline);
	printf("  Descriptor pool: %p\n\n",
	       (void*)atlas->computeContext->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG].descriptorPool);

	// Test 1: Create fragmentation
	printf("========================================\n");
	printf("Test 1: Create Fragmentation Scenario\n");
	printf("========================================\n");

	uint32_t glyphCount = 0;
	t0 = get_time_us();
	create_fragmentation(atlas, &glyphCount);
	t1 = get_time_us();

	printf("Time to add glyphs: %lu μs\n", (unsigned long)(t1 - t0));
	printf("Average per glyph: %lu μs\n\n", (unsigned long)((t1 - t0) / glyphCount));

	// Create command buffer
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = winCtx->graphicsQueueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool commandPool;
	if (vkCreateCommandPool(winCtx->device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}

	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	if (vkAllocateCommandBuffers(winCtx->device, &allocInfo, &cmd) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate command buffer\n");
		vkDestroyCommandPool(winCtx->device, commandPool, NULL);
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}

	// Test 2: Process uploads
	printf("========================================\n");
	printf("Test 2: Process GPU Uploads\n");
	printf("========================================\n");

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &beginInfo);

	t0 = get_time_us();
	vknvg__processUploads(atlas, cmd);
	t1 = get_time_us();

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	uint64_t t2 = get_time_us();

	printf("Upload command recording: %lu μs\n", (unsigned long)(t1 - t0));
	printf("GPU execution time: %lu μs\n", (unsigned long)(t2 - t1));
	printf("Total upload time: %lu μs\n\n", (unsigned long)(t2 - t0));

	// Test 3: Check atlas state
	printf("========================================\n");
	printf("Test 3: Atlas State After Uploads\n");
	printf("========================================\n");
	print_packing_stats(atlas);
	printf("\n");

	// Test 4: Manually trigger defragmentation
	printf("========================================\n");
	printf("Test 4: Trigger Defragmentation\n");
	printf("========================================\n");

	// Check if defrag should run
	if (atlas->atlasManager && atlas->atlasManager->atlasCount > 0) {
		VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[0];

		if (vknvg__shouldDefragmentAtlas(atlasInst)) {
			printf("Atlas is fragmented, starting defragmentation...\n\n");

			// Initialize defrag context
			vknvg__initDefragContext(&atlas->defragContext, 0, 2.0f);
			vknvg__startDefragmentation(&atlas->defragContext, atlas->atlasManager);

			// Plan moves
			vknvg__planDefragMoves(&atlas->defragContext, atlas->atlasManager);
			printf("Planned %u glyph moves\n", atlas->defragContext.moveCount);

			if (atlas->defragContext.moveCount > 0) {
				atlas->defragContext.state = VKNVG_DEFRAG_EXECUTING;

				printf("Defrag context state:\n");
				printf("  state: %d (EXECUTING=%d)\n", atlas->defragContext.state, VKNVG_DEFRAG_EXECUTING);
				printf("  moveCount: %u\n", atlas->defragContext.moveCount);
				printf("  atlasIndex: %u\n", atlas->defragContext.atlasIndex);
				printf("  Atlas compute enabled: %d\n", atlas->useComputeDefrag);
				printf("  Atlas compute context: %p\n\n", (void*)atlas->computeContext);

				// Execute defragmentation moves
				vkResetCommandBuffer(cmd, 0);
				vkBeginCommandBuffer(cmd, &beginInfo);

				// Transition to GENERAL layout for compute
				VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[0];
				VkImageMemoryBarrier barrier = {0};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = atlasInst->image;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

				vkCmdPipelineBarrier(cmd,
				                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				                     0, 0, NULL, 0, NULL, 1, &barrier);

				printf("Executing %u defragmentation moves...\n", atlas->defragContext.moveCount);

				t0 = get_time_us();

				// Execute all moves
				uint32_t computeMoves = 0;
				uint32_t fallbackMoves = 0;

				for (uint32_t i = 0; i < atlas->defragContext.moveCount; i++) {
					atlas->defragContext.currentMove = i;
					vknvg__executeSingleMove(&atlas->defragContext, cmd, atlasInst->image);

					if (atlas->defragContext.useCompute) {
						computeMoves++;
					} else {
						fallbackMoves++;
					}
				}

				t1 = get_time_us();

				// Transition back to SHADER_READ_ONLY
				barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmd,
				                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				                     0, 0, NULL, 0, NULL, 1, &barrier);

				vkEndCommandBuffer(cmd);

				// Submit and wait
				vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(winCtx->graphicsQueue);

				t2 = get_time_us();

				printf("\n");
				printf("Defragmentation Results:\n");
				printf("  Moves via compute shader: %u\n", computeMoves);
				printf("  Moves via fallback (vkCmdCopyImage): %u\n", fallbackMoves);
				printf("  Command recording time: %lu μs\n", (unsigned long)(t1 - t0));
				printf("  GPU execution time: %lu μs\n", (unsigned long)(t2 - t1));
				printf("  Total defrag time: %lu μs\n", (unsigned long)(t2 - t0));
				printf("  Average per move: %lu μs\n",
				       (unsigned long)((t2 - t0) / atlas->defragContext.moveCount));
				printf("  Bytes copied: %u\n", atlas->defragContext.bytesCopied);

				if (computeMoves > 0) {
					printf("\n✓ COMPUTE SHADER PATH EXECUTED\n");
				} else {
					printf("\n⚠ Fallback path used (compute shader not executed)\n");
				}
			} else {
				printf("No moves planned (atlas already optimal)\n");
			}
		} else {
			printf("Atlas not fragmented enough to trigger defragmentation\n");
			printf("(fragmentation threshold not met)\n");
		}
	}

	printf("\n");

	// Test 5: Final statistics
	printf("========================================\n");
	printf("Test 5: Final Statistics\n");
	printf("========================================\n");

	uint32_t cacheHits, cacheMisses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &cacheHits, &cacheMisses, &evictions, &uploads);

	printf("Cache statistics:\n");
	printf("  Hits: %u\n", cacheHits);
	printf("  Misses: %u\n", cacheMisses);
	printf("  Evictions: %u\n", evictions);
	printf("  Uploads: %u\n", uploads);
	printf("  Hit rate: %.1f%%\n",
	       cacheHits + cacheMisses > 0 ?
	       (100.0f * cacheHits / (cacheHits + cacheMisses)) : 0.0f);

	printf("\nDefragmentation statistics:\n");
	uint32_t totalMoves, bytesCopied;
	float fragmentation;
	vknvg__getDefragStats(&atlas->defragContext, &totalMoves, &bytesCopied, &fragmentation);
	printf("  Total moves: %u\n", totalMoves);
	printf("  Bytes copied: %u\n", bytesCopied);
	printf("  Final fragmentation: %.1f%%\n", fragmentation * 100.0f);

	printf("\nFinal atlas state:\n");
	print_packing_stats(atlas);

	// Cleanup
	vkFreeCommandBuffers(winCtx->device, commandPool, 1, &cmd);
	vkDestroyCommandPool(winCtx->device, commandPool, NULL);
	vknvg__destroyVirtualAtlas(atlas);
	window_destroy_context(winCtx);

	printf("\n========================================\n");
	printf("All Tests Completed Successfully\n");
	printf("========================================\n");
	return 0;
}
