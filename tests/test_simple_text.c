#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <time.h>
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"

int main(void)
{
	printf("=== Simple Text Test ===\n");

	// Create window
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Simple Text Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}
	printf("✓ Window created\n");

	// Create NanoVG
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ NanoVG created\n");

	// Load font
	int font = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Font loaded\n");

	// Render 120 frames (2 seconds at 60fps)
	for (int frame = 0; frame < 120; frame++) {
		// Acquire image
		uint32_t imageIndex;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

		vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
		                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		// Get command buffer
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
		clearValues[0].color = (VkClearColorValue){{0.2f, 0.3f, 0.4f, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport
		VkViewport viewport = {0};
		viewport.width = 800.0f;
		viewport.height = 600.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = {0};
		scissor.extent = winCtx->swapchainExtent;
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		// Notify NanoVG
		nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, 800, 600, 1.0f);

		// Draw text
		nvgFontFace(vg, "sans");
		nvgFontSize(vg, 72.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, 100, 100, "HELLO WORLD", NULL);

		nvgFontSize(vg, 48.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 0, 255));
		nvgText(vg, 100, 200, "Testing Text", NULL);

		nvgFontSize(vg, 32.0f);
		nvgFillColor(vg, nvgRGBA(0, 255, 255, 255));
		nvgText(vg, 100, 300, "123456789", NULL);

		// End NanoVG frame
		nvgEndFrame(vg);

		// End render pass
		vkCmdEndRenderPass(cmdBuf);
		vkEndCommandBuffer(cmdBuf);

		// Submit
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

		// Present
		VkPresentInfoKHR presentInfo = {0};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &winCtx->swapchain;
		presentInfo.pImageIndices = &imageIndex;
		vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);

		vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

		// Small delay
		struct timespec ts = {0, 16666666};  // ~60fps
		nanosleep(&ts, NULL);

		if (frame % 30 == 0) {
			printf("Frame %d/120\n", frame);
		}
	}

	printf("✓ Test complete\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
