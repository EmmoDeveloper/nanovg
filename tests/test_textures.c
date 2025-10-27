#include "window_utils.h"
#include "../src/vulkan/nvg_vk_context.h"
#include "../src/vulkan/nvg_vk_buffer.h"
#include "../src/vulkan/nvg_vk_texture.h"
#include "../src/vulkan/nvg_vk_pipeline.h"
#include "../src/vulkan/nvg_vk_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper to create a checkerboard texture
static void create_checkerboard_texture(unsigned char* data, int width, int height, int squareSize)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int xi = x / squareSize;
			int yi = y / squareSize;
			int isWhite = (xi + yi) % 2;

			int idx = (y * width + x) * 4;
			data[idx + 0] = isWhite ? 255 : 64;   // R
			data[idx + 1] = isWhite ? 255 : 64;   // G
			data[idx + 2] = isWhite ? 255 : 64;   // B
			data[idx + 3] = 255;                  // A
		}
	}
}

// Helper to create a gradient texture
static void create_gradient_texture(unsigned char* data, int width, int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 4;
			data[idx + 0] = (unsigned char)(x * 255 / width);         // R gradient
			data[idx + 1] = (unsigned char)(y * 255 / height);        // G gradient
			data[idx + 2] = (unsigned char)((width - x) * 255 / width); // B gradient
			data[idx + 3] = 255;                                      // A
		}
	}
}

// Helper to create textured rectangle using TRIANGLES with image
static void create_textured_rect(NVGVkContext* ctx, float x, float y, float w, float h, int texId)
{
	int start = ctx->vertexCount;
	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Rectangle with proper UV coordinates
	verts[start + 0].x = x;     verts[start + 0].y = y;     verts[start + 0].u = 0.0f; verts[start + 0].v = 0.0f;
	verts[start + 1].x = x + w; verts[start + 1].y = y;     verts[start + 1].u = 1.0f; verts[start + 1].v = 0.0f;
	verts[start + 2].x = x + w; verts[start + 2].y = y + h; verts[start + 2].u = 1.0f; verts[start + 2].v = 1.0f;

	verts[start + 3].x = x;     verts[start + 3].y = y;     verts[start + 3].u = 0.0f; verts[start + 3].v = 0.0f;
	verts[start + 4].x = x + w; verts[start + 4].y = y + h; verts[start + 4].u = 1.0f; verts[start + 4].v = 1.0f;
	verts[start + 5].x = x;     verts[start + 5].y = y + h; verts[start + 5].u = 0.0f; verts[start + 5].v = 1.0f;

	ctx->vertexCount += 6;

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_TRIANGLES;
	ctx->calls[callIdx].image = texId;  // Use actual texture
	ctx->calls[callIdx].triangleOffset = start;
	ctx->calls[callIdx].triangleCount = 6;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = 0;

	// Setup uniforms (for textured rendering, innerCol acts as tint)
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = 1.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = 1.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = 1.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = 1.0f;
	ctx->uniformCount++;
}

// Helper to create tinted textured rectangle
static void create_tinted_textured_rect(NVGVkContext* ctx, float x, float y, float w, float h, int texId, float r, float g, float b, float a)
{
	int start = ctx->vertexCount;
	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Rectangle with proper UV coordinates
	verts[start + 0].x = x;     verts[start + 0].y = y;     verts[start + 0].u = 0.0f; verts[start + 0].v = 0.0f;
	verts[start + 1].x = x + w; verts[start + 1].y = y;     verts[start + 1].u = 1.0f; verts[start + 1].v = 0.0f;
	verts[start + 2].x = x + w; verts[start + 2].y = y + h; verts[start + 2].u = 1.0f; verts[start + 2].v = 1.0f;

	verts[start + 3].x = x;     verts[start + 3].y = y;     verts[start + 3].u = 0.0f; verts[start + 3].v = 0.0f;
	verts[start + 4].x = x + w; verts[start + 4].y = y + h; verts[start + 4].u = 1.0f; verts[start + 4].v = 1.0f;
	verts[start + 5].x = x;     verts[start + 5].y = y + h; verts[start + 5].u = 0.0f; verts[start + 5].v = 1.0f;

	ctx->vertexCount += 6;

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_TRIANGLES;
	ctx->calls[callIdx].image = texId;
	ctx->calls[callIdx].triangleOffset = start;
	ctx->calls[callIdx].triangleCount = 6;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = 0;

	// Setup uniforms with tint color
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = r;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = a;
	ctx->uniformCount++;
}

