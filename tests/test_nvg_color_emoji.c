#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

// Test color emoji rendering through NanoVG API
// Uses Noto Color Emoji font with COLR table

int main(void)
{
	printf("=== NanoVG Color Emoji Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Color Emoji Test");
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

	// Load color emoji font
	printf("3. Loading color emoji font...\n");
	int emojiFont = nvgCreateFont(vg, "emoji", "fonts/emoji/Noto-COLRv1.ttf");
	if (emojiFont == -1) {
		fprintf(stderr, "Failed to load emoji font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Emoji font loaded (id=%d)\n\n", emojiFont);

	// Emoji test strings
	const char* emojiTests[] = {
		"😀",  // U+1F600 Grinning face
		"❤️",  // U+2764 Red heart
		"👍",  // U+1F44D Thumbs up
		"🎉",  // U+1F389 Party popper
		"🌟",  // U+1F31F Star
		"🚀",  // U+1F680 Rocket
	};
	const char* labels[] = {
		"Grinning Face",
		"Red Heart",
		"Thumbs Up",
		"Party Popper",
		"Star",
		"Rocket"
	};

	printf("4. Rendering color emoji (press ESC to close, window will auto-close after 5 seconds)...\n\n");

	// Render loop
	int frameCount = 0;
	int maxFrames = 300;  // 5 seconds at 60fps
	int screenshotSaved = 0;
	uint32_t imageIndex = 0;

	while (!glfwWindowShouldClose(winCtx->window) && frameCount < maxFrames) {
		glfwPollEvents();

		// Acquire swapchain image
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkSemaphore imageAvailableSemaphore;
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

		// White background
		VkClearValue clearValues[2] = {0};
		clearValues[0].color = (VkClearColorValue){{1.0f, 1.0f, 1.0f, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		VkViewport viewport = {0};
		viewport.width = 800.0f;
		viewport.height = 600.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = {0};
		scissor.extent = winCtx->swapchainExtent;
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, 800, 600, 1.0f);

		// Title
		nvgFontFaceId(vg, emojiFont);
		nvgFontSize(vg, 48.0f);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgText(vg, 400.0f, 30.0f, "Color Emoji Test", NULL);

		// Render emoji samples in a grid
		nvgFontSize(vg, 64.0f);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

		for (int i = 0; i < 6; i++) {
			int row = i / 3;
			int col = i % 3;
			float x = 150.0f + col * 250.0f;
			float y = 150.0f + row * 200.0f;

			// Draw emoji (use white fill color so emoji colors show through)
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, x, y, emojiTests[i], NULL);

			// Draw label
			nvgFontSize(vg, 18.0f);
			nvgFillColor(vg, nvgRGBA(100, 100, 100, 255));
			nvgText(vg, x, y + 80.0f, labels[i], NULL);
			nvgFontSize(vg, 64.0f);
		}

		// Footer
		nvgFontSize(vg, 16.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
		nvgText(vg, 400.0f, 580.0f, "Cairo COLR v1 Rendering", NULL);

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

		// Save screenshot after first frame
		if (frameCount == 0 && !screenshotSaved) {
			printf("   Saving screenshot...\n");
			if (window_save_screenshot(winCtx, imageIndex, "color_emoji_test.ppm")) {
				printf("   ✓ Screenshot saved to color_emoji_test.ppm\n");
				screenshotSaved = 1;
			}
		}

		frameCount++;
	}

	printf("   ✓ Rendered %d frames\n\n", frameCount);

	// Cleanup
	printf("5. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n");

	printf("\n=== Color Emoji Test Complete ===\n");
	return 0;
}
