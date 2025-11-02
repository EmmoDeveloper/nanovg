#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

#define TEST_FONT_SIZE 48.0f
#define TEST_TEXT_COLOR nvgColorWhite()

int main(void)
{
	printf("=== NanoVG Baseline & Kerning Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Baseline & Kerning Test");
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

	printf("1. Testing baseline metrics query (font size: %.1f)...\\n", TEST_FONT_SIZE);
	nvgFontFaceId(vg, font);
	nvgFontSize(vg, TEST_FONT_SIZE);

	float ascender, descender, lineHeight;
	nvgFontBaseline(vg, &ascender, &descender, &lineHeight);
	printf("   Ascender: %.2f\\n", ascender);
	printf("   Descender: %.2f\\n", descender);
	printf("   Line height: %.2f\\n\\n", lineHeight);

	printf("2. Testing baseline shift...\\n");
	printf("   Default baseline shift: 0.0\\n");
	printf("   Setting baseline shift to -10.0 (shift up)\\n");
	nvgBaselineShift(vg, -10.0f);
	printf("   Setting baseline shift to 10.0 (shift down)\\n");
	nvgBaselineShift(vg, 10.0f);
	printf("   Resetting baseline shift to 0.0\\n\\n");
	nvgBaselineShift(vg, 0.0f);

	printf("3. Testing kerning control...\\n");
	float kern_AV = nvgGetKerning(vg, 'A', 'V');
	printf("   Kerning between 'A' and 'V': %.2f pixels\\n", kern_AV);
	printf("   Disabling kerning...\\n");
	nvgKerningEnabled(vg, 0);
	printf("   Re-enabling kerning...\\n");
	nvgKerningEnabled(vg, 1);
	printf("\\n");

	printf("4. Rendering test frame...\\n");

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

	// Test 1: Normal baseline
	nvgText(vg, 50, 100, "Normal baseline", NULL);
	printf("   Rendered text at normal baseline\\n");

	// Test 2: Shifted baseline (up)
	nvgBaselineShift(vg, -20.0f);
	nvgText(vg, 50, 200, "Shifted up -20px", NULL);
	printf("   Rendered text with baseline shifted up\\n");

	// Test 3: Shifted baseline (down)
	nvgBaselineShift(vg, 20.0f);
	nvgText(vg, 50, 300, "Shifted down +20px", NULL);
	printf("   Rendered text with baseline shifted down\\n");

	// Test 4: Reset baseline
	nvgBaselineShift(vg, 0.0f);

	// Test 5: Kerning enabled (default)
	nvgText(vg, 50, 400, "AVATAR (kern on)", NULL);
	printf("   Rendered text with kerning enabled\\n");

	// Test 6: Kerning disabled
	nvgKerningEnabled(vg, 0);
	nvgText(vg, 50, 500, "AVATAR (kern off)", NULL);
	printf("   Rendered text with kerning disabled\\n");

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

	printf("\\n=== Test Complete ===\\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
