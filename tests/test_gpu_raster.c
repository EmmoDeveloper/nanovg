#include <stdio.h>
#include <stdlib.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"
#include "../src/font/nvg_font.h"

int main(void) {
	printf("=== NanoVG GPU Rasterization Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "GPU Raster Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load font
	int font = nvgCreateFont(vg, "sans", "fonts/Roboto/Roboto-Regular.ttf");
	if (font == -1) {
		printf("Failed to load Roboto font, trying Inter...\n");
		font = nvgCreateFont(vg, "sans", "fonts/variable/Inter/Inter-VariableFont_opsz,wght.ttf");
	}
	if (font == -1) {
		printf("Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Get font system from NanoVG context
	NVGFontSystem* fs = (NVGFontSystem*)nvgGetFontSystem(vg);
	if (!fs) {
		printf("Failed to get font system\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Get Vulkan context from NanoVG
	void* vkContext = nvgInternalParams(vg)->userPtr;
	if (!vkContext) {
		printf("Failed to get Vulkan context\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Enable GPU rasterization
	printf("Enabling GPU rasterization...\n");
	if (nvgFont_EnableGpuRasterization(fs, vkContext)) {
		printf("✓ GPU rasterizer initialized successfully\n");
		printf("✓ GPU rasterization enabled\n\n");
	} else {
		printf("✗ Failed to initialize GPU rasterizer\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Render a single frame
	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 800, 600, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Render text with GPU rasterization
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Test different font sizes to trigger GPU rasterization
	const char* test_text = "GPU Rasterization Test - abcdefghijklm ABCDEFGHIJKLM 0123456789";

	float y = 100;
	float sizes[] = {24, 32, 48, 64, 96, 128};
	int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

	for (int i = 0; i < num_sizes; i++) {
		nvgFontSize(vg, sizes[i]);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

		nvgText(vg, 50, y, test_text, NULL);

		// Calculate next y position
		float ascender, descender, lineh;
		nvgTextMetrics(vg, &ascender, &descender, &lineh);
		y += lineh + 10;

		if (y > 550) break;
	}

	nvgEndFrame(vg);

	nvgVkEndRenderPass(vg);
	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	// Submit and present
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSemaphores[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapchains[] = {winCtx->swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(winCtx->presentQueue, &presentInfo);
	vkQueueWaitIdle(winCtx->presentQueue);

	printf("\n=== Test Complete ===\n");
	printf("Screenshot saved to: screendumps/gpu_raster_test.ppm\n");

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
