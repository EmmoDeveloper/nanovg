#include "window_utils.h"
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void)
{
	printf("=== NanoVG Scissor Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Scissor Test");
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
	printf("3. Rendering scissored rectangles...\n");

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

	// Large red rectangle (400x400) scissored to top-left (200x200)
	nvgScissor(vg, 50, 50, 200, 200);
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 400, 400);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 1.0f));
	nvgFill(vg);
	nvgResetScissor(vg);

	// Large green rectangle (400x400) scissored to top-right (200x200)
	nvgScissor(vg, 550, 50, 200, 200);
	nvgBeginPath(vg);
	nvgRect(vg, 350, 50, 400, 400);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 1.0f));
	nvgFill(vg);
	nvgResetScissor(vg);

	// Large blue rectangle (400x400) scissored to bottom-left (200x200)
	nvgScissor(vg, 50, 350, 200, 200);
	nvgBeginPath(vg);
	nvgRect(vg, 50, 150, 400, 400);
	nvgFillColor(vg, nvgRGBAf(0.0f, 0.0f, 1.0f, 1.0f));
	nvgFill(vg);
	nvgResetScissor(vg);

	// Large yellow rectangle (400x400) scissored to bottom-right (200x200)
	nvgScissor(vg, 550, 350, 200, 200);
	nvgBeginPath(vg);
	nvgRect(vg, 350, 150, 400, 400);
	nvgFillColor(vg, nvgRGBAf(1.0f, 1.0f, 0.0f, 1.0f));
	nvgFill(vg);
	nvgResetScissor(vg);

	// Center magenta rectangle (300x300) scissored to small center region (100x100)
	nvgScissor(vg, 350, 250, 100, 100);
	nvgBeginPath(vg);
	nvgRect(vg, 250, 200, 300, 300);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 1.0f, 1.0f));
	nvgFill(vg);
	nvgResetScissor(vg);

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

	printf("   ✓ Scissored rectangles rendered\n\n");

	// Save screenshot
	printf("4. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/scissor_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/scissor_test.ppm\n\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Scissor Test PASSED ===\n");
	return 0;
}
