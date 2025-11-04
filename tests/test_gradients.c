#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Gradient Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(900, 700, "Gradient Test");
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
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 900.0f;
	viewport.height = 700.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Draw with NanoVG
	printf("Drawing gradient types...\n");
	nvgBeginFrame(vg, 900, 700, 1.0f);

	// Row 1: Linear gradients
	// Horizontal linear gradient
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 200, 150);
	NVGpaint linGrad1 = nvgLinearGradient(vg, 50, 125, 250, 125,
	                                       nvgRGBA(255, 0, 0, 255),
	                                       nvgRGBA(0, 0, 255, 255));
	nvgFillPaint(vg, linGrad1);
	nvgFill(vg);

	// Diagonal linear gradient
	nvgBeginPath(vg);
	nvgRect(vg, 300, 50, 200, 150);
	NVGpaint linGrad2 = nvgLinearGradient(vg, 300, 50, 500, 200,
	                                       nvgRGBA(0, 255, 0, 255),
	                                       nvgRGBA(255, 255, 0, 255));
	nvgFillPaint(vg, linGrad2);
	nvgFill(vg);

	// Vertical linear gradient
	nvgBeginPath(vg);
	nvgRect(vg, 550, 50, 200, 150);
	NVGpaint linGrad3 = nvgLinearGradient(vg, 650, 50, 650, 200,
	                                       nvgRGBA(255, 0, 255, 255),
	                                       nvgRGBA(0, 255, 255, 255));
	nvgFillPaint(vg, linGrad3);
	nvgFill(vg);

	// Row 2: Radial gradients
	// Small radial gradient
	nvgBeginPath(vg);
	nvgCircle(vg, 150, 350, 80);
	NVGpaint radGrad1 = nvgRadialGradient(vg, 150, 350, 20, 80,
	                                       nvgRGBA(255, 255, 255, 255),
	                                       nvgRGBA(255, 0, 0, 255));
	nvgFillPaint(vg, radGrad1);
	nvgFill(vg);

	// Large radial gradient
	nvgBeginPath(vg);
	nvgCircle(vg, 400, 350, 80);
	NVGpaint radGrad2 = nvgRadialGradient(vg, 400, 350, 0, 80,
	                                       nvgRGBA(255, 255, 0, 255),
	                                       nvgRGBA(255, 128, 0, 0));
	nvgFillPaint(vg, radGrad2);
	nvgFill(vg);

	// Offset radial gradient
	nvgBeginPath(vg);
	nvgCircle(vg, 650, 350, 80);
	NVGpaint radGrad3 = nvgRadialGradient(vg, 630, 330, 10, 100,
	                                       nvgRGBA(0, 255, 255, 255),
	                                       nvgRGBA(0, 0, 128, 255));
	nvgFillPaint(vg, radGrad3);
	nvgFill(vg);

	// Row 3: Box gradients
	// Sharp box gradient
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 50, 500, 200, 150, 10);
	NVGpaint boxGrad1 = nvgBoxGradient(vg, 50, 500, 200, 150, 10, 20,
	                                     nvgRGBA(128, 0, 255, 255),
	                                     nvgRGBA(0, 0, 0, 255));
	nvgFillPaint(vg, boxGrad1);
	nvgFill(vg);

	// Soft box gradient
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 300, 500, 200, 150, 20);
	NVGpaint boxGrad2 = nvgBoxGradient(vg, 300, 500, 200, 150, 20, 40,
	                                     nvgRGBA(255, 200, 0, 255),
	                                     nvgRGBA(255, 100, 0, 128));
	nvgFillPaint(vg, boxGrad2);
	nvgFill(vg);

	// Wide box gradient
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 550, 500, 200, 150, 30);
	NVGpaint boxGrad3 = nvgBoxGradient(vg, 550, 500, 200, 150, 30, 60,
	                                     nvgRGBA(0, 200, 100, 255),
	                                     nvgRGBA(0, 50, 150, 255));
	nvgFillPaint(vg, boxGrad3);
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("All gradients drawn successfully!\n\n");

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
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/gradients_test.ppm")) {
		printf("✓ Screenshot saved to build/test/screendumps/gradients_test.ppm\n\n");
	}

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Gradient Test PASSED ===\n");
	return 0;
}
