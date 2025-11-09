#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "window_utils.h"

int main(void) {
	printf("=== NanoVG Glyph-Level API Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "Glyph API Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load font
	int font = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (font == -1) {
		printf("Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Test text metrics
	float ascender, descender, lineh;
	nvgFontSize(vg, 48.0f);
	nvgFontFace(vg, "sans");
	nvgTextMetrics(vg, &ascender, &descender, &lineh);
	printf("Text Metrics (48px):\n");
	printf("  Ascender:  %.2f\n", ascender);
	printf("  Descender: %.2f\n", descender);
	printf("  Line Height: %.2f\n\n", lineh);

	// Test glyph metrics for 'A'
	NVGglyphMetrics metrics;
	if (nvgGetGlyphMetrics(vg, 'A', &metrics) == 0) {
		printf("Glyph Metrics for 'A':\n");
		printf("  Glyph Index: %u\n", metrics.glyphIndex);
		printf("  Bearing X: %.2f\n", metrics.bearingX);
		printf("  Bearing Y: %.2f\n", metrics.bearingY);
		printf("  Advance X: %.2f\n", metrics.advanceX);
		printf("  Advance Y: %.2f\n", metrics.advanceY);
		printf("  Width: %.2f\n", metrics.width);
		printf("  Height: %.2f\n\n", metrics.height);
	}

	// Test kerning between 'A' and 'V'
	if (nvgGetGlyphMetrics(vg, 'A', &metrics) == 0) {
		unsigned int glyphA = metrics.glyphIndex;
		if (nvgGetGlyphMetrics(vg, 'V', &metrics) == 0) {
			unsigned int glyphV = metrics.glyphIndex;
			float kerning = nvgGetKerning(vg, glyphA, glyphV);
			printf("Kerning between 'A' and 'V': %.2f\n\n", kerning);
		}
	}

	// Render
	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearColor = {{{0.15f, 0.15f, 0.15f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1200, 900, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1200, 900, 1.0f);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1200, 900);
	nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
	nvgFill(vg);

	// Title
	nvgFontSize(vg, 32.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, 50, "Glyph-Level API Test", NULL);

	float y = 120;

	// Section 1: Text Metrics
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Text Metrics:", NULL);
	y += 30;

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	char buf[128];
	snprintf(buf, sizeof(buf), "Ascender: %.2f  Descender: %.2f  Line Height: %.2f",
	         ascender, descender, lineh);
	nvgText(vg, 70, y, buf, NULL);
	y += 60;

	// Section 2: Individual Glyph Rendering
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Individual Glyph Rendering:", NULL);
	y += 30;

	nvgFontSize(vg, 72.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	float x = 100;
	const char* text = "WAVE";
	for (int i = 0; text[i]; i++) {
		float advance = nvgRenderGlyph(vg, text[i], x, y);
		x += advance;
	}
	y += 100;

	// Section 3: Kerning Comparison
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Kerning:", NULL);
	y += 30;

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "With kerning:", NULL);
	y += 25;

	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgKerningEnabled(vg, 1);
	nvgText(vg, 70, y, "WAVE", NULL);
	y += 60;

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Without kerning:", NULL);
	y += 25;

	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 128, 128, 255));
	nvgKerningEnabled(vg, 0);
	nvgText(vg, 70, y, "WAVE", NULL);

	// Section 4: Glyph Metrics Display
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgKerningEnabled(vg, 1);
	nvgText(vg, 50, 700, "Glyph Metrics for 'A':", NULL);

	if (nvgGetGlyphMetrics(vg, 'A', &metrics) == 0) {
		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
		y = 730;
		snprintf(buf, sizeof(buf), "Bearing: (%.2f, %.2f)", metrics.bearingX, metrics.bearingY);
		nvgText(vg, 70, y, buf, NULL);
		y += 20;
		snprintf(buf, sizeof(buf), "Advance: (%.2f, %.2f)", metrics.advanceX, metrics.advanceY);
		nvgText(vg, 70, y, buf, NULL);
		y += 20;
		snprintf(buf, sizeof(buf), "Size: %.2f x %.2f", metrics.width, metrics.height);
		nvgText(vg, 70, y, buf, NULL);
	}

	// Footer
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
	nvgText(vg, 50, 870, "Glyph-level API: nvgTextMetrics, nvgGetGlyphMetrics, nvgGetKerning, nvgRenderGlyph", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSem[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSem;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSem[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSem;

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);
	vkWaitForFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame], VK_TRUE, UINT64_MAX);

	window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/glyph_api_test.ppm");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Glyph API Test PASSED ===\n");
	return 0;
}
