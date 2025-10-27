#include "window_utils.h"
#include "../src/nanovg.h"
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

// Helper to create a complex path (star shape with self-intersection)
static void create_star_path(NVGVkContext* ctx, float cx, float cy, float outerR, float innerR, int points, float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = ctx->vertexCount;
	path->fillCount = 0;
	path->strokeOffset = 0;
	path->strokeCount = 0;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Create star path as triangle list (convert fan to list)
	for (int i = 0; i < points * 2; i++) {
		float angle1 = (float)i / (points * 2) * 2.0f * M_PI;
		float angle2 = (float)((i + 1) % (points * 2)) / (points * 2) * 2.0f * M_PI;
		float radius1 = (i % 2 == 0) ? outerR : innerR;
		float radius2 = ((i + 1) % 2 == 0) ? outerR : innerR;

		// Triangle: center, current point, next point
		// Center
		verts[ctx->vertexCount].x = cx;
		verts[ctx->vertexCount].y = cy;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		// Current edge
		verts[ctx->vertexCount].x = cx + cosf(angle1) * radius1;
		verts[ctx->vertexCount].y = cy + sinf(angle1) * radius1;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		// Next edge
		verts[ctx->vertexCount].x = cx + cosf(angle2) * radius2;
		verts[ctx->vertexCount].y = cy + sinf(angle2) * radius2;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;
	}

	path->fillCount = points * 2 * 3;  // 3 vertices per triangle

	// Create bounding quad for cover pass
	int quadStart = ctx->vertexCount;
	float margin = outerR * 1.1f;

	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy - margin, 0.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy - margin, 1.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy + margin, 1.0f, 1.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy - margin, 0.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy + margin, 1.0f, 1.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy + margin, 0.0f, 1.0f};

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_FILL;
	ctx->calls[callIdx].image = -1;
	ctx->calls[callIdx].pathOffset = pathIdx;
	ctx->calls[callIdx].pathCount = 1;
	ctx->calls[callIdx].triangleOffset = quadStart;
	ctx->calls[callIdx].triangleCount = 6;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = 0;

	// Setup uniforms for solid color (both innerCol and outerCol same)
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = r;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = a;
	ctx->uniforms[ctx->uniformCount].outerCol[0] = r;  // Same as inner for solid
	ctx->uniforms[ctx->uniformCount].outerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].outerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].outerCol[3] = a;
	ctx->uniforms[ctx->uniformCount].type = 0;  // Gradient type
	ctx->uniforms[ctx->uniformCount].extent[0] = 1.0f;
	ctx->uniforms[ctx->uniformCount].extent[1] = 1.0f;
	ctx->uniforms[ctx->uniformCount].feather = 1.0f;
	ctx->uniformCount++;
}

