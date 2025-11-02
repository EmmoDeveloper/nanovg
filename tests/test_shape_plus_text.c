#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"

int main(void)
{
	printf("=== Shape + Text Test ===\n");

	// Seed random number generator
	srand(time(NULL));

	// Create window
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Shape + Text Test");
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

	// Render 360 frames (6 seconds at 60fps, 6 different scenes)
	for (int frame = 0; frame < 360; frame++) {
		int scene = frame / 60;  // Change scene every 60 frames (1 second)
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

		// Add randomness every frame
		int shapeR = rand() % 256;
		int shapeG = rand() % 256;
		int shapeB = rand() % 256;
		int textR = rand() % 256;
		int textG = rand() % 256;
		int textB = rand() % 256;
		float shapeX = 100 + (rand() % 500);
		float shapeY = 100 + (rand() % 400);
		float textX = 100 + (rand() % 600);
		float textY = 100 + (rand() % 400);
		float size = 50 + (rand() % 100);

		// Scene changes every second
		if (scene == 0) {
			// Scene 1: Random circle + "HELLO"
			nvgBeginPath(vg);
			nvgCircle(vg, shapeX, shapeY, size);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 72.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(vg, textX, textY, "HELLO", NULL);
		}
		else if (scene == 1) {
			// Scene 2: Random rectangle + "WORLD"
			nvgBeginPath(vg);
			nvgRect(vg, shapeX, shapeY, size * 2, size * 1.5f);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 96.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, textX, textY, "WORLD", NULL);
		}
		else if (scene == 2) {
			// Scene 3: Random rounded rect + "TEST"
			nvgBeginPath(vg);
			nvgRoundedRect(vg, shapeX, shapeY, size * 2, size * 1.5f, 20);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 64.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(vg, textX, textY, "TEST", NULL);
		}
		else if (scene == 3) {
			// Scene 4: Random circle + "123"
			nvgBeginPath(vg);
			nvgCircle(vg, shapeX, shapeY, size);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 120.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, textX, textY, "123", NULL);
		}
		else if (scene == 4) {
			// Scene 5: Random triangle + "ABC"
			nvgBeginPath(vg);
			nvgMoveTo(vg, shapeX, shapeY);
			nvgLineTo(vg, shapeX - size, shapeY + size * 2);
			nvgLineTo(vg, shapeX + size, shapeY + size * 2);
			nvgClosePath(vg);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 80.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
			nvgText(vg, textX, textY, "ABC", NULL);
		}
		else {
			// Scene 6: Random rect + "END"
			nvgBeginPath(vg);
			nvgRect(vg, shapeX, shapeY, size * 2, size);
			nvgFillColor(vg, nvgRGBA(shapeR, shapeG, shapeB, 255));
			nvgFill(vg);

			nvgFontFace(vg, "sans");
			nvgFontSize(vg, 100.0f);
			nvgFillColor(vg, nvgRGBA(textR, textG, textB, 255));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, textX, textY, "END", NULL);
		}

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

		if (frame % 60 == 0) {
			printf("Scene %d (Frame %d/360)\n", scene + 1, frame);
		}
	}

	printf("✓ Test complete\n");
	printf("\nYou should have seen 6 scenes (1 second each):\n");
	printf("  Scene 1: Green circle + Red 'HELLO'\n");
	printf("  Scene 2: Blue rectangle + Yellow 'WORLD'\n");
	printf("  Scene 3: Purple rounded rect + Cyan 'TEST'\n");
	printf("  Scene 4: Orange circle + White '123'\n");
	printf("  Scene 5: Red triangle + Green 'ABC'\n");
	printf("  Scene 6: Yellow rect + Magenta 'END'\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
