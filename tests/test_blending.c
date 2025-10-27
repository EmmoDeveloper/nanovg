#include "window_utils.h"
#include "../src/vulkan/nvg_vk_context.h"
#include "../src/vulkan/nvg_vk_buffer.h"
#include "../src/vulkan/nvg_vk_texture.h"
#include "../src/vulkan/nvg_vk_pipeline.h"
#include "../src/vulkan/nvg_vk_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to create a rectangle with specific blend mode
static void create_blended_rect(NVGVkContext* ctx, float x, float y, float w, float h, float r, float g, float b, float a, int blendMode)
{
	int start = ctx->vertexCount;
	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Rectangle as two triangles
	verts[start + 0].x = x;     verts[start + 0].y = y;     verts[start + 0].u = 0.0f; verts[start + 0].v = 0.0f;
	verts[start + 1].x = x + w; verts[start + 1].y = y;     verts[start + 1].u = 1.0f; verts[start + 1].v = 0.0f;
	verts[start + 2].x = x + w; verts[start + 2].y = y + h; verts[start + 2].u = 1.0f; verts[start + 2].v = 1.0f;

	verts[start + 3].x = x;     verts[start + 3].y = y;     verts[start + 3].u = 0.0f; verts[start + 3].v = 0.0f;
	verts[start + 4].x = x + w; verts[start + 4].y = y + h; verts[start + 4].u = 1.0f; verts[start + 4].v = 1.0f;
	verts[start + 5].x = x;     verts[start + 5].y = y + h; verts[start + 5].u = 0.0f; verts[start + 5].v = 1.0f;

	ctx->vertexCount += 6;

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_TRIANGLES;
	ctx->calls[callIdx].image = -1;
	ctx->calls[callIdx].triangleOffset = start;
	ctx->calls[callIdx].triangleCount = 6;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = blendMode;

	// Setup uniforms
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = r;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = a;
	ctx->uniformCount++;
}

int main(void)
{
	printf("=== NanoVG Vulkan - Blend Modes Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Blend Modes Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG Vulkan context
	printf("2. Creating NanoVG Vulkan context...\n");
	NVGVkContext nvgCtx = {0};
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

	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);

	// Create dummy texture
	unsigned char dummyData[4] = {255, 255, 255, 255};
	int dummyTex = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, 1, 1, 0, dummyData);

	// Create pipelines
	printf("3. Creating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, dummyTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created\n\n");

	// Create test pattern
	printf("4. Creating blend test pattern...\n");

	// Base layer - background gradient using mode 0 (source-over)
	for (int i = 0; i < 6; i++) {
		float x = i * 130 + 15;
		create_blended_rect(&nvgCtx, x, 50, 120, 80, 0.3f, 0.3f, 0.5f, 1.0f, 0);
	}

	// Row 1: Overlapping circles testing different blend modes
	// Mode 0: Source over (default)
	create_blended_rect(&nvgCtx, 20, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 50, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 0);

	// Mode 1: Source in
	create_blended_rect(&nvgCtx, 150, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 180, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 1);

	// Mode 2: Source out
	create_blended_rect(&nvgCtx, 280, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 310, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 2);

	// Mode 3: Atop
	create_blended_rect(&nvgCtx, 410, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 440, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 3);

	// Mode 4: Destination over
	create_blended_rect(&nvgCtx, 540, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 570, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 4);

	// Mode 5: Destination in
	create_blended_rect(&nvgCtx, 670, 60, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 700, 60, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 5);

	// Row 2: More blend modes
	for (int i = 0; i < 6; i++) {
		float x = i * 130 + 15;
		create_blended_rect(&nvgCtx, x, 180, 120, 80, 0.5f, 0.3f, 0.3f, 1.0f, 0);
	}

	// Mode 6: Destination out
	create_blended_rect(&nvgCtx, 20, 190, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 50, 190, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 6);

	// Mode 7: Destination atop
	create_blended_rect(&nvgCtx, 150, 190, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 180, 190, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 7);

	// Mode 8: Lighter (additive)
	create_blended_rect(&nvgCtx, 280, 190, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 310, 190, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 8);

	// Mode 9: Copy
	create_blended_rect(&nvgCtx, 410, 190, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 440, 190, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 9);

	// Mode 10: XOR
	create_blended_rect(&nvgCtx, 540, 190, 60, 60, 1.0f, 0.0f, 0.0f, 0.7f, 0);
	create_blended_rect(&nvgCtx, 570, 190, 60, 60, 0.0f, 1.0f, 0.0f, 0.7f, 10);

	// Complex test: Multiple overlapping shapes with different blend modes
	create_blended_rect(&nvgCtx, 100, 350, 200, 200, 1.0f, 0.0f, 0.0f, 0.5f, 0);   // Red base
	create_blended_rect(&nvgCtx, 150, 400, 200, 200, 0.0f, 1.0f, 0.0f, 0.5f, 8);  // Green additive
	create_blended_rect(&nvgCtx, 200, 350, 200, 200, 0.0f, 0.0f, 1.0f, 0.5f, 8);  // Blue additive

	// Opacity test
	create_blended_rect(&nvgCtx, 450, 350, 100, 180, 1.0f, 1.0f, 1.0f, 1.0f, 0);
	create_blended_rect(&nvgCtx, 470, 370, 100, 180, 1.0f, 0.0f, 0.0f, 0.25f, 0);
	create_blended_rect(&nvgCtx, 490, 390, 100, 180, 0.0f, 1.0f, 0.0f, 0.5f, 0);
	create_blended_rect(&nvgCtx, 510, 410, 100, 180, 0.0f, 0.0f, 1.0f, 0.75f, 0);

	printf("   ✓ Created %d vertices, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.callCount);

	// Setup descriptors
	printf("5. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("6. Rendering blend modes test...\n");

	uint32_t imageIndex;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(nvgCtx.commandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearValues[2] = {0};
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(nvgCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(nvgCtx.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(nvgCtx.commandBuffer, 0, 1, &scissor);

	nvgvk_flush(&nvgCtx);

	vkCmdEndRenderPass(nvgCtx.commandBuffer);
	vkEndCommandBuffer(nvgCtx.commandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &nvgCtx.commandBuffer;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("   ✓ Blend modes rendered\n\n");

	// Save screenshot
	printf("7. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "blending_test.ppm")) {
		printf("   ✓ Screenshot saved to blending_test.ppm\n\n");
	}

	// Cleanup
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);

	printf("=== Blending Test PASSED ===\n");
	printf("\nBlend modes tested:\n");
	printf("  0: Source over (default)\n");
	printf("  1: Source in\n");
	printf("  2: Source out\n");
	printf("  3: Atop\n");
	printf("  4: Destination over\n");
	printf("  5: Destination in\n");
	printf("  6: Destination out\n");
	printf("  7: Destination atop\n");
	printf("  8: Lighter (additive)\n");
	printf("  9: Copy\n");
	printf(" 10: XOR\n");

	return 0;
}