// Helper to create a path with hole (ring/donut)
static void create_ring_path(NVGVkContext* ctx, float cx, float cy, float outerR, float innerR, int segments, float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = ctx->vertexCount;
	path->strokeOffset = 0;
	path->strokeCount = 0;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Create ring as triangle list (quads connecting inner and outer circles)
	for (int i = 0; i < segments; i++) {
		float angle1 = (float)i / segments * 2.0f * M_PI;
		float angle2 = (float)((i + 1) % segments) / segments * 2.0f * M_PI;

		// Outer edge vertices
		float ox1 = cx + cosf(angle1) * outerR;
		float oy1 = cy + sinf(angle1) * outerR;
		float ox2 = cx + cosf(angle2) * outerR;
		float oy2 = cy + sinf(angle2) * outerR;

		// Inner edge vertices
		float ix1 = cx + cosf(angle1) * innerR;
		float iy1 = cy + sinf(angle1) * innerR;
		float ix2 = cx + cosf(angle2) * innerR;
		float iy2 = cy + sinf(angle2) * innerR;

		// First triangle (ox1, ix1, ox2)
		verts[ctx->vertexCount].x = ox1;
		verts[ctx->vertexCount].y = oy1;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		verts[ctx->vertexCount].x = ix1;
		verts[ctx->vertexCount].y = iy1;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		verts[ctx->vertexCount].x = ox2;
		verts[ctx->vertexCount].y = oy2;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		// Second triangle (ix1, ix2, ox2)
		verts[ctx->vertexCount].x = ix1;
		verts[ctx->vertexCount].y = iy1;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		verts[ctx->vertexCount].x = ix2;
		verts[ctx->vertexCount].y = iy2;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;

		verts[ctx->vertexCount].x = ox2;
		verts[ctx->vertexCount].y = oy2;
		verts[ctx->vertexCount].u = 0.5f;
		verts[ctx->vertexCount].v = 0.5f;
		ctx->vertexCount++;
	}

	path->fillCount = segments * 6;  // 6 vertices per quad (2 triangles)

	// Create bounding quad
	int quadStart = ctx->vertexCount;
	float margin = outerR * 1.1f;

	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy - margin, 0.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy - margin, 1.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy + margin, 1.0f, 1.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy - margin, 0.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx + margin, cy + margin, 1.0f, 1.0f};
	verts[ctx->vertexCount++] = (NVGvertex){cx - margin, cy + margin, 0.0f, 1.0f};

	// Add render call
	int callIdx = ctx->callCount++;
	ctx->calls[callIdx].type = NVGVK_FILL;
	ctx->calls[callIdx].image = -1;
	ctx->calls[callIdx].pathOffset = pathIdx;
	ctx->calls[callIdx].pathCount = 1;
	ctx->calls[callIdx].triangleOffset = quadStart;
	ctx->calls[callIdx].triangleCount = 6;
	ctx->calls[callIdx].uniformOffset = ctx->uniformCount;
	ctx->calls[callIdx].blendFunc = 0;

	// Setup uniforms for solid color (both innerCol and outerCol same)
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = r;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = a;
	ctx->uniforms[ctx->uniformCount].outerCol[0] = r;  // Same as inner for solid
	ctx->uniforms[ctx->uniformCount].outerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].outerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].outerCol[3] = a;
	ctx->uniforms[ctx->uniformCount].type = 0;  // Gradient type
	ctx->uniforms[ctx->uniformCount].extent[0] = 1.0f;
	ctx->uniforms[ctx->uniformCount].extent[1] = 1.0f;
	ctx->uniforms[ctx->uniformCount].feather = 1.0f;
	ctx->uniformCount++;
}

int main(void)
{
	printf("=== NanoVG Vulkan - Fill (Stencil) Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Fill Test");
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

	// Set viewport
	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);

	// Create dummy texture
	printf("3. Creating dummy texture...\n");
	unsigned char dummyData[4] = {255, 255, 255, 255};
	int dummyTex = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_RGBA, 1, 1, 0, dummyData);
	if (dummyTex < 0) {
		fprintf(stderr, "Failed to create dummy texture\n");
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Dummy texture created\n\n");

	// Create pipelines
	printf("4. Creating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, dummyTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created\n\n");

	// Create complex fills
	printf("5. Creating complex fills...\n");

	// 5-pointed star (yellow)
	create_star_path(&nvgCtx, 200, 150, 80, 35, 5, 1.0f, 1.0f, 0.0f, 1.0f);

	// 6-pointed star (cyan)
	create_star_path(&nvgCtx, 600, 150, 80, 35, 6, 0.0f, 1.0f, 1.0f, 1.0f);

	// Ring/donut (magenta)
	create_ring_path(&nvgCtx, 200, 400, 90, 50, 32, 1.0f, 0.0f, 1.0f, 1.0f);

	// Double ring (orange)
	create_ring_path(&nvgCtx, 600, 400, 90, 30, 32, 1.0f, 0.5f, 0.0f, 1.0f);

	printf("   ✓ Created %d vertices, %d paths, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.pathCount, nvgCtx.callCount);

	// Setup descriptors
	printf("6. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("7. Rendering complex fills...\n");

	uint32_t imageIndex;

	// Create semaphore for image acquisition
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
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.15f, 1.0f}};
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

	printf("   ✓ Complex fills rendered\n\n");

	// Save screenshot
	printf("8. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "fill_test.ppm")) {
		printf("   ✓ Screenshot saved to fill_test.ppm\n\n");
	}

	// Cleanup
	printf("9. Cleaning up...\n");
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Fill Test PASSED ===\n");
	return 0;
}
