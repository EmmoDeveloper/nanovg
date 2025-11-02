#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"

// 1-minute stress test with maximum text and shapes
// Goal: Trigger defragmentation and test compute shader path

#define TEST_DURATION_SEC 60
#define TARGET_FPS 60

// Helper to get time in seconds
static double get_time_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Render crazy amount of text at random positions
static void render_text_chaos(NVGcontext* vg, int width, int height, float time)
{
	// Lots of different sizes to create varied glyph atlas usage
	float sizes[] = {12, 16, 20, 24, 28, 32, 40, 48, 64};
	int numSizes = sizeof(sizes) / sizeof(sizes[0]);

	// Simple ASCII text (DejaVuSans supports this)
	const char* texts[] = {
		"HELLO",
		"WORLD",
		"TEST",
		"TEXT",
		"STRESS",
		"CHAOS",
		"SHAPES",
		"RENDER",
		"ATLAS",
		"VULKAN",
		"12345",
		"ABCDE",
	};
	int numTexts = sizeof(texts) / sizeof(texts[0]);

	// Set text alignment for all text rendering
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	// Render 100 text items per frame at random positions
	for (int i = 0; i < 100; i++) {
		float x = (float)(rand() % width);
		float y = (float)(rand() % height);
		float size = sizes[rand() % numSizes];
		int textIdx = rand() % numTexts;

		// Bright white, fully opaque text
		nvgFontSize(vg, size);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgText(vg, x, y, texts[textIdx], NULL);
	}

	// Add some rotating text
	float angle = time * 2.0f;
	nvgSave(vg);
	nvgTranslate(vg, width / 2.0f, height / 2.0f);
	nvgRotate(vg, angle);
	nvgFontSize(vg, 48);
	nvgFillColor(vg, nvgRGBA(255, 255, 0, 200));
	nvgText(vg, -100, 0, "SPINNING!", NULL);
	nvgRestore(vg);
}

// Render crazy shapes
static void render_shape_chaos(NVGcontext* vg, int width, int height, float time)
{
	// 200 random shapes per frame
	for (int i = 0; i < 200; i++) {
		float x = (float)(rand() % width);
		float y = (float)(rand() % height);
		float size = 5.0f + (float)(rand() % 50);

		// Random color
		float r = (float)(rand() % 256) / 255.0f;
		float g = (float)(rand() % 256) / 255.0f;
		float b = (float)(rand() % 256) / 255.0f;
		float a = 0.3f + (float)(rand() % 40) / 100.0f;

		int shapeType = rand() % 4;

		nvgBeginPath(vg);

		switch (shapeType) {
			case 0: // Circle
				nvgCircle(vg, x, y, size);
				break;
			case 1: // Rectangle
				nvgRect(vg, x, y, size * 1.5f, size);
				break;
			case 2: // Rounded rectangle
				nvgRoundedRect(vg, x, y, size * 1.5f, size, size * 0.2f);
				break;
			case 3: // Triangle
				nvgMoveTo(vg, x, y - size);
				nvgLineTo(vg, x + size, y + size);
				nvgLineTo(vg, x - size, y + size);
				nvgClosePath(vg);
				break;
		}

		nvgFillColor(vg, nvgRGBAf(r, g, b, a));
		nvgFill(vg);

		// Some with stroke
		if (rand() % 3 == 0) {
			nvgStrokeColor(vg, nvgRGBAf(1.0f - r, 1.0f - g, 1.0f - b, a));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		}
	}

	// Add some animated gradient circles
	float pulse = sinf(time * 3.0f) * 0.5f + 0.5f;
	for (int i = 0; i < 10; i++) {
		float angle = time + i * 0.628f;
		float x = width / 2.0f + cosf(angle) * width * 0.3f;
		float y = height / 2.0f + sinf(angle) * height * 0.3f;
		float radius = 30.0f + pulse * 20.0f;

		NVGpaint paint = nvgRadialGradient(vg, x, y, radius * 0.5f, radius,
		                                     nvgRGBA(255, 200, 0, 200),
		                                     nvgRGBA(255, 0, 100, 0));

		nvgBeginPath(vg);
		nvgCircle(vg, x, y, radius);
		nvgFillPaint(vg, paint);
		nvgFill(vg);
	}
}

// Render performance stats
static void render_stats(NVGcontext* vg, int width, int height,
                         float fps, float elapsed, int frame,
                         uint32_t cacheHits, uint32_t cacheMisses,
                         uint32_t evictions, uint32_t uploads)
{
	nvgSave(vg);

	// Semi-transparent background
	nvgBeginPath(vg);
	nvgRect(vg, 10, 10, 380, 200);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 200));
	nvgFill(vg);

	// Stats text
	nvgFontSize(vg, 18);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	char buf[256];
	float y = 35;

	snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Time: %.1f / %d sec", elapsed, TEST_DURATION_SEC);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Frame: %d", frame);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Cache Hits: %u", cacheHits);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Cache Misses: %u", cacheMisses);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Evictions: %u", evictions);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	snprintf(buf, sizeof(buf), "Uploads: %u", uploads);
	nvgText(vg, 20, y, buf, NULL);
	y += 25;

	float hitRate = (cacheHits + cacheMisses) > 0 ?
	                100.0f * cacheHits / (cacheHits + cacheMisses) : 0.0f;
	snprintf(buf, sizeof(buf), "Hit Rate: %.1f%%", hitRate);
	nvgText(vg, 20, y, buf, NULL);

	nvgRestore(vg);
}

