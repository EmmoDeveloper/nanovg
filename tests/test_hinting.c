#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

#define TEST_FONT_SIZE 48.0f
#define TEST_TEXT_COLOR nvgColorWhite()

int main(void)
{
	printf("=== NanoVG Font Hinting Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Font Hinting Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "mono", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("Testing font hinting modes...\n");
	printf("Font size: %.1f\n\n", TEST_FONT_SIZE);

	// Render one frame
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
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

	// NanoVG rendering
	nvgBeginFrame(vg, 800, 600, 1.0f);

	nvgFontFaceId(vg, font);
	nvgFontSize(vg, TEST_FONT_SIZE);
	nvgFillColor(vg, TEST_TEXT_COLOR);

	// Test 1: Default hinting
	printf("1. NVG_HINTING_DEFAULT\n");
	nvgFontHinting(vg, NVG_HINTING_DEFAULT);
	nvgText(vg, 50, 100, "Default hinting", NULL);
	printf("   Rendered text with default hinting\n");

	// Test 2: No hinting
	printf("2. NVG_HINTING_NONE\n");
	nvgFontHinting(vg, NVG_HINTING_NONE);
	nvgText(vg, 50, 200, "No hinting (raw outlines)", NULL);
	printf("   Rendered text with no hinting\n");

	// Test 3: Light hinting
	printf("3. NVG_HINTING_LIGHT\n");
	nvgFontHinting(vg, NVG_HINTING_LIGHT);
	nvgText(vg, 50, 300, "Light hinting", NULL);
	printf("   Rendered text with light hinting\n");

	// Test 4: Full hinting
	printf("4. NVG_HINTING_FULL\n");
	nvgFontHinting(vg, NVG_HINTING_FULL);
	nvgText(vg, 50, 400, "Full hinting (grid fit)", NULL);
	printf("   Rendered text with full hinting\n");

	// Test 5: Back to default
	printf("5. Reset to NVG_HINTING_DEFAULT\n");
	nvgFontHinting(vg, NVG_HINTING_DEFAULT);
	nvgText(vg, 50, 500, "Back to default", NULL);
	printf("   Rendered text with default hinting\n");

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
	vkQueueWaitIdle(winCtx->graphicsQueue);

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &winCtx->swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("\n=== Test Complete ===\n");
	printf("Hinting modes tested: DEFAULT, NONE, LIGHT, FULL\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
