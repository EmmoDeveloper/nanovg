#include "window_utils.h"
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	printf("=== NanoVG Blend Modes Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Blend Modes Test");
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

	// Render
	printf("3. Rendering blend modes test...\n");

	uint32_t imageIndex;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Get command buffer from NanoVG
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
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
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

	// Begin NanoVG frame
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Row 1: Base layer background gradient
	for (int i = 0; i < 6; i++) {
		float x = i * 130 + 15;
		nvgBeginPath(vg);
		nvgRect(vg, x, 50, 120, 80);
		nvgFillColor(vg, nvgRGBAf(0.3f, 0.3f, 0.5f, 1.0f));
		nvgFill(vg);
	}

	// Row 1: Overlapping rectangles testing different blend modes

	// NVG_SOURCE_OVER (default)
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 20, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgBeginPath(vg);
	nvgRect(vg, 50, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_SOURCE_IN
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 150, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_IN);
	nvgBeginPath(vg);
	nvgRect(vg, 180, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_SOURCE_OUT
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 280, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OUT);
	nvgBeginPath(vg);
	nvgRect(vg, 310, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_ATOP
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 410, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_ATOP);
	nvgBeginPath(vg);
	nvgRect(vg, 440, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_DESTINATION_OVER
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 540, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 570, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_DESTINATION_IN
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 670, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_IN);
	nvgBeginPath(vg);
	nvgRect(vg, 700, 60, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// Row 2: More blend modes
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	for (int i = 0; i < 6; i++) {
		float x = i * 130 + 15;
		nvgBeginPath(vg);
		nvgRect(vg, x, 180, 120, 80);
		nvgFillColor(vg, nvgRGBAf(0.5f, 0.3f, 0.3f, 1.0f));
		nvgFill(vg);
	}

	// NVG_DESTINATION_OUT
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 20, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OUT);
	nvgBeginPath(vg);
	nvgRect(vg, 50, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_DESTINATION_ATOP
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 150, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_ATOP);
	nvgBeginPath(vg);
	nvgRect(vg, 180, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_LIGHTER (additive)
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 280, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
	nvgBeginPath(vg);
	nvgRect(vg, 310, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_COPY
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 410, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_COPY);
	nvgBeginPath(vg);
	nvgRect(vg, 440, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// NVG_XOR
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 540, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.7f));
	nvgFill(vg);
	nvgGlobalCompositeOperation(vg, NVG_XOR);
	nvgBeginPath(vg);
	nvgRect(vg, 570, 190, 60, 60);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.7f));
	nvgFill(vg);

	// Complex test: Multiple overlapping shapes with different blend modes
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 100, 350, 200, 200);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.5f));
	nvgFill(vg);

	nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
	nvgBeginPath(vg);
	nvgRect(vg, 150, 400, 200, 200);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.5f));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 200, 350, 200, 200);
	nvgFillColor(vg, nvgRGBAf(0.0f, 0.0f, 1.0f, 0.5f));
	nvgFill(vg);

	// Opacity test
	nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 450, 350, 100, 180);
	nvgFillColor(vg, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 470, 370, 100, 180);
	nvgFillColor(vg, nvgRGBAf(1.0f, 0.0f, 0.0f, 0.25f));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 490, 390, 100, 180);
	nvgFillColor(vg, nvgRGBAf(0.0f, 1.0f, 0.0f, 0.5f));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 510, 410, 100, 180);
	nvgFillColor(vg, nvgRGBAf(0.0f, 0.0f, 1.0f, 0.75f));
	nvgFill(vg);

	nvgEndFrame(vg);

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

	printf("   ✓ Blend modes rendered\n\n");

	// Save screenshot
	printf("4. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/blending_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/blending_test.ppm\n\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Blending Test PASSED ===\n");
	printf("\nBlend modes tested:\n");
	printf("  NVG_SOURCE_OVER (default)\n");
	printf("  NVG_SOURCE_IN\n");
	printf("  NVG_SOURCE_OUT\n");
	printf("  NVG_ATOP\n");
	printf("  NVG_DESTINATION_OVER\n");
	printf("  NVG_DESTINATION_IN\n");
	printf("  NVG_DESTINATION_OUT\n");
	printf("  NVG_DESTINATION_ATOP\n");
	printf("  NVG_LIGHTER (additive)\n");
	printf("  NVG_COPY\n");
	printf("  NVG_XOR\n");

	return 0;
}
