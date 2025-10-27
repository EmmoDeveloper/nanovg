#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Multi-Shape Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Multi-Shape Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

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
	clearValues[0].color = (VkClearColorValue){{0.2f, 0.2f, 0.2f, 1.0f}};
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

	// Draw with NanoVG
	printf("Drawing multiple shapes...\n");
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Red circle
	nvgBeginPath(vg);
	nvgCircle(vg, 200, 200, 80);
	nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
	nvgFill(vg);

	// Green rectangle
	nvgBeginPath(vg);
	nvgRect(vg, 350, 150, 150, 100);
	nvgFillColor(vg, nvgRGBA(0, 255, 0, 255));
	nvgFill(vg);

	// Blue rounded rectangle with stroke
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 100, 350, 200, 150, 10);
	nvgFillColor(vg, nvgRGBA(0, 0, 255, 200));
	nvgFill(vg);
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgStrokeWidth(vg, 3.0f);
	nvgStroke(vg);

	// Yellow circle with gradient
	nvgBeginPath(vg);
	nvgCircle(vg, 600, 400, 100);
	NVGpaint gradient = nvgLinearGradient(vg, 550, 350, 650, 450,
	                                       nvgRGBA(255, 255, 0, 255),
	                                       nvgRGBA(255, 128, 0, 255));
	nvgFillPaint(vg, gradient);
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("All shapes drawn successfully!\n\n");

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
	if (window_save_screenshot(winCtx, imageIndex, "multi_shapes_test.ppm")) {
		printf("✓ Screenshot saved to multi_shapes_test.ppm\n\n");
	}

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Multi-Shape Test PASSED ===\n");
	return 0;
}
