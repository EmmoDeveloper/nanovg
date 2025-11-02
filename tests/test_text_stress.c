#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"

static double get_time_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(void)
{
	printf("========================================\n");
	printf("TEXT STRESS TEST\n");
	printf("========================================\n");
	printf("Duration: 60 seconds\n");
	printf("Random characters at high volume\n");
	printf("Goal: Stress atlas and trigger defragmentation\n");
	printf("========================================\n\n");

	// Seed random
	srand(time(NULL));

	// Create window
	WindowVulkanContext* winCtx = window_create_context(1280, 720, "Text Stress Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}
	printf("✓ Window created (1280x720)\n");

	// Create NanoVG
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ NanoVG context created\n");

	// Load font
	int fontNormal = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (fontNormal == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Font loaded\n\n");

	printf("Starting text stress test...\n");
	printf("Rendering 500 random characters per frame\n\n");

	// Timing
	double startTime = get_time_sec();
	int frameCount = 0;
	float fps = 0.0f;
	int fpsFrameCount = 0;
	double lastFpsTime = startTime;

	int width = 1280;
	int height = 720;

	// Large character set for variety
	const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?/~`";
	int numChars = strlen(chars);

	// Test loop - 60 seconds
	while (1) {
		double currentTime = get_time_sec();
		double elapsed = currentTime - startTime;

		// Check if test duration exceeded
		if (elapsed >= 60.0) {
			printf("\n✓ Test duration reached (60 seconds)\n");
			break;
		}

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

		// Animated background color
		float hue = (float)elapsed / 60.0f;
		float r = 0.1f + 0.1f * sinf(hue * 6.28f);
		float g = 0.1f + 0.1f * sinf(hue * 6.28f + 2.09f);
		float b = 0.1f + 0.1f * sinf(hue * 6.28f + 4.18f);

		VkClearValue clearValues[2] = {0};
		clearValues[0].color = (VkClearColorValue){{r, g, b, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		VkViewport viewport = {0};
		viewport.width = (float)width;
		viewport.height = (float)height;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = {0};
		scissor.extent = winCtx->swapchainExtent;
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		// Notify NanoVG
		nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, width, height, 1.0f);

		// Set font
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		// Render 500 random characters at random positions
		for (int i = 0; i < 500; i++) {
			// Random position
			float x = (float)(rand() % width);
			float y = (float)(rand() % height);

			// Random size (12pt to 72pt)
			float size = 12.0f + (float)(rand() % 60);
			nvgFontSize(vg, size);

			// Random bright color
			int cr = 128 + (rand() % 128);
			int cg = 128 + (rand() % 128);
			int cb = 128 + (rand() % 128);
			nvgFillColor(vg, nvgRGBA(cr, cg, cb, 255));

			// Random character
			char text[2];
			text[0] = chars[rand() % numChars];
			text[1] = '\0';

			nvgText(vg, x, y, text, NULL);
		}

		// Draw FPS overlay
		nvgFontSize(vg, 24.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		char buf[128];
		snprintf(buf, sizeof(buf), "FPS: %.1f | Frame: %d | Time: %.1fs", fps, frameCount, elapsed);
		nvgText(vg, 10, 10, buf, NULL);

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

		frameCount++;
		fpsFrameCount++;

		// FPS calculation
		if (currentTime - lastFpsTime >= 0.5) {
			fps = fpsFrameCount / (float)(currentTime - lastFpsTime);
			fpsFrameCount = 0;
			lastFpsTime = currentTime;
		}

		// Print progress every 5 seconds
		if (frameCount % 300 == 0) {
			printf("[%.1fs] Frame %d | FPS: %.1f\n", elapsed, frameCount, fps);
		}

		// Frame rate limiting to 60 FPS
		double frameTime = 1.0 / 60.0;
		double renderTime = get_time_sec() - currentTime;
		if (renderTime < frameTime) {
			double sleepTime = frameTime - renderTime;
			struct timespec ts;
			ts.tv_sec = (time_t)sleepTime;
			ts.tv_nsec = (long)((sleepTime - ts.tv_sec) * 1000000000);
			nanosleep(&ts, NULL);
		}
	}

	// Final stats
	double totalTime = get_time_sec() - startTime;
	float avgFps = frameCount / totalTime;

	printf("\n========================================\n");
	printf("Test Complete!\n");
	printf("========================================\n");
	printf("Total time: %.2f seconds\n", totalTime);
	printf("Total frames: %d\n", frameCount);
	printf("Total characters rendered: ~%d\n", frameCount * 500);
	printf("Average FPS: %.1f\n", avgFps);
	printf("\nCheck output for [DEFRAG] and [ATLAS] messages\n");
	printf("========================================\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