int main(void)
{
	printf("========================================\n");
	printf("1-MINUTE STRESS TEST\n");
	printf("========================================\n");
	printf("Duration: %d seconds\n", TEST_DURATION_SEC);
	printf("Target FPS: %d\n", TARGET_FPS);
	printf("Per frame: 100 text items + 200 shapes\n");
	printf("Goal: Trigger defragmentation\n");
	printf("Watch for [DEFRAG] and [ATLAS] output\n");
	printf("========================================\n\n");

	srand((unsigned int)time(NULL));

	// Create window
	WindowVulkanContext* winCtx = window_create_context(1280, 720, "1-Minute Stress Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("✓ Window created (1280x720)\n");

	// Create NanoVG context
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ NanoVG context created\n");
	printf("  (Compute defragmentation auto-enabled if available)\n");

	// Load fonts
	int fontNormal = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (fontNormal == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Font loaded\n");

	nvgFontFace(vg, "sans");

	printf("\nStarting stress test...\n");
	printf("Press Ctrl+C to stop early\n\n");

	// Timing
	double startTime = get_time_sec();
	double lastFpsTime = startTime;
	double lastFrameTime = startTime;
	int frameCount = 0;
	int fpsFrameCount = 0;
	float fps = 0.0f;

	// Test loop
	while (!window_should_close(winCtx)) {
		double currentTime = get_time_sec();
		double elapsed = currentTime - startTime;

		// Check if test duration exceeded
		if (elapsed >= TEST_DURATION_SEC) {
			printf("\n✓ Test duration reached (%d seconds)\n", TEST_DURATION_SEC);
			break;
		}

		// FPS calculation (every 0.5 seconds)
		if (currentTime - lastFpsTime >= 0.5) {
			fps = fpsFrameCount / (currentTime - lastFpsTime);
			fpsFrameCount = 0;
			lastFpsTime = currentTime;
		}

		// Frame timing
		float deltaTime = (float)(currentTime - lastFrameTime);
		lastFrameTime = currentTime;

		window_poll_events();

		// Acquire swapchain image
		uint32_t imageIndex;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

		VkResult result = vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
		                                        imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);
			continue;
		}

		// Get NanoVG's command buffer
		VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo = {0};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		int width, height;
		window_get_framebuffer_size(winCtx, &width, &height);

		// Begin render pass
		VkRenderPassBeginInfo renderPassInfo = {0};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = winCtx->renderPass;
		renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
		renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

		// Animated background color
		float hue = fmodf((float)elapsed * 20.0f, 360.0f);
		float r = 0.1f + sinf(hue * 0.01745f) * 0.1f;
		float g = 0.1f + sinf((hue + 120.0f) * 0.01745f) * 0.1f;
		float b = 0.1f + sinf((hue + 240.0f) * 0.01745f) * 0.1f;

		VkClearValue clearValues[2] = {0};
		clearValues[0].color = (VkClearColorValue){{r, g, b, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;
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

		// Notify NanoVG about render pass state (CRITICAL for pipeline binding!)
		nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, width, height, 1.0f);

		// Set font face for text rendering (must be after nvgBeginFrame)
		nvgFontFace(vg, "sans");

		// Debug: Print once to confirm text rendering is being called
		static int first_text_render = 1;
		if (first_text_render) {
			printf("[DEBUG] About to render text (frame %d)\n", frameCount);
			first_text_render = 0;
		}

		// Render ONLY shapes (temporarily disable text to debug)
		render_shape_chaos(vg, width, height, (float)elapsed);

		// Draw a simple filled rectangle to test if fills work at all
		nvgBeginPath(vg);
		nvgRect(vg, 50, 50, 200, 100);
		nvgFillColor(vg, nvgRGBA(255, 0, 255, 255));  // Magenta
		nvgFill(vg);

		// Try rendering text with explicit save/restore
		nvgSave(vg);
		nvgFontFace(vg, "sans");
		nvgFontSize(vg, 120.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 0, 255));  // Bright yellow
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, width / 2.0f, height / 2.0f, "TEXT TEST", NULL);
		nvgRestore(vg);

		// End NanoVG frame
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

		// Print progress every 5 seconds
		if (frameCount % (TARGET_FPS * 5) == 0) {
			printf("[%.1fs] Frame %d | FPS: %.1f\n",
			       elapsed, frameCount, fps);
		}

		// Frame rate limiting to 60 FPS so text is visible
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
	printf("Average FPS: %.1f\n", avgFps);

	printf("\nCheck the output above for:\n");
	printf("  [DEFRAG] messages = compute shader path used\n");
	printf("  [ATLAS] messages = defragmentation triggered\n");
	printf("========================================\n");

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
