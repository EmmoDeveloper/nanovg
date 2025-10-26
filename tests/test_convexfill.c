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

// Helper to create a convex polygon
static void create_convex_polygon(NVGVkContext* ctx, float cx, float cy, float radius, int sides, float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = ctx->vertexCount;
	path->strokeOffset = 0;
	path->strokeCount = 0;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Create polygon as triangle list (convert fan to list)
	for (int i = 0; i < sides; i++) {
		float angle1 = (float)i / sides * 2.0f * M_PI;
		float angle2 = (float)((i + 1) % sides) / sides * 2.0f * M_PI;

		// Center vertex
		verts[ctx->vertexCount].x = cx;
		verts[ctx->vertexCount].y = cy;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		// First edge vertex
		verts[ctx->vertexCount].x = cx + cosf(angle1) * radius;
		verts[ctx->vertexCount].y = cy + sinf(angle1) * radius;
		verts[ctx->vertexCount].u = (cosf(angle1) + 1.0f) * 0.5f;
		verts[ctx->vertexCount].v = (sinf(angle1) + 1.0f) * 0.5f;
		ctx->vertexCount++;

		// Second edge vertex
		verts[ctx->vertexCount].x = cx + cosf(angle2) * radius;
		verts[ctx->vertexCount].y = cy + sinf(angle2) * radius;
		verts[ctx->vertexCount].u = (cosf(angle2) + 1.0f) * 0.5f;
		verts[ctx->vertexCount].v = (sinf(angle2) + 1.0f) * 0.5f;
		ctx->vertexCount++;
	}

	path->fillCount = sides * 3;  // 3 vertices per triangle

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_CONVEXFILL;
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

// Helper to create a rounded rectangle as convex path
static void create_rounded_rect(NVGVkContext* ctx, float x, float y, float w, float h, float cornerRadius, int cornerSegments, float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = ctx->vertexCount;
	path->strokeOffset = 0;
	path->strokeCount = 0;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Calculate center point
	float cx = x + w * 0.5f;
	float cy = y + h * 0.5f;

	// Collect outline points
	float points[200][2];  // Max 200 points
	int pointCount = 0;

	// Top-left corner
	for (int i = 0; i <= cornerSegments; i++) {
		float angle = M_PI + (float)i / cornerSegments * M_PI / 2.0f;
		points[pointCount][0] = x + cornerRadius + cosf(angle) * cornerRadius;
		points[pointCount][1] = y + cornerRadius + sinf(angle) * cornerRadius;
		pointCount++;
	}

	// Bottom-left corner
	for (int i = 0; i <= cornerSegments; i++) {
		float angle = M_PI / 2.0f + (float)i / cornerSegments * M_PI / 2.0f;
		points[pointCount][0] = x + cornerRadius + cosf(angle) * cornerRadius;
		points[pointCount][1] = y + h - cornerRadius + sinf(angle) * cornerRadius;
		pointCount++;
	}

	// Bottom-right corner
	for (int i = 0; i <= cornerSegments; i++) {
		float angle = (float)i / cornerSegments * M_PI / 2.0f;
		points[pointCount][0] = x + w - cornerRadius + cosf(angle) * cornerRadius;
		points[pointCount][1] = y + h - cornerRadius + sinf(angle) * cornerRadius;
		pointCount++;
	}

	// Top-right corner
	for (int i = 0; i <= cornerSegments; i++) {
		float angle = -M_PI / 2.0f + (float)i / cornerSegments * M_PI / 2.0f;
		points[pointCount][0] = x + w - cornerRadius + cosf(angle) * cornerRadius;
		points[pointCount][1] = y + cornerRadius + sinf(angle) * cornerRadius;
		pointCount++;
	}

	// Convert to triangle list with center point
	for (int i = 0; i < pointCount; i++) {
		// Center
		verts[ctx->vertexCount].x = cx;
		verts[ctx->vertexCount].y = cy;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		// Current point
		verts[ctx->vertexCount].x = points[i][0];
		verts[ctx->vertexCount].y = points[i][1];
		verts[ctx->vertexCount].u = (points[i][0] - x) / w;
		verts[ctx->vertexCount].v = (points[i][1] - y) / h;
		ctx->vertexCount++;

		// Next point (wrapping around)
		int next = (i + 1) % pointCount;
		verts[ctx->vertexCount].x = points[next][0];
		verts[ctx->vertexCount].y = points[next][1];
		verts[ctx->vertexCount].u = (points[next][0] - x) / w;
		verts[ctx->vertexCount].v = (points[next][1] - y) / h;
		ctx->vertexCount++;
	}

	path->fillCount = pointCount * 3;

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_CONVEXFILL;
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
	printf("=== NanoVG Vulkan - Convex Fill Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Convex Fill Test");
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

	// Create convex shapes
	printf("4. Creating convex shapes...\n");

	// Triangle (red)
	create_convex_polygon(&nvgCtx, 100, 100, 60, 3, 1.0f, 0.0f, 0.0f, 1.0f);

	// Pentagon (green)
	create_convex_polygon(&nvgCtx, 300, 100, 60, 5, 0.0f, 1.0f, 0.0f, 1.0f);

	// Hexagon (blue)
	create_convex_polygon(&nvgCtx, 500, 100, 60, 6, 0.0f, 0.0f, 1.0f, 1.0f);

	// Octagon (yellow)
	create_convex_polygon(&nvgCtx, 700, 100, 60, 8, 1.0f, 1.0f, 0.0f, 1.0f);

	// Rounded rectangles
	create_rounded_rect(&nvgCtx, 50, 250, 150, 100, 20, 4, 0.0f, 1.0f, 1.0f, 1.0f);  // Cyan
	create_rounded_rect(&nvgCtx, 250, 250, 150, 100, 30, 6, 1.0f, 0.0f, 1.0f, 1.0f);  // Magenta
	create_rounded_rect(&nvgCtx, 450, 250, 150, 100, 40, 8, 1.0f, 0.5f, 0.0f, 1.0f);  // Orange

	// Circle (approximated as 32-sided polygon) - white
	create_convex_polygon(&nvgCtx, 400, 480, 80, 32, 1.0f, 1.0f, 1.0f, 1.0f);

	printf("   ✓ Created %d vertices, %d paths, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.pathCount, nvgCtx.callCount);

	// Setup descriptors
	printf("5. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("6. Rendering convex fills...\n");

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
	clearValues[0].color = (VkClearColorValue){{0.15f, 0.15f, 0.15f, 1.0f}};
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

	printf("   ✓ Convex fills rendered\n\n");

	// Save screenshot
	printf("7. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "convexfill_test.ppm")) {
		printf("   ✓ Screenshot saved to convexfill_test.ppm\n\n");
	}

	// Cleanup
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);

	printf("=== Convex Fill Test PASSED ===\n");
	return 0;
}
