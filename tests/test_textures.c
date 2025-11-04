#include "window_utils.h"
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void create_checkerboard_texture(unsigned char* data, int width, int height, int squareSize)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int xi = x / squareSize;
			int yi = y / squareSize;
			int isWhite = (xi + yi) % 2;

			int idx = (y * width + x) * 4;
			data[idx + 0] = isWhite ? 255 : 64;
			data[idx + 1] = isWhite ? 255 : 64;
			data[idx + 2] = isWhite ? 255 : 64;
			data[idx + 3] = 255;
		}
	}
}

static void create_gradient_texture(unsigned char* data, int width, int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 4;
			data[idx + 0] = (unsigned char)(x * 255 / width);
			data[idx + 1] = (unsigned char)(y * 255 / height);
			data[idx + 2] = (unsigned char)((width - x) * 255 / width);
			data[idx + 3] = 255;
		}
	}
}

int main(void)
{
	printf("=== NanoVG Texture Mapping Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Texture Test");
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

	// Create test textures
	printf("3. Creating test textures...\n");

	int texWidth = 128;
	int texHeight = 128;

	// Checkerboard texture
	unsigned char* checkerData = (unsigned char*)malloc(texWidth * texHeight * 4);
	create_checkerboard_texture(checkerData, texWidth, texHeight, 16);
	int checkerTex = nvgCreateImageRGBA(vg, texWidth, texHeight, 0, checkerData);
	free(checkerData);

	if (checkerTex < 0) {
		fprintf(stderr, "Failed to create checkerboard texture\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Gradient texture
	unsigned char* gradientData = (unsigned char*)malloc(texWidth * texHeight * 4);
	create_gradient_texture(gradientData, texWidth, texHeight);
	int gradientTex = nvgCreateImageRGBA(vg, texWidth, texHeight, 0, gradientData);
	free(gradientData);

	if (gradientTex < 0) {
		fprintf(stderr, "Failed to create gradient texture\n");
		nvgDeleteImage(vg, checkerTex);
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("   ✓ Created checkerboard and gradient textures\n\n");

	// Render
	printf("4. Rendering textured geometry...\n");

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
	clearValues[0].color = (VkClearColorValue){{0.2f, 0.2f, 0.25f, 1.0f}};
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

	// Checkerboard rectangles (untinted)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 150, 150);
	NVGpaint imgPaint = nvgImagePattern(vg, 50, 50, 150, 150, 0, checkerTex, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 250, 50, 200, 100);
	imgPaint = nvgImagePattern(vg, 250, 50, 200, 100, 0, checkerTex, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	// Gradient rectangle (untinted)
	nvgBeginPath(vg);
	nvgRect(vg, 500, 50, 150, 150);
	imgPaint = nvgImagePattern(vg, 500, 50, 150, 150, 0, gradientTex, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	// Tinted checkerboard (red tint)
	nvgBeginPath(vg);
	nvgRect(vg, 50, 250, 150, 100);
	imgPaint = nvgImagePattern(vg, 50, 250, 150, 100, 0, checkerTex, 1.0f);
	imgPaint.innerColor = nvgRGBAf(1.0f, 0.3f, 0.3f, 1.0f);
	imgPaint.outerColor = nvgRGBAf(1.0f, 0.3f, 0.3f, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	// Tinted gradient (green tint)
	nvgBeginPath(vg);
	nvgRect(vg, 250, 250, 150, 100);
	imgPaint = nvgImagePattern(vg, 250, 250, 150, 100, 0, gradientTex, 1.0f);
	imgPaint.innerColor = nvgRGBAf(0.3f, 1.0f, 0.3f, 1.0f);
	imgPaint.outerColor = nvgRGBAf(0.3f, 1.0f, 0.3f, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	// Tinted checkerboard (blue tint)
	nvgBeginPath(vg);
	nvgRect(vg, 450, 250, 150, 100);
	imgPaint = nvgImagePattern(vg, 450, 250, 150, 100, 0, checkerTex, 1.0f);
	imgPaint.innerColor = nvgRGBAf(0.3f, 0.3f, 1.0f, 1.0f);
	imgPaint.outerColor = nvgRGBAf(0.3f, 0.3f, 1.0f, 1.0f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	// Semi-transparent textured rectangles
	nvgBeginPath(vg);
	nvgRect(vg, 100, 400, 200, 150);
	imgPaint = nvgImagePattern(vg, 100, 400, 200, 150, 0, checkerTex, 0.7f);
	nvgFillPaint(vg, imgPaint);
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 200, 450, 200, 100);
	imgPaint = nvgImagePattern(vg, 200, 450, 200, 100, 0, gradientTex, 0.5f);
	nvgFillPaint(vg, imgPaint);
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

	printf("   ✓ Textured geometry rendered\n\n");

	// Save screenshot
	printf("5. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/textures_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/textures_test.ppm\n\n");
	}

	// Cleanup
	nvgDeleteImage(vg, checkerTex);
	nvgDeleteImage(vg, gradientTex);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Texture Test PASSED ===\n");
	return 0;
}
