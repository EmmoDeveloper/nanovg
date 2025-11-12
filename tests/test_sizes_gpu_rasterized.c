#include <stdio.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"
#include "../src/font/nvg_font.h"

int main(void) {
	printf("=== Multiple Font Sizes Test (GPU Rasterized) ===\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "Font Sizes GPU");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (font == -1) {
		printf("Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	NVGFontSystem* fs = (NVGFontSystem*)nvgGetFontSystem(vg);
	void* vkContext = nvgInternalParams(vg)->userPtr;

	if (nvgFont_SetRasterMode(fs, NVG_RASTER_GPU, vkContext)) {
		printf("GPU rasterization enabled (FORCED mode)\n");
	} else {
		printf("Failed to enable GPU rasterization\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	// PASS 1: Determine what glyphs are needed by calling nvgText
	// This will queue up GPU rasterization jobs
	nvgBeginFrame(vg, 1200, 900, 1.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	float y = 100;
	nvgFontSize(vg, 24);
	nvgText(vg, 50, y, "Size 24: The quick brown fox jumps over the lazy dog", NULL);
	y += 50;

	nvgFontSize(vg, 32);
	nvgText(vg, 50, y, "Size 32: The quick brown fox jumps over", NULL);
	y += 60;

	nvgFontSize(vg, 48);
	nvgText(vg, 50, y, "Size 48: The quick brown fox", NULL);
	y += 80;

	nvgFontSize(vg, 64);
	nvgText(vg, 50, y, "Size 64: Quick fox", NULL);
	y += 100;

	nvgFontSize(vg, 96);
	nvgText(vg, 50, y, "Size 96: Fox", NULL);

	nvgEndFrame(vg);

	// Flush all GPU rasterization jobs to the command buffer
	printf("=== Flushing GPU jobs before render pass ===\n");
	int flushed = nvgFont_FlushGpuRasterJobs(fs, cmd);
	printf("=== Flushed %d jobs ===\n", flushed);

	// Now begin the render pass and render the text
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

	// PASS 2: Actually render the text (glyphs are already rasterized)
	nvgBeginFrame(vg, 1200, 900, 1.0f);

	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	y = 100;
	nvgFontSize(vg, 24);
	nvgText(vg, 50, y, "Size 24: The quick brown fox jumps over the lazy dog", NULL);
	y += 50;

	nvgFontSize(vg, 32);
	nvgText(vg, 50, y, "Size 32: The quick brown fox jumps over", NULL);
	y += 60;

	nvgFontSize(vg, 48);
	nvgText(vg, 50, y, "Size 48: The quick brown fox", NULL);
	y += 80;

	nvgFontSize(vg, 64);
	nvgText(vg, 50, y, "Size 64: Quick fox", NULL);
	y += 100;

	nvgFontSize(vg, 96);
	nvgText(vg, 50, y, "Size 96: Fox", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

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

	printf("Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "screendumps/sizes_gpu_test.ppm")) {
		printf("Screenshot saved\n");
	}

	vkDeviceWaitIdle(winCtx->device);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("Test complete\n");
	return 0;
}
