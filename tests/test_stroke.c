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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper to create a stroked path
static void create_stroke_path(NVGVkContext* ctx, float* points, int numPoints, float width, float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = 0;
	path->fillCount = 0;
	path->strokeOffset = ctx->vertexCount;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Generate stroke as triangle list (quads as two triangles each)
	int vertCount = 0;
	for (int i = 0; i < numPoints - 1; i++) {
		float x1 = points[i * 2];
		float y1 = points[i * 2 + 1];
		float x2 = points[i * 2 + 2];
		float y2 = points[i * 2 + 3];

		// Calculate perpendicular direction
		float dx = x2 - x1;
		float dy = y2 - y1;
		float len = sqrtf(dx * dx + dy * dy);
		if (len < 0.001f) continue;

		float nx = -dy / len * width * 0.5f;
		float ny = dx / len * width * 0.5f;

		// Quad vertices
		float v1x = x1 + nx, v1y = y1 + ny;  // Top-left
		float v2x = x1 - nx, v2y = y1 - ny;  // Bottom-left
		float v3x = x2 + nx, v3y = y2 + ny;  // Top-right
		float v4x = x2 - nx, v4y = y2 - ny;  // Bottom-right

		// First triangle (v1, v2, v3)
		verts[ctx->vertexCount++] = (NVGvertex){v1x, v1y, 0.0f, 0.0f};
		verts[ctx->vertexCount++] = (NVGvertex){v2x, v2y, 1.0f, 0.0f};
		verts[ctx->vertexCount++] = (NVGvertex){v3x, v3y, 0.0f, 1.0f};

		// Second triangle (v2, v4, v3)
		verts[ctx->vertexCount++] = (NVGvertex){v2x, v2y, 1.0f, 0.0f};
		verts[ctx->vertexCount++] = (NVGvertex){v4x, v4y, 1.0f, 1.0f};
		verts[ctx->vertexCount++] = (NVGvertex){v3x, v3y, 0.0f, 1.0f};

		vertCount += 6;
	}

	path->strokeCount = vertCount;

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_STROKE;
	ctx->calls[callIdx].image = -1;
	ctx->calls[callIdx].pathOffset = pathIdx;
	ctx->calls[callIdx].pathCount = 1;
	ctx->calls[callIdx].triangleOffset = 0;
	ctx->calls[callIdx].triangleCount = 0;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = 0;

	// Setup uniforms
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
	printf("=== NanoVG Vulkan - Stroke Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Stroke Test");
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

	// Create dummy texture
	unsigned char dummyData[4] = {255, 255, 255, 255};
	int dummyTex = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, 1, 1, 0, dummyData);

	// Create pipelines
	printf("3. Creating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, dummyTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created\n\n");

	// Create stroked paths
	printf("4. Creating stroked paths...\n");

	// Straight line (thin, red)
	float line1[] = {100, 50, 300, 50};
	create_stroke_path(&nvgCtx, line1, 2, 2.0f, 1.0f, 0.0f, 0.0f, 1.0f);

	// Straight line (medium, green)
	float line2[] = {100, 100, 300, 100};
	create_stroke_path(&nvgCtx, line2, 2, 5.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	// Straight line (thick, blue)
	float line3[] = {100, 160, 300, 160};
	create_stroke_path(&nvgCtx, line3, 2, 10.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Diagonal line (yellow)
	float diag[] = {350, 50, 500, 180};
	create_stroke_path(&nvgCtx, diag, 2, 5.0f, 1.0f, 1.0f, 0.0f, 1.0f);

	// Zigzag path (cyan)
	float zigzag[] = {550, 50, 600, 100, 650, 50, 700, 100, 750, 50};
	create_stroke_path(&nvgCtx, zigzag, 5, 4.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	// Sine wave (magenta)
	float sineWave[40];
	for (int i = 0; i < 20; i++) {
		sineWave[i * 2] = 100 + i * 30;
		sineWave[i * 2 + 1] = 300 + sinf(i * 0.5f) * 40;
	}
	create_stroke_path(&nvgCtx, sineWave, 20, 3.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	// Square path (orange)
	float square[] = {100, 400, 200, 400, 200, 500, 100, 500, 100, 400};
	create_stroke_path(&nvgCtx, square, 5, 6.0f, 1.0f, 0.5f, 0.0f, 1.0f);

	// Circle path (white)
	float circle[66];
	for (int i = 0; i <= 32; i++) {
		float angle = (float)i / 32 * 2.0f * M_PI;
		circle[i * 2] = 450 + cosf(angle) * 80;
		circle[i * 2 + 1] = 450 + sinf(angle) * 80;
	}
	create_stroke_path(&nvgCtx, circle, 33, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f);

	// Spiral (lime)
	float spiral[102];
	for (int i = 0; i <= 50; i++) {
		float angle = (float)i / 50 * 4.0f * M_PI;
		float radius = 10 + i * 1.2f;
		spiral[i * 2] = 650 + cosf(angle) * radius;
		spiral[i * 2 + 1] = 450 + sinf(angle) * radius;
	}
	create_stroke_path(&nvgCtx, spiral, 51, 2.5f, 0.5f, 1.0f, 0.0f, 1.0f);

	printf("   ✓ Created %d vertices, %d paths, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.pathCount, nvgCtx.callCount);

	// Setup descriptors
	printf("5. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("6. Rendering strokes...\n");

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
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
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

	printf("   ✓ Strokes rendered\n\n");

	// Save screenshot
	printf("7. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "stroke_test.ppm")) {
		printf("   ✓ Screenshot saved to stroke_test.ppm\n\n");
	}

	// Cleanup
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);

	printf("=== Stroke Test PASSED ===\n");
	return 0;
}
