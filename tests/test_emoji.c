#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== COLR Emoji Rendering Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "COLR Emoji Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load regular sans font
	int sansFont = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (sansFont == -1) {
		printf("Failed to load sans font: fonts/sans/NotoSans-Regular.ttf\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Sans font loaded successfully\n");

	// Load emoji font (Noto Color Emoji)
	int emojiFont = nvgCreateFont(vg, "emoji", "fonts/emoji/Noto-COLRv1.ttf");
	if (emojiFont == -1) {
		printf("Failed to load emoji font: fonts/emoji/Noto-COLRv1.ttf\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Emoji font loaded successfully\n");

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
	VkClearValue clearColor = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 800, 600, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Clear background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 800, 600);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgFill(vg);

	// Draw regular text
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, 50, 100, "Hello World!", NULL);

	// Draw text with emoji mixed in
	nvgFontSize(vg, 64.0f);
	nvgText(vg, 50, 200, "Text ", NULL);

	nvgFontFace(vg, "emoji");
	nvgFontSize(vg, 64.0f);
	const char* emoji1 = "\xF0\x9F\x98\x80";  // U+1F600 GRINNING FACE
	nvgText(vg, 150, 200, emoji1, NULL);

	nvgFontFace(vg, "sans");
	nvgText(vg, 220, 200, " and more text", NULL);

	// Draw single large emoji
	nvgFontFace(vg, "emoji");
	nvgFontSize(vg, 128.0f);
	nvgText(vg, 336, 400, emoji1, NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	// Submit and present
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSemaphores[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);

	// Save screenshot
	printf("Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "screendumps/emoji_test.ppm")) {
		printf("Screenshot saved to screendumps/emoji_test.ppm\n");
	} else {
		printf("Failed to save screenshot\n");
	}

	// Cleanup
	vkDeviceWaitIdle(winCtx->device);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("Test completed\n");
	return 0;
}
