#include <stdio.h>
#include <stdlib.h>
#include "nanovg/nanovg.h"
#include "backends/vulkan/nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== LCD Subpixel Rendering Test ===\n\n");
	printf("This test displays text in different rendering modes:\n");
	printf("  1. Grayscale (no subpixel)\n");
	printf("  2. RGB subpixel\n");
	printf("  3. BGR subpixel\n");
	printf("\nLook closely at the text edges to see which mode looks sharpest.\n");
	printf("The best mode will have crisp edges without color fringing.\n\n");

	WindowVulkanContext* winCtx = window_create_context(1400, 900, "LCD Subpixel Rendering Test");
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
		printf("Could not load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	const char* testText = "The quick brown fox jumps over the lazy dog";
	const char* testText2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789";

	// Render once and save screenshot
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
	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{0.98f, 0.98f, 0.98f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1400, 900, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1400, 900, 1.0f);

	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1400, 900);
	nvgFillColor(vg, nvgRGBA(250, 250, 250, 255));
	nvgFill(vg);

	nvgFontFace(vg, "sans");

	// Section 1: Grayscale (no subpixel)
	nvgTextSubpixelMode(vg, 0);  // NVG_SUBPIXEL_NONE
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, 50, 100, "1. GRAYSCALE (No Subpixel)", NULL);

	nvgFontSize(vg, 72.0f);
	nvgFillColor(vg, nvgRGBA(40, 40, 40, 255));
	nvgText(vg, 50, 180, testText, NULL);
	nvgText(vg, 50, 240, testText2, NULL);

	// Section 2: RGB subpixel
	nvgTextSubpixelMode(vg, 1);  // NVG_SUBPIXEL_RGB
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, 50, 380, "2. RGB SUBPIXEL", NULL);

	nvgFontSize(vg, 72.0f);
	nvgFillColor(vg, nvgRGBA(40, 40, 40, 255));
	nvgText(vg, 50, 460, testText, NULL);
	nvgText(vg, 50, 520, testText2, NULL);

	// Section 3: BGR subpixel
	nvgTextSubpixelMode(vg, 2);  // NVG_SUBPIXEL_BGR
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, 50, 660, "3. BGR SUBPIXEL", NULL);

	nvgFontSize(vg, 72.0f);
	nvgFillColor(vg, nvgRGBA(40, 40, 40, 255));
	nvgText(vg, 50, 740, testText, NULL);
	nvgText(vg, 50, 800, testText2, NULL);

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

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = {winCtx->swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(winCtx->presentQueue, &presentInfo);
	vkQueueWaitIdle(winCtx->presentQueue);

	// Save screenshot
	window_save_screenshot(winCtx, imageIndex, "screendumps/test_subpixel_rendering.png");
	printf("\nScreenshot saved to screendumps/test_subpixel_rendering.ppm\n");
	printf("Compare the three sections to see which subpixel mode looks best on your display.\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
