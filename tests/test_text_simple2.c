#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Shapes Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Shapes Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");

	// Acquire swapchain image
	printf("3. Acquiring swapchain image...\n");
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	printf("   ✓ Image acquired (index: %u)\n\n", imageIndex);

	// Get command buffer from NanoVG
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	// Begin command buffer
	printf("4. Beginning command buffer...\n");
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
	clearValues[1].depthStencil.stencil = 0;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport and scissor
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
	printf("   ✓ Render pass begun\n\n");

	// Begin NanoVG frame and draw
	printf("5. Drawing with NanoVG...\n");
	nvgBeginFrame(vg, 800, 600, 1.0f);

	nvgBeginPath(vg);
	nvgCircle(vg, 400, 300, 80);
	nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("   ✓ Circle drawn\n\n");

	// End render pass and command buffer
	printf("6. Ending render pass...\n");
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
	printf("   ✓ Submitted and rendered\n\n");

	// Save screenshot
	printf("7. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/shapes_test.ppm")) {
		printf("   ✓ Screenshot saved to shapes_test.ppm\n\n");
	}

	// Cleanup
	printf("8. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Shapes Test PASSED ===\n");
	return 0;
}
