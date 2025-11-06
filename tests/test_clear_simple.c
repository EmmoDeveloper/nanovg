#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== Simple Clear Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Clear Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Allocate command buffer
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = winCtx->commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuf;
	vkAllocateCommandBuffers(winCtx->device, &cmdAllocInfo, &cmdBuf);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	// Begin render pass with RED clear color
	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearValues[2] = {0};
	clearValues[0].color = (VkClearColorValue){{1.0f, 0.0f, 0.0f, 1.0f}};  // RED
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Don't draw anything, just end render pass
	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	// Submit
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

	printf("Rendering complete, saving screenshot...\n");

	// Save screenshot
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/clear_test.ppm")) {
		printf("Screenshot saved to clear_test.ppm\n");
	}

	// Cleanup
	vkFreeCommandBuffers(winCtx->device, winCtx->commandPool, 1, &cmdBuf);
	window_destroy_context(winCtx);

	printf("=== Clear Test COMPLETE ===\n");
	return 0;
}
