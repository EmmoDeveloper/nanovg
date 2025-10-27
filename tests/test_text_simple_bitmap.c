#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

int main(void)
{
	printf("=== NanoVG Simple Bitmap Text Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Bitmap Text Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	// Create NanoVG context WITHOUT MSDF
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "sans", FONT_PATH);
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Render
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
	clearValues[0].color.float32[0] = 0.2f;
	clearValues[0].color.float32[1] = 0.2f;
	clearValues[0].color.float32[2] = 0.25f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	VkViewport viewport = {0};
	viewport.width = (float)winCtx->swapchainExtent.width;
	viewport.height = (float)winCtx->swapchainExtent.height;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 100, 100, "Hello Bitmap!", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	window_save_screenshot(winCtx, imageIndex, "text_bitmap_test.ppm");
	printf("Screenshot saved to text_bitmap_test.ppm\n");

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Test PASSED ===\n");
	return 0;
}
