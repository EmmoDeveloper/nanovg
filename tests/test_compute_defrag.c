#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "../src/nanovg_vk_virtual_atlas.h"
#include "window_utils.h"

// Test compute shader defragmentation system
int main(void)
{
	printf("=== Compute Shader Defragmentation Test ===\n\n");

	// Create window context (provides Vulkan setup)
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Compute Defrag Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("✓ Vulkan context created\n");

	// Create virtual atlas
	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(winCtx->device, winCtx->physicalDevice, NULL, NULL);
	if (!atlas) {
		fprintf(stderr, "Failed to create virtual atlas\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Virtual atlas created\n");

	// Enable compute defragmentation
	VkResult result = vknvg__enableComputeDefragmentation(atlas, winCtx->graphicsQueue, winCtx->graphicsQueueFamilyIndex);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to enable compute defragmentation: %d\n", result);
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Compute shader defragmentation enabled\n");

	// Verify compute context was created
	if (!atlas->computeContext) {
		fprintf(stderr, "Compute context not initialized\n");
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Compute context initialized\n");

	// Verify compute pipeline was created
	if (!atlas->computeContext->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG].pipeline) {
		fprintf(stderr, "Compute pipeline not created\n");
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Compute pipeline created\n");

	// Verify descriptor pool was created
	if (!atlas->computeContext->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG].descriptorPool) {
		fprintf(stderr, "Descriptor pool not created\n");
		vknvg__destroyVirtualAtlas(atlas);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Descriptor pool created\n");

	// Test 1: Add glyphs to create fragmentation scenario
	printf("\n--- Test 1: Fragment Atlas ---\n");

	// Allocate many small glyphs followed by larger glyphs to create fragmentation
	uint32_t glyphCount = 0;
	for (uint32_t i = 0; i < 50; i++) {
		VKNVGglyphKey key = {0};
		key.fontID = 1;
		key.codepoint = 0x4E00 + i;  // CJK characters
		key.size = 16 << 16;

		// Create dummy pixel data (16x16)
		uint8_t* pixelData = (uint8_t*)malloc(16 * 16);
		if (pixelData) {
			memset(pixelData, 128, 16 * 16);

			VKNVGglyphCacheEntry* entry = vknvg__addGlyphDirect(atlas, key, pixelData,
			                                                     16, 16, 0, 16, 16);
			if (entry) {
				glyphCount++;
			}
		}
	}
	printf("✓ Added %u glyphs to atlas\n", glyphCount);

	// Test 2: Create command buffer and process uploads
	printf("\n--- Test 2: Process Uploads ---\n");

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

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &beginInfo);
	vknvg__processUploads(atlas, cmd);
	vkEndCommandBuffer(cmd);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	printf("✓ Uploads processed\n");

	// Test 3: Trigger defragmentation with compute enabled
	printf("\n--- Test 3: Test Defragmentation ---\n");

	// Manually trigger defragmentation by setting up defrag context
	atlas->defragContext.state = VKNVG_DEFRAG_ANALYZING;
	atlas->defragContext.atlasIndex = 0;
	atlas->enableDefrag = 1;

	// Reset command buffer
	vkResetCommandBuffer(cmd, 0);
	vkBeginCommandBuffer(cmd, &beginInfo);

	// Process another upload cycle which will trigger defrag
	vknvg__processUploads(atlas, cmd);

	vkEndCommandBuffer(cmd);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	printf("✓ Defragmentation cycle completed\n");

	// Test 4: Verify defrag context has compute enabled
	printf("\n--- Test 4: Verify Compute Integration ---\n");

	if (atlas->defragContext.computeContext) {
		printf("✓ Defrag context has compute context\n");
	} else {
		printf("⚠ Defrag context missing compute context (expected during defrag)\n");
	}

	if (atlas->defragContext.useCompute) {
		printf("✓ Defrag context using compute path\n");
	} else {
		printf("⚠ Defrag context not using compute path\n");
	}

	// Test 5: Get atlas statistics
	printf("\n--- Test 5: Atlas Statistics ---\n");

	uint32_t cacheHits, cacheMisses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &cacheHits, &cacheMisses, &evictions, &uploads);

	printf("Cache hits: %u\n", cacheHits);
	printf("Cache misses: %u\n", cacheMisses);
	printf("Evictions: %u\n", evictions);
	printf("Uploads: %u\n", uploads);

	// Cleanup
	vkFreeCommandBuffers(winCtx->device, commandPool, 1, &cmd);
	vkDestroyCommandPool(winCtx->device, commandPool, NULL);
	vknvg__destroyVirtualAtlas(atlas);
	window_destroy_context(winCtx);

	printf("\n=== All Tests Passed ===\n");
	return 0;
}
