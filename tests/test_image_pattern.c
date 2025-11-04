#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("=== NanoVG Image Pattern Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Image Pattern Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Create a simple test pattern image (64x64 checkerboard)
	int imgWidth = 64, imgHeight = 64;
	unsigned char* imgData = (unsigned char*)malloc(imgWidth * imgHeight * 4);
	for (int y = 0; y < imgHeight; y++) {
		for (int x = 0; x < imgWidth; x++) {
			int idx = (y * imgWidth + x) * 4;
			int checker = ((x / 8) + (y / 8)) % 2;
			unsigned char color = checker ? 255 : 64;
			imgData[idx + 0] = color;  // R
			imgData[idx + 1] = color;  // G
			imgData[idx + 2] = color;  // B
			imgData[idx + 3] = 255;    // A
		}
	}

	// Create NanoVG image from data
	int image = nvgCreateImageRGBA(vg, imgWidth, imgHeight, 0, imgData);
	free(imgData);

	if (image == 0) {
		printf("Failed to create image\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Created checkerboard pattern image (id: %d)\n", image);

	// Acquire swapchain image
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Get command buffer and begin
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);
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

	VkClearValue clearValues[2] = {0};
	clearValues[0].color = (VkClearColorValue){{0.2f, 0.2f, 0.3f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Draw with NanoVG
	printf("Drawing shapes with image patterns...\n");
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Test 1: Rectangle with image pattern (no rotation)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 200, 150);
	NVGpaint imgPattern1 = nvgImagePattern(vg, 50, 50, 200, 150, 0, image, 1.0f);
	nvgFillPaint(vg, imgPattern1);
	nvgFill(vg);

	// Test 2: Circle with image pattern (no rotation)
	nvgBeginPath(vg);
	nvgCircle(vg, 450, 125, 80);
	NVGpaint imgPattern2 = nvgImagePattern(vg, 370, 45, 160, 160, 0, image, 1.0f);
	nvgFillPaint(vg, imgPattern2);
	nvgFill(vg);

	// Test 3: Rectangle with rotated pattern (45 degrees)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 250, 200, 150);
	NVGpaint imgPattern3 = nvgImagePattern(vg, 150, 325, 200, 150, 45.0f * 3.14159f / 180.0f, image, 1.0f);
	nvgFillPaint(vg, imgPattern3);
	nvgFill(vg);

	// Test 4: Rounded rectangle with pattern and alpha
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 300, 250, 200, 150, 20);
	NVGpaint imgPattern4 = nvgImagePattern(vg, 300, 250, 200, 150, 0, image, 0.5f);
	nvgFillPaint(vg, imgPattern4);
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("All patterns drawn successfully!\n\n");

	// End render pass and submit
	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);
	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	// Save screenshot
	printf("Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/image_pattern_test.ppm")) {
		printf("✓ Screenshot saved to image_pattern_test.ppm\n\n");
	}

	nvgDeleteImage(vg, image);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Image Pattern Test PASSED ===\n");
	return 0;
}
