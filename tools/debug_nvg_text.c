#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "src/nvg_vk.h"
#include "src/nanovg.h"
#include "tests/window_utils.h"

int main(void)
{
	printf("=== NanoVG Text Coordinate Debug ===\n");

	// Create window
	WindowVulkanContext* winCtx = window_create_context(1280, 720, "Debug Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}
	printf("Window created: requested 1280x720\n");
	printf("Swapchain extent: %dx%d\n", winCtx->swapchainExtent.width, winCtx->swapchainExtent.height);

	// Create NanoVG
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG\n");
		window_destroy_context(winCtx);
		return 1;
	}

	// Load font
	int font = nvgCreateFont(vg, "mono", "/usr/share/fonts/truetype/freefont/FreeMono.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Render single frame
	uint32_t imageIndex;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearValues[2] = {0};
	clearValues[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport to actual swapchain size
	VkViewport viewport = {0};
	viewport.width = (float)winCtx->swapchainExtent.width;
	viewport.height = (float)winCtx->swapchainExtent.height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Begin NanoVG frame with actual swapchain dimensions
	printf("NanoVG frame: %dx%d\n", winCtx->swapchainExtent.width, winCtx->swapchainExtent.height);
	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Test different font sizes and positions
	nvgFontFace(vg, "mono");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	// Draw coordinate markers
	nvgFontSize(vg, 48.0f);
	nvgText(vg, 10, 10, "TOP-LEFT (10,10)", NULL);
	nvgText(vg, 10, 100, "Line at (10,100)", NULL);
	nvgText(vg, 10, 200, "Line at (10,200)", NULL);

	// Draw test lines
	const char* testText = "Line 1\nLine 2\nLine 3\nLine 4\nLine 5";
	nvgFontSize(vg, 24.0f);
	nvgTextLines(vg, 10, 300, testText, NULL);

	// Draw bottom marker
	nvgFontSize(vg, 48.0f);
	nvgText(vg, 10, (float)winCtx->swapchainExtent.height - 60, "BOTTOM", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.pWaitDstStageMask = waitStages;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &winCtx->swapchain;
	presentInfo.pImageIndices = &imageIndex;
	vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);

	vkQueueWaitIdle(winCtx->graphicsQueue);

	window_save_screenshot(winCtx, imageIndex, "debug_coords.ppm");
	printf("Saved screenshot to debug_coords.ppm\n");

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
