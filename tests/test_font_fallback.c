#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "../src/tools/window_utils.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
	printf("=== NanoVG Font Fallback Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 800, "Font Fallback Test");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);

	// Load base font (Latin)
	int fontLatin = nvgCreateFont(vg, "latin", "fonts/sans/NotoSans-Regular.ttf");
	if (fontLatin == -1) {
		fprintf(stderr, "Failed to load Latin font\n");
		return 1;
	}

	// Load fallback fonts
	int fontCJK = nvgCreateFont(vg, "cjk", "fonts/cjk/NotoSansCJKsc-Regular.otf");
	int fontArabic = nvgCreateFont(vg, "arabic", "fonts/arabic/NotoSansArabic-Regular.ttf");
	int fontEmoji = nvgCreateFont(vg, "emoji", "fonts/emoji/NotoColorEmoji.ttf");

	if (fontCJK == -1 || fontArabic == -1 || fontEmoji == -1) {
		fprintf(stderr, "Failed to load fallback fonts\n");
		return 1;
	}

	// Set up fallback chain: Latin -> CJK -> Arabic -> Emoji
	nvgAddFallbackFontId(vg, fontLatin, fontCJK);
	nvgAddFallbackFontId(vg, fontLatin, fontArabic);
	nvgAddFallbackFontId(vg, fontLatin, fontEmoji);

	printf("Loaded fonts:\n");
	printf("  Latin:  %d\n", fontLatin);
	printf("  CJK:    %d\n", fontCJK);
	printf("  Arabic: %d\n", fontArabic);
	printf("  Emoji:  %d\n\n", fontEmoji);

	// Prepare rendering
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                                        winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                                        VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to acquire swapchain image\n");
		return 1;
	}

	// Get NanoVG's command buffer
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

	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{1.0f, 1.0f, 1.0f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport and scissor
	VkViewport viewport = {0, 0, (float)winCtx->swapchainExtent.width, (float)winCtx->swapchainExtent.height, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Tell NanoVG about the render pass
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Begin NanoVG frame
	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Title
	nvgFontSize(vg, 36.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, 60, "Font Fallback Test - Multilingual Text", NULL);

	// Test different scripts with fallback
	float y = 150;
	nvgFontSize(vg, 32.0f);

	// English (Latin) - should use base font
	nvgFillColor(vg, nvgRGBA(255, 255, 128, 255));
	nvgText(vg, 50, y, "English: Hello World!", NULL);
	y += 60;

	// Chinese (CJK) - should fallback to CJK font
	nvgFillColor(vg, nvgRGBA(128, 255, 128, 255));
	nvgText(vg, 50, y, "Chinese: 你好世界", NULL);
	y += 60;

	// Japanese (CJK) - should fallback to CJK font
	nvgFillColor(vg, nvgRGBA(128, 255, 255, 255));
	nvgText(vg, 50, y, "Japanese: こんにちは世界", NULL);
	y += 60;

	// Arabic - should fallback to Arabic font
	nvgFillColor(vg, nvgRGBA(255, 128, 255, 255));
	nvgText(vg, 50, y, "Arabic: مرحبا بالعالم", NULL);
	y += 60;

	// Mixed: English + Chinese
	nvgFillColor(vg, nvgRGBA(255, 200, 128, 255));
	nvgText(vg, 50, y, "Mixed: Hello 世界", NULL);
	y += 60;

	// Mixed: English + Arabic
	nvgFillColor(vg, nvgRGBA(200, 255, 200, 255));
	nvgText(vg, 50, y, "Mixed: Hello مرحبا", NULL);
	y += 60;

	// Emoji (if supported)
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, y, "Emoji: 😀 🎉 ❤️", NULL);
	y += 60;

	// Full mix
	nvgFontSize(vg, 28.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 255, 255));
	nvgText(vg, 50, y, "All: Hello 你好 مرحبا 😀", NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/font_fallback_test.png");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Font Fallback Test PASSED ===\n");
	return 0;
}
