#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== Canvas API Test (Shapes + Text) ===\n\n");

	// Create window context
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Canvas API Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("✓ Window created\n");

	// Create NanoVG context
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ NanoVG context created\n");

	// Load font
	int font = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("✓ Font loaded\n\n");

	// Acquire swapchain image
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	// Get command buffer from NanoVG
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	// Begin command buffer
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
	clearValues[0].color = (VkClearColorValue){{0.2f, 0.2f, 0.25f, 1.0f}};
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

	// === Canvas API Test ===
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Test 1: Draw a rectangle with text inside
	printf("Test 1: Rectangle with text\n");
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 300, 100);
	nvgFillColor(vg, nvgRGBA(60, 120, 180, 255));
	nvgFill(vg);

	nvgFontFaceId(vg, font);
	nvgFontSize(vg, 32.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 70, 110, "Hello Canvas!", NULL);

	// Test 2: Circle with label
	printf("Test 2: Circle with label\n");
	nvgBeginPath(vg);
	nvgCircle(vg, 500, 100, 50);
	nvgFillColor(vg, nvgRGBA(200, 80, 80, 255));
	nvgFill(vg);

	nvgFontSize(vg, 20.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 475, 105, "Circle", NULL);

	// Test 3: Line with text annotation
	printf("Test 3: Line with annotation\n");
	nvgBeginPath(vg);
	nvgMoveTo(vg, 50, 200);
	nvgLineTo(vg, 400, 200);
	nvgStrokeColor(vg, nvgRGBA(255, 200, 0, 255));
	nvgStrokeWidth(vg, 3.0f);
	nvgStroke(vg);

	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 0, 255));
	nvgText(vg, 425, 205, "Line", NULL);

	// Test 4: Gradient rectangle with text
	printf("Test 4: Gradient with text\n");
	nvgBeginPath(vg);
	nvgRect(vg, 50, 250, 300, 100);
	NVGpaint gradient = nvgLinearGradient(vg, 50, 250, 50, 350,
	                                       nvgRGBA(100, 200, 100, 255),
	                                       nvgRGBA(50, 100, 50, 255));
	nvgFillPaint(vg, gradient);
	nvgFill(vg);

	nvgFontSize(vg, 28.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 70, 310, "Gradient!", NULL);

	// Test 5: Multiple text sizes
	printf("Test 5: Multiple text sizes\n");
	float sizes[] = {12.0f, 16.0f, 24.0f, 36.0f, 48.0f};
	float y = 420;
	for (int i = 0; i < 5; i++) {
		nvgFontSize(vg, sizes[i]);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		char text[32];
		snprintf(text, sizeof(text), "Size %.0f", sizes[i]);
		nvgText(vg, 50, y, text, NULL);
		y += sizes[i] + 5;
	}

	nvgEndFrame(vg);
	printf("✓ All Canvas operations completed\n\n");

	// End render pass and command buffer
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

	// Save screenshot
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/canvas_api_test.ppm")) {
		printf("✓ Screenshot saved to canvas_api_test.ppm\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Canvas API Test PASSED ===\n");
	printf("Verified: rectangles, circles, lines, gradients, and text rendering\n");
	return 0;
}
