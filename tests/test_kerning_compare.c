#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "window_utils.h"

int main(void) {
	printf("=== NanoVG Kerning Comparison Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 400, "Kerning Compare");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "sans", "fonts/variable/Inter/Inter-VariableFont_opsz,wght.ttf");
	if (font == -1) {
		printf("Failed to load Inter font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Measure text with and without kerning
	nvgFontSize(vg, 120.0f);
	nvgFontFace(vg, "sans");

	float bounds[4];
	nvgKerningEnabled(vg, 0);
	float widthNoKern = nvgTextBounds(vg, 0, 0, "WAVE", NULL, bounds);

	nvgKerningEnabled(vg, 1);
	float widthWithKern = nvgTextBounds(vg, 0, 0, "WAVE", NULL, bounds);

	printf("Text measurements:\n");
	printf("  WITHOUT kerning: %.2f pixels\n", widthNoKern);
	printf("  WITH kerning:    %.2f pixels\n", widthWithKern);
	printf("  Difference:      %.2f pixels\n\n", widthNoKern - widthWithKern);

	// Render side-by-side comparison
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

	VkViewport viewport = {0, 0, (float)winCtx->swapchainExtent.width, (float)winCtx->swapchainExtent.height, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height);
	nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
	nvgFill(vg);

	// LEFT: WITHOUT kerning
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(200, 100, 100, 255));
	nvgText(vg, 50, 40, "WITHOUT Kerning", NULL);

	nvgFontSize(vg, 120.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgKerningEnabled(vg, 0);
	nvgText(vg, 50, 180, "WAVE", NULL);

	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
	char info[64];
	snprintf(info, sizeof(info), "Width: %.2f px", widthNoKern);
	nvgText(vg, 50, 220, info, NULL);

	// RIGHT: WITH kerning
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(100, 200, 100, 255));
	nvgText(vg, 650, 40, "WITH Kerning", NULL);

	nvgFontSize(vg, 120.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgKerningEnabled(vg, 1);
	nvgText(vg, 650, 180, "WAVE", NULL);

	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
	snprintf(info, sizeof(info), "Width: %.2f px", widthWithKern);
	nvgText(vg, 650, 220, info, NULL);

	// Divider line
	nvgBeginPath(vg);
	nvgMoveTo(vg, 600, 0);
	nvgLineTo(vg, 600, winCtx->swapchainExtent.height);
	nvgStrokeColor(vg, nvgRGBA(80, 80, 80, 255));
	nvgStrokeWidth(vg, 2.0f);
	nvgStroke(vg);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/kerning_comparison.ppm");
	printf("Saved: kerning_comparison.ppm\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Kerning Comparison Test PASSED ===\n");
	return 0;
}
