#include "window_utils.h"
#include "../src/nanovg.h"
#include "../src/vulkan/nvg_vk_context.h"
#include "../src/vulkan/nvg_vk_buffer.h"
#include "../src/vulkan/nvg_vk_texture.h"
#include "../src/vulkan/nvg_vk_shader.h"
#include "../src/vulkan/nvg_vk_pipeline.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("=== NanoVG Vulkan Backend - Phase 1, 2 & 3 Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Vulkan Phase 1 Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG Vulkan context
	printf("2. Creating NanoVG Vulkan context...\n");
	NVGVkContext nvgCtx;
	NVGVkCreateInfo createInfo = {0};
	createInfo.device = winCtx->device;
	createInfo.physicalDevice = winCtx->physicalDevice;
	createInfo.queue = winCtx->graphicsQueue;
	createInfo.commandPool = winCtx->commandPool;
	createInfo.flags = 0;

	if (!nvgvk_create(&nvgCtx, &createInfo)) {
		fprintf(stderr, "Failed to create NanoVG Vulkan context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG Vulkan context created\n\n");

	// Test viewport
	printf("3. Testing viewport...\n");
	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);
	printf("   ✓ Viewport set to 800x600\n\n");

	// Test buffer creation
	printf("4. Testing buffer management...\n");
	NVGVkBuffer testBuffer;
	if (!nvgvk_buffer_create(&nvgCtx, &testBuffer, 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
		fprintf(stderr, "Failed to create buffer\n");
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Buffer created (4096 bytes)\n");

	// Test buffer upload
	float testData[] = {1.0f, 2.0f, 3.0f, 4.0f};
	if (!nvgvk_buffer_upload(&nvgCtx, &testBuffer, testData, sizeof(testData))) {
		fprintf(stderr, "Failed to upload to buffer\n");
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Data uploaded to buffer\n");

	// Test buffer growth
	char largeData[8192] = {0};
	if (!nvgvk_buffer_reserve(&nvgCtx, &testBuffer, sizeof(largeData))) {
		fprintf(stderr, "Failed to grow buffer\n");
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Buffer grown to 8192 bytes\n");

	// Test flush (currently just resets state)
	printf("\n5. Testing flush...\n");
	nvgvk_flush(&nvgCtx);
	printf("   ✓ Flush completed\n\n");

	// Phase 2: Test texture creation
	printf("6. Testing texture management...\n");

	// Create a simple 64x64 RGBA texture
	unsigned char texData[64 * 64 * 4];
	for (int i = 0; i < 64 * 64; i++) {
		texData[i * 4 + 0] = 255;  // R
		texData[i * 4 + 1] = 0;    // G
		texData[i * 4 + 2] = 0;    // B
		texData[i * 4 + 3] = 255;  // A
	}

	int texId = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, 64, 64, 0, texData);
	if (texId < 0) {
		fprintf(stderr, "Failed to create texture\n");
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Texture created (64x64 RGBA, id=%d)\n", texId);

	// Test texture size query
	int tw, th;
	if (!nvgvk_get_texture_size(&nvgCtx, texId, &tw, &th)) {
		fprintf(stderr, "Failed to get texture size\n");
		nvgvk_delete_texture(&nvgCtx, texId);
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Texture size queried: %dx%d\n", tw, th);

	// Test texture update
	unsigned char updateData[32 * 32 * 4];
	for (int i = 0; i < 32 * 32; i++) {
		updateData[i * 4 + 0] = 0;    // R
		updateData[i * 4 + 1] = 255;  // G
		updateData[i * 4 + 2] = 0;    // B
		updateData[i * 4 + 3] = 255;  // A
	}

	if (!nvgvk_update_texture(&nvgCtx, texId, 0, 0, 32, 32, updateData)) {
		fprintf(stderr, "Failed to update texture\n");
		nvgvk_delete_texture(&nvgCtx, texId);
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Texture updated (32x32 region)\n");

	// Phase 3: Test pipeline creation
	printf("\n7. Testing pipeline management (Phase 3)...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, texId);
		nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created (4 variants)\n");

	// Test pipeline binding
	printf("\n8. Testing pipeline binding...\n");
	nvgvk_bind_pipeline(&nvgCtx, NVGVK_PIPELINE_SIMPLE);
	printf("   ✓ Pipeline bound (SIMPLE)\n");

	// Cleanup
	printf("\n9. Cleaning up...\n");
	nvgvk_destroy_pipelines(&nvgCtx);
	printf("   ✓ Pipelines destroyed\n");

	nvgvk_delete_texture(&nvgCtx, texId);
	printf("   ✓ Texture destroyed\n");

	nvgvk_buffer_destroy(&nvgCtx, &testBuffer);
	printf("   ✓ Buffer destroyed\n");

	nvgvk_delete(&nvgCtx);
	printf("   ✓ NanoVG Vulkan context destroyed\n");

	window_destroy_context(winCtx);
	printf("   ✓ Window context destroyed\n\n");

	printf("=== Phase 1, 2 & 3 Tests PASSED ===\n");
	return 0;
}