int main(void)
{
	printf("=== NanoVG Vulkan - Texture Mapping Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Texture Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG Vulkan context
	printf("2. Creating NanoVG Vulkan context...\n");
	NVGVkContext nvgCtx = {0};
	NVGVkCreateInfo createInfo = {0};
	createInfo.device = winCtx->device;
	createInfo.physicalDevice = winCtx->physicalDevice;
	createInfo.queue = winCtx->graphicsQueue;
	createInfo.commandPool = winCtx->commandPool;
	createInfo.flags = 0;

	if (!nvgvk_create(&nvgCtx, &createInfo)) {
		fprintf(stderr, "Failed to create NanoVG Vulkan context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG Vulkan context created\n\n");

	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);

	// Create test textures
	printf("3. Creating test textures...\n");

	// Checkerboard texture
	int texWidth = 128;
	int texHeight = 128;
	unsigned char* checkerData = (unsigned char*)malloc(texWidth * texHeight * 4);
	create_checkerboard_texture(checkerData, texWidth, texHeight, 16);
	int checkerTex = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, texWidth, texHeight, 0, checkerData);
	free(checkerData);

	if (checkerTex < 0) {
		fprintf(stderr, "Failed to create checkerboard texture\n");
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}

	// Gradient texture
	unsigned char* gradientData = (unsigned char*)malloc(texWidth * texHeight * 4);
	create_gradient_texture(gradientData, texWidth, texHeight);
	int gradientTex = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, texWidth, texHeight, 0, gradientData);
	free(gradientData);

	if (gradientTex < 0) {
		fprintf(stderr, "Failed to create gradient texture\n");
		nvgvk_delete_texture(&nvgCtx, checkerTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("   ✓ Created checkerboard and gradient textures\n\n");

	// Create pipelines
	printf("4. Creating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, checkerTex);
		nvgvk_delete_texture(&nvgCtx, gradientTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created\n\n");

	// Create textured geometry
	printf("5. Creating textured geometry...\n");

	// Checkerboard rectangles (untinted)
	create_textured_rect(&nvgCtx, 50, 50, 150, 150, checkerTex);
	create_textured_rect(&nvgCtx, 250, 50, 200, 100, checkerTex);

	// Gradient rectangles (untinted)
	create_textured_rect(&nvgCtx, 500, 50, 150, 150, gradientTex);

	// Tinted checkerboard (red tint)
	create_tinted_textured_rect(&nvgCtx, 50, 250, 150, 100, checkerTex, 1.0f, 0.3f, 0.3f, 1.0f);

	// Tinted gradient (green tint)
	create_tinted_textured_rect(&nvgCtx, 250, 250, 150, 100, gradientTex, 0.3f, 1.0f, 0.3f, 1.0f);

	// Tinted checkerboard (blue tint)
	create_tinted_textured_rect(&nvgCtx, 450, 250, 150, 100, checkerTex, 0.3f, 0.3f, 1.0f, 1.0f);

	// Semi-transparent textured rectangles
	create_tinted_textured_rect(&nvgCtx, 100, 400, 200, 150, checkerTex, 1.0f, 1.0f, 1.0f, 0.7f);
	create_tinted_textured_rect(&nvgCtx, 200, 450, 200, 100, gradientTex, 1.0f, 1.0f, 1.0f, 0.5f);

	printf("   ✓ Created %d vertices, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.callCount);

	// Setup descriptors
	printf("6. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("7. Rendering textured geometry...\n");

	uint32_t imageIndex;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(nvgCtx.commandBuffer, &beginInfo);

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

	vkCmdBeginRenderPass(nvgCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(nvgCtx.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(nvgCtx.commandBuffer, 0, 1, &scissor);

	nvgvk_flush(&nvgCtx);

	vkCmdEndRenderPass(nvgCtx.commandBuffer);
	vkEndCommandBuffer(nvgCtx.commandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &nvgCtx.commandBuffer;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("   ✓ Textured geometry rendered\n\n");

	// Save screenshot
	printf("8. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "textures_test.ppm")) {
		printf("   ✓ Screenshot saved to textures_test.ppm\n\n");
	}

	// Cleanup
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, checkerTex);
	nvgvk_delete_texture(&nvgCtx, gradientTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);

	printf("=== Texture Test PASSED ===\n");
	return 0;
}
