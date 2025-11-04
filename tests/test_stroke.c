#include "window_utils.h"
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void)
{
	printf("=== NanoVG Stroke Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Stroke Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");

	// Render
	printf("3. Rendering strokes...\n");

	uint32_t imageIndex;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Get command buffer from NanoVG
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuf, &beginInfo);

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

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Begin NanoVG frame
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Thin horizontal line (red)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 100, 50);
	nvgLineTo(vg, 300, 50);
	nvgStrokeWidth(vg, 2.0f);
	nvgStrokeColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 1.0f));
	nvgStroke(vg);

	// Medium horizontal line (green)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 100, 100);
	nvgLineTo(vg, 300, 100);
	nvgStrokeWidth(vg, 5.0f);
	nvgStrokeColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 1.0f));
	nvgStroke(vg);

	// Thick horizontal line (blue)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 100, 160);
	nvgLineTo(vg, 300, 160);
	nvgStrokeWidth(vg, 10.0f);
	nvgStrokeColor(vg, nvgRGBAf(0.0f, 0.0f, 1.0f, 1.0f));
	nvgStroke(vg);

	// Diagonal line (yellow)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 350, 50);
	nvgLineTo(vg, 500, 180);
	nvgStrokeWidth(vg, 5.0f);
	nvgStrokeColor(vg, nvgRGBAf(1.0f, 1.0f, 0.0f, 1.0f));
	nvgStroke(vg);

	// Zigzag path (cyan)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 550, 50);
	nvgLineTo(vg, 600, 100);
	nvgLineTo(vg, 650, 50);
	nvgLineTo(vg, 700, 100);
	nvgLineTo(vg, 750, 50);
	nvgStrokeWidth(vg, 4.0f);
	nvgStrokeColor(vg, nvgRGBAf(0.0f, 1.0f, 1.0f, 1.0f));
	nvgStroke(vg);

	// Sine wave (magenta)
	nvgBeginPath(vg);
	for (int i = 0; i < 20; i++) {
		float x = 100 + i * 30;
		float y = 300 + sinf(i * 0.5f) * 40;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgStrokeWidth(vg, 3.0f);
	nvgStrokeColor(vg, nvgRGBAf(1.0f, 0.0f, 1.0f, 1.0f));
	nvgStroke(vg);

	// Square path (orange)
	nvgBeginPath(vg);
	nvgMoveTo(vg, 100, 400);
	nvgLineTo(vg, 200, 400);
	nvgLineTo(vg, 200, 500);
	nvgLineTo(vg, 100, 500);
	nvgClosePath(vg);
	nvgStrokeWidth(vg, 6.0f);
	nvgStrokeColor(vg, nvgRGBAf(1.0f, 0.5f, 0.0f, 1.0f));
	nvgStroke(vg);

	// Circle (white)
	nvgBeginPath(vg);
	nvgCircle(vg, 450, 450, 80);
	nvgStrokeWidth(vg, 5.0f);
	nvgStrokeColor(vg, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
	nvgStroke(vg);

	// Spiral (lime)
	nvgBeginPath(vg);
	for (int i = 0; i <= 50; i++) {
		float angle = (float)i / 50 * 4.0f * M_PI;
		float radius = 10 + i * 1.2f;
		float x = 650 + cosf(angle) * radius;
		float y = 450 + sinf(angle) * radius;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgStrokeWidth(vg, 2.5f);
	nvgStrokeColor(vg, nvgRGBAf(0.5f, 1.0f, 0.0f, 1.0f));
	nvgStroke(vg);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("   ✓ Strokes rendered\n\n");

	// Save screenshot
	printf("4. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/stroke_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/stroke_test.ppm\n\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Stroke Test PASSED ===\n");
	return 0;
}
