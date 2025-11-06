#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
	printf("=== Visual Bounds Test ===\n");
	printf("This test renders text and draws bounding boxes to visually verify accuracy.\n\n");

	WindowVulkanContext* winCtx = window_create_context(1024, 768, "Visual Bounds Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "dejavu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	const char* test_texts[] = {
		"Hello World",
		"office fluffy",
		"Ligatures: fi fl ffi ffl",
		"CAPITALS",
		"Mixed 123 Numbers"
	};
	int num_texts = 5;

	printf("Rendering %d test strings with bounds visualization...\n", num_texts);
	printf("- Green box: nvgTextBounds (simple)\n");
	printf("- Red box: nvgTextBoundsWithShaping (shaped)\n");
	printf("- Text will be rendered in white\n\n");

	// Render a single frame
	// Acquire image
	uint32_t imageIndex;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Get NanoVG command buffer
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

		// Begin render pass
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

	// Set viewport and scissor
	VkViewport viewport = {0};
	viewport.width = (float)winCtx->swapchainExtent.width;
	viewport.height = (float)winCtx->swapchainExtent.height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Begin NanoVG frame
	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

		nvgFontFaceId(vg, font);
		nvgFontSize(vg, 48.0f);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		float y = 50.0f;
		for (int i = 0; i < num_texts; i++) {
			float x = 50.0f;
			const char* text = test_texts[i];

			// Get simple bounds
			float bounds1[4];
			nvgTextBounds(vg, x, y, text, NULL, bounds1);

			// Get shaped bounds
			float bounds2[4];
			nvgTextBoundsWithShaping(vg, x, y, text, NULL, bounds2);

			// Draw simple bounds box (green)
			nvgBeginPath(vg);
			nvgRect(vg, bounds1[0], bounds1[1], bounds1[2] - bounds1[0], bounds1[3] - bounds1[1]);
			nvgStrokeColor(vg, nvgRGBA(0, 255, 0, 200));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);

			// Draw shaped bounds box (red)
			nvgBeginPath(vg);
			nvgRect(vg, bounds2[0], bounds2[1], bounds2[2] - bounds2[0], bounds2[3] - bounds2[1]);
			nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 200));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);

			// Draw text (white)
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, x, y, text, NULL);

			// Print bounds info
			printf("'%s':\n", text);
			printf("  Simple:  [%.1f, %.1f, %.1f, %.1f] width=%.1f\n",
				bounds1[0], bounds1[1], bounds1[2], bounds1[3], bounds1[2] - bounds1[0]);
			printf("  Shaped:  [%.1f, %.1f, %.1f, %.1f] width=%.1f\n",
				bounds2[0], bounds2[1], bounds2[2], bounds2[3], bounds2[2] - bounds2[0]);
			if (bounds1[2] != bounds2[2] || bounds1[0] != bounds2[0]) {
				printf("  DIFFERENCE detected!\n");
			}
			printf("\n");

			y += 120.0f;
		}

	nvgEndFrame(vg);

	// End render pass and command buffer
	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	// Submit command buffer
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.pWaitDstStageMask = waitStages;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	printf("Frame rendered. Saving screenshot...\n");

	// Save screenshot
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/visual_bounds_test.ppm") == 0) {
		printf("Screenshot saved to visual_bounds_test.ppm\n");
	} else {
		printf("Failed to save screenshot\n");
	}

	// Present
	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &winCtx->swapchain;
	presentInfo.pImageIndices = &imageIndex;
	vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("Test complete.\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
