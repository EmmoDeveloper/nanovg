#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("=== Simple Image Pattern Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(400, 400, "Simple Image Pattern");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Create a simple 2x2 colored image for easy debugging
	int imgWidth = 2, imgHeight = 2;
	unsigned char imgData[] = {
		255, 0, 0, 255,    // Red
		0, 255, 0, 255,    // Green
		0, 0, 255, 255,    // Blue
		255, 255, 0, 255   // Yellow
	};

	int image = nvgCreateImageRGBA(vg, imgWidth, imgHeight, 0, imgData);
	if (image == 0) {
		printf("Failed to create image\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Created 2x2 test image\n");

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
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 400.0f;
	viewport.height = 400.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Draw with NanoVG
	printf("Drawing test patterns...\n");
	nvgBeginFrame(vg, 400, 400, 1.0f);

	// Test 1: Non-rotated pattern (should show red, green, blue, yellow quadrants)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 100, 100);
	NVGpaint pat1 = nvgImagePattern(vg, 50, 50, 100, 100, 0, image, 1.0f);
	nvgFillPaint(vg, pat1);
	nvgFill(vg);

	// Test 2: Same but centered differently to understand coordinate system
	nvgBeginPath(vg);
	nvgRect(vg, 200, 50, 100, 100);
	NVGpaint pat2 = nvgImagePattern(vg, 250, 100, 100, 100, 0, image, 1.0f);
	nvgFillPaint(vg, pat2);
	nvgFill(vg);

	// Test 3: Small rotation (10 degrees)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 200, 100, 100);
	NVGpaint pat3 = nvgImagePattern(vg, 100, 250, 100, 100, 10.0f * 3.14159f / 180.0f, image, 1.0f);
	nvgFillPaint(vg, pat3);
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("Patterns drawn\n\n");

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
	if (window_save_screenshot(winCtx, imageIndex, "image_simple_test.ppm")) {
		printf("✓ Screenshot saved\n\n");
	}

	nvgDeleteImage(vg, image);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Test Complete ===\n");
	return 0;
}
