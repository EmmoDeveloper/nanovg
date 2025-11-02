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
	printf("CJK STRESS TEST\n");
	printf("========================================\n");
	printf("Duration: 60 seconds\n");
	printf("Random CJK characters at high volume\n");
	printf("Goal: Stress atlas with large character set\n");
	printf("========================================\n\n");

	// Seed random
	srand(time(NULL));

	// Create window
	WindowVulkanContext* winCtx = window_create_context(1280, 720, "CJK Stress Test");
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

	// Load CJK fonts - each regional variant contains overlapping coverage
	// JP version has best Hiragana/Katakana, SC has Chinese, KR has Hangul
	// We'll use JP as primary since it has good coverage of all CJK
	int fontCJK = nvgCreateFont(vg, "cjk", "fonts/cjk/NotoSansCJKjp-Regular.otf");
	if (fontCJK == -1) {
		fprintf(stderr, "Failed to load CJK font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ CJK font loaded (Japanese variant with full CJK coverage)\n\n");

	// ========================================
	// PRE-INITIALIZATION PHASE
	// ========================================
	printf("========================================\n");
	printf("PRE-INITIALIZATION PHASE\n");
	printf("========================================\n");
	printf("Warming up CJK font system by pre-rendering representative glyphs...\n\n");

	// Begin an initialization frame
	uint32_t initImageIndex;
	VkSemaphore initSemaphore;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &initSemaphore);
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      initSemaphore, VK_NULL_HANDLE, &initImageIndex);

	VkCommandBuffer initCmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo initBeginInfo = {0};
	initBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	initBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(initCmd, &initBeginInfo);

	VkRenderPassBeginInfo initRenderPassInfo = {0};
	initRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	initRenderPassInfo.renderPass = winCtx->renderPass;
	initRenderPassInfo.framebuffer = winCtx->framebuffers[initImageIndex];
	initRenderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue initClearValues[2] = {0};
	initClearValues[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
	initClearValues[1].depthStencil.depth = 1.0f;
	initRenderPassInfo.clearValueCount = 2;
	initRenderPassInfo.pClearValues = initClearValues;
	vkCmdBeginRenderPass(initCmd, &initRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport initViewport = {0};
	initViewport.width = 1280.0f;
	initViewport.height = 720.0f;
	initViewport.maxDepth = 1.0f;
	vkCmdSetViewport(initCmd, 0, 1, &initViewport);

	VkRect2D initScissor = {0};
	initScissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(initCmd, 0, 1, &initScissor);

	nvgVkBeginRenderPass(vg, &initRenderPassInfo, initViewport, initScissor);
	nvgBeginFrame(vg, 1280, 720, 1.0f);

	nvgFontFace(vg, "cjk");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	// Pre-render ALL CJK glyphs - CJK is the given, the foundation
	// This reflects reality for billions of users
	char utf8Buf[5];
	int totalGlyphs = 0;
	float x = 10.0f, y = 10.0f;

	printf("Pre-rendering ALL CJK characters (full character sets)...\n");
	printf("CJK is the foundation - not an afterthought.\n\n");

	double phaseStart = get_time_sec();

	// Chinese: ALL characters from U+4E00 to U+9FFF (20,992 characters)
	printf("Loading Chinese characters (U+4E00 to U+9FFF)...\n");
	int chineseCount = 0;
	for (unsigned int cp = 0x4E00; cp <= 0x9FFF; cp++) {
		utf8Buf[0] = (char)(0xE0 | (cp >> 12));
		utf8Buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		utf8Buf[2] = (char)(0x80 | (cp & 0x3F));
		utf8Buf[3] = '\0';
		nvgText(vg, x, y, utf8Buf, NULL);
		x += 60.0f;
		if (x > 1200.0f) {
			x = 10.0f;
			y += 60.0f;
		}
		chineseCount++;
		if (chineseCount % 2000 == 0) {
			printf("  Progress: %d Chinese characters loaded...\n", chineseCount);
		}
	}
	printf("  ✓ %d Chinese characters loaded\n", chineseCount);
	totalGlyphs += chineseCount;

	// Hiragana: ALL characters
	printf("Loading Hiragana characters (U+3040 to U+309F)...\n");
	int hiraganaCount = 0;
	for (unsigned int cp = 0x3040; cp <= 0x309F; cp++) {
		utf8Buf[0] = (char)(0xE0 | (cp >> 12));
		utf8Buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		utf8Buf[2] = (char)(0x80 | (cp & 0x3F));
		utf8Buf[3] = '\0';
		nvgText(vg, x, y, utf8Buf, NULL);
		x += 60.0f;
		if (x > 1200.0f) {
			x = 10.0f;
			y += 60.0f;
		}
		hiraganaCount++;
	}
	printf("  ✓ %d Hiragana characters loaded\n", hiraganaCount);
	totalGlyphs += hiraganaCount;

	// Katakana: ALL characters
	printf("Loading Katakana characters (U+30A0 to U+30FF)...\n");
	int katakanaCount = 0;
	for (unsigned int cp = 0x30A0; cp <= 0x30FF; cp++) {
		utf8Buf[0] = (char)(0xE0 | (cp >> 12));
		utf8Buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		utf8Buf[2] = (char)(0x80 | (cp & 0x3F));
		utf8Buf[3] = '\0';
		nvgText(vg, x, y, utf8Buf, NULL);
		x += 60.0f;
		if (x > 1200.0f) {
			x = 10.0f;
			y += 60.0f;
		}
		katakanaCount++;
	}
	printf("  ✓ %d Katakana characters loaded\n", katakanaCount);
	totalGlyphs += katakanaCount;

	// Hangul: ALL characters from U+AC00 to U+D7AF (11,172 characters)
	printf("Loading Hangul characters (U+AC00 to U+D7AF)...\n");
	int hangulCount = 0;
	for (unsigned int cp = 0xAC00; cp <= 0xD7AF; cp++) {
		utf8Buf[0] = (char)(0xE0 | (cp >> 12));
		utf8Buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		utf8Buf[2] = (char)(0x80 | (cp & 0x3F));
		utf8Buf[3] = '\0';
		nvgText(vg, x, y, utf8Buf, NULL);
		x += 60.0f;
		if (x > 1200.0f) {
			x = 10.0f;
			y += 60.0f;
		}
		hangulCount++;
		if (hangulCount % 2000 == 0) {
			printf("  Progress: %d Hangul characters loaded...\n", hangulCount);
		}
	}
	printf("  ✓ %d Hangul characters loaded\n", hangulCount);
	totalGlyphs += hangulCount;

	double phaseTime = get_time_sec() - phaseStart;
	printf("\n✓ CJK foundation complete: %d glyphs loaded in %.2f seconds\n", totalGlyphs, phaseTime);

	nvgEndFrame(vg);
	vkCmdEndRenderPass(initCmd);
	vkEndCommandBuffer(initCmd);

	VkSubmitInfo initSubmitInfo = {0};
	initSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	initSubmitInfo.commandBufferCount = 1;
	initSubmitInfo.pCommandBuffers = &initCmd;
	initSubmitInfo.waitSemaphoreCount = 1;
	initSubmitInfo.pWaitSemaphores = &initSemaphore;
	VkPipelineStageFlags initWaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	initSubmitInfo.pWaitDstStageMask = initWaitStages;
	vkQueueSubmit(winCtx->graphicsQueue, 1, &initSubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	VkPresentInfoKHR initPresentInfo = {0};
	initPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	initPresentInfo.swapchainCount = 1;
	initPresentInfo.pSwapchains = &winCtx->swapchain;
	initPresentInfo.pImageIndices = &initImageIndex;
	vkQueuePresentKHR(winCtx->graphicsQueue, &initPresentInfo);

	vkDestroySemaphore(winCtx->device, initSemaphore, NULL);

	printf("\n✓ Pre-initialization complete - font system fully warmed up\n");
	printf("========================================\n\n");

	printf("Starting CJK stress test...\n");
	printf("Rendering 200 persistent CJK characters (regenerated every 3 seconds)\n");
	printf("Character ranges:\n");
	printf("  - Chinese: U+4E00 to U+9FFF (20,992 chars)\n");
	printf("  - Hiragana: U+3040 to U+309F (96 chars)\n");
	printf("  - Katakana: U+30A0 to U+30FF (96 chars)\n");
	printf("  - Hangul: U+AC00 to U+D7AF (11,172 chars)\n");
	printf("\nTesting character generation:\n");

	// Test each range
	for (int r = 0; r < 4; r++) {
		unsigned int testCodepoint;
		if (r == 0) {
			testCodepoint = 0x4E00;  // 一 (Chinese "one")
			printf("  Chinese example: U+%04X\n", testCodepoint);
		} else if (r == 1) {
			testCodepoint = 0x3042;  // あ (Hiragana "a")
			printf("  Hiragana example: U+%04X\n", testCodepoint);
		} else if (r == 2) {
			testCodepoint = 0x30A2;  // ア (Katakana "a")
			printf("  Katakana example: U+%04X\n", testCodepoint);
		} else {
			testCodepoint = 0xAC00;  // 가 (Hangul "ga")
			printf("  Hangul example: U+%04X\n", testCodepoint);
		}
	}
	printf("\n");

	// Timing
	double startTime = get_time_sec();
	int frameCount = 0;
	float fps = 0.0f;
	int fpsFrameCount = 0;
	double lastFpsTime = startTime;

	int width = 1280;
	int height = 720;

	// Pre-generate character data that persists across frames
	#define NUM_CHARS 200
	struct CharData {
		float x, y, size;
		int r, g, b;
		unsigned int codepoint;
		char text[5];
	} chars[NUM_CHARS];

	// Generate initial characters
	int totalChars = 0x5200 + 96 + 96 + 0x2BB0;
	for (int i = 0; i < NUM_CHARS; i++) {
		chars[i].x = (float)(rand() % width);
		chars[i].y = (float)(rand() % height);
		chars[i].size = 24.0f + (float)(rand() % 48);
		chars[i].r = 128 + (rand() % 128);
		chars[i].g = 128 + (rand() % 128);
		chars[i].b = 128 + (rand() % 128);

		int pick = rand() % totalChars;
		if (pick < 0x5200) {
			chars[i].codepoint = 0x4E00 + pick;
		} else if (pick < 0x5200 + 96) {
			chars[i].codepoint = 0x3040 + (pick - 0x5200);
		} else if (pick < 0x5200 + 96 + 96) {
			chars[i].codepoint = 0x30A0 + (pick - 0x5200 - 96);
		} else {
			chars[i].codepoint = 0xAC00 + (pick - 0x5200 - 96 - 96);
		}

		// Convert to UTF-8
		unsigned int cp = chars[i].codepoint;
		if (cp < 0x80) {
			chars[i].text[0] = (char)cp;
			chars[i].text[1] = '\0';
		} else if (cp < 0x800) {
			chars[i].text[0] = (char)(0xC0 | (cp >> 6));
			chars[i].text[1] = (char)(0x80 | (cp & 0x3F));
			chars[i].text[2] = '\0';
		} else if (cp < 0x10000) {
			chars[i].text[0] = (char)(0xE0 | (cp >> 12));
			chars[i].text[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
			chars[i].text[2] = (char)(0x80 | (cp & 0x3F));
			chars[i].text[3] = '\0';
		}
	}

	double lastRegenTime = startTime;

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

		// Regenerate characters every 3 seconds
		if (currentTime - lastRegenTime >= 3.0) {
			for (int i = 0; i < NUM_CHARS; i++) {
				chars[i].x = (float)(rand() % width);
				chars[i].y = (float)(rand() % height);
				chars[i].size = 24.0f + (float)(rand() % 48);
				chars[i].r = 128 + (rand() % 128);
				chars[i].g = 128 + (rand() % 128);
				chars[i].b = 128 + (rand() % 128);

				int pick = rand() % totalChars;
				if (pick < 0x5200) {
					chars[i].codepoint = 0x4E00 + pick;
				} else if (pick < 0x5200 + 96) {
					chars[i].codepoint = 0x3040 + (pick - 0x5200);
				} else if (pick < 0x5200 + 96 + 96) {
					chars[i].codepoint = 0x30A0 + (pick - 0x5200 - 96);
				} else {
					chars[i].codepoint = 0xAC00 + (pick - 0x5200 - 96 - 96);
				}

				// Convert to UTF-8
				unsigned int cp = chars[i].codepoint;
				if (cp < 0x80) {
					chars[i].text[0] = (char)cp;
					chars[i].text[1] = '\0';
				} else if (cp < 0x800) {
					chars[i].text[0] = (char)(0xC0 | (cp >> 6));
					chars[i].text[1] = (char)(0x80 | (cp & 0x3F));
					chars[i].text[2] = '\0';
				} else if (cp < 0x10000) {
					chars[i].text[0] = (char)(0xE0 | (cp >> 12));
					chars[i].text[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
					chars[i].text[2] = (char)(0x80 | (cp & 0x3F));
					chars[i].text[3] = '\0';
				}
			}
			lastRegenTime = currentTime;
		}

		// Set font
		nvgFontFace(vg, "cjk");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		// Render persistent characters
		for (int i = 0; i < NUM_CHARS; i++) {
			nvgFontSize(vg, chars[i].size);
			nvgFillColor(vg, nvgRGBA(chars[i].r, chars[i].g, chars[i].b, 255));
			nvgText(vg, chars[i].x, chars[i].y, chars[i].text, NULL);
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
	printf("Total character draw calls: ~%d\n", frameCount * 200);
	printf("Potential unique glyphs: 32,356\n");
	printf("Average FPS: %.1f\n", avgFps);
	printf("\nCheck output for [DEFRAG] and [ATLAS] messages\n");
	printf("========================================\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
