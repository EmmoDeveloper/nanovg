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
	printf("=== NanoVG Convex Fill Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Convex Fill Test");
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
	printf("3. Rendering convex fills...\n");

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
	clearValues[0].color = (VkClearColorValue){{0.15f, 0.15f, 0.15f, 1.0f}};
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

	// Triangle (red) at (100, 100)
	nvgBeginPath(vg);
	float cx = 100, cy = 100, radius = 60;
	for (int i = 0; i < 3; i++) {
		float angle = (float)i / 3 * 2.0f * M_PI;
		float x = cx + cosf(angle) * radius;
		float y = cy + sinf(angle) * radius;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgClosePath(vg);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 1.0f));
	nvgFill(vg);

	// Pentagon (green) at (300, 100)
	nvgBeginPath(vg);
	cx = 300; cy = 100; radius = 60;
	for (int i = 0; i < 5; i++) {
		float angle = (float)i / 5 * 2.0f * M_PI;
		float x = cx + cosf(angle) * radius;
		float y = cy + sinf(angle) * radius;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgClosePath(vg);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 1.0f));
	nvgFill(vg);

	// Hexagon (blue) at (500, 100)
	nvgBeginPath(vg);
	cx = 500; cy = 100; radius = 60;
	for (int i = 0; i < 6; i++) {
		float angle = (float)i / 6 * 2.0f * M_PI;
		float x = cx + cosf(angle) * radius;
		float y = cy + sinf(angle) * radius;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgClosePath(vg);
	nvgFillColor(vg, nvgRGBAf(0.0f, 0.0f, 1.0f, 1.0f));
	nvgFill(vg);

	// Octagon (yellow) at (700, 100)
	nvgBeginPath(vg);
	cx = 700; cy = 100; radius = 60;
	for (int i = 0; i < 8; i++) {
		float angle = (float)i / 8 * 2.0f * M_PI;
		float x = cx + cosf(angle) * radius;
		float y = cy + sinf(angle) * radius;
		if (i == 0)
			nvgMoveTo(vg, x, y);
		else
			nvgLineTo(vg, x, y);
	}
	nvgClosePath(vg);
	nvgFillColor(vg, nvgRGBAf(1.0f, 1.0f, 0.0f, 1.0f));
	nvgFill(vg);

	// Rounded rectangle (cyan) at (50, 250, 150, 100)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 50, 250, 150, 100, 20);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 1.0f, 1.0f));
	nvgFill(vg);

	// Rounded rectangle (magenta) at (250, 250, 150, 100)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 250, 250, 150, 100, 30);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 1.0f, 1.0f));
	nvgFill(vg);

	// Rounded rectangle (orange) at (450, 250, 150, 100)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 450, 250, 150, 100, 40);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.5f, 0.0f, 1.0f));
	nvgFill(vg);

	// Circle (white) at (400, 480)
	nvgBeginPath(vg);
	nvgCircle(vg, 400, 480, 80);
	nvgFillColor(vg, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
	nvgFill(vg);

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

	printf("   ✓ Convex fills rendered\n\n");

	// Save screenshot
	printf("4. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/convexfill_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/convexfill_test.ppm\n\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Convex Fill Test PASSED ===\n");
	return 0;
}
