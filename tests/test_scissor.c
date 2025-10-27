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

// Helper to create a rectangle with scissor region
static void create_scissored_rect(NVGVkContext* ctx, float x, float y, float w, float h,
                                   float sx, float sy, float sw, float sh,
                                   float r, float g, float b, float a)
{
	int pathIdx = ctx->pathCount++;
	NVGVkPath* path = &ctx->paths[pathIdx];

	path->fillOffset = ctx->vertexCount;
	path->strokeOffset = 0;
	path->strokeCount = 0;

	NVGvertex* verts = (NVGvertex*)ctx->vertices;

	// Create rectangle as two triangles
	float x2 = x + w;
	float y2 = y + h;

	// Triangle 1
	verts[ctx->vertexCount++] = (NVGvertex){x, y, 0.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){x2, y, 1.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){x, y2, 0.0f, 1.0f};

	// Triangle 2
	verts[ctx->vertexCount++] = (NVGvertex){x2, y, 1.0f, 0.0f};
	verts[ctx->vertexCount++] = (NVGvertex){x2, y2, 1.0f, 1.0f};
	verts[ctx->vertexCount++] = (NVGvertex){x, y2, 0.0f, 1.0f};

	path->fillCount = 6;

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

	// Setup uniforms with scissor region
	memset(&ctx->uniforms[ctx->uniformCount], 0, sizeof(NVGVkUniforms));
	ctx->uniforms[ctx->uniformCount].viewSize[0] = 800.0f;
	ctx->uniforms[ctx->uniformCount].viewSize[1] = 600.0f;
	ctx->uniforms[ctx->uniformCount].innerCol[0] = r;
	ctx->uniforms[ctx->uniformCount].innerCol[1] = g;
	ctx->uniforms[ctx->uniformCount].innerCol[2] = b;
	ctx->uniforms[ctx->uniformCount].innerCol[3] = a;

	// Setup scissor transform
	// Scissor matrix transforms from pixel coordinates to normalized scissor space
	float sx_center = sx + sw * 0.5f;
	float sy_center = sy + sh * 0.5f;
	float sx_halfwidth = sw * 0.5f;
	float sy_halfheight = sh * 0.5f;

	// Identity-based transform that maps scissor rect to [-1,1] range
	ctx->uniforms[ctx->uniformCount].scissorMat[0] = 1.0f / sx_halfwidth;
	ctx->uniforms[ctx->uniformCount].scissorMat[1] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[2] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[3] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[4] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[5] = 1.0f / sy_halfheight;
	ctx->uniforms[ctx->uniformCount].scissorMat[6] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[7] = 0.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[8] = -sx_center / sx_halfwidth;
	ctx->uniforms[ctx->uniformCount].scissorMat[9] = -sy_center / sy_halfheight;
	ctx->uniforms[ctx->uniformCount].scissorMat[10] = 1.0f;
	ctx->uniforms[ctx->uniformCount].scissorMat[11] = 0.0f;

	ctx->uniforms[ctx->uniformCount].scissorExt[0] = 1.0f;
	ctx->uniforms[ctx->uniformCount].scissorExt[1] = 1.0f;
	ctx->uniforms[ctx->uniformCount].scissorScale[0] = 1.0f;
	ctx->uniforms[ctx->uniformCount].scissorScale[1] = 1.0f;

	ctx->uniformCount++;
}

int main(void)
{
	printf("=== NanoVG Vulkan - Scissor Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Scissor Test");
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

	// Create scissored rectangles
	printf("4. Creating scissored rectangles...\n");

	// Large red rectangle (400x400) scissored to top-left (200x200)
	create_scissored_rect(&nvgCtx, 50, 50, 400, 400,
	                      50, 50, 200, 200,
	                      1.0f, 0.0f, 0.0f, 1.0f);

	// Large green rectangle (400x400) scissored to top-right (200x200)
	create_scissored_rect(&nvgCtx, 350, 50, 400, 400,
	                      550, 50, 200, 200,
	                      0.0f, 1.0f, 0.0f, 1.0f);

	// Large blue rectangle (400x400) scissored to bottom-left (200x200)
	create_scissored_rect(&nvgCtx, 50, 150, 400, 400,
	                      50, 350, 200, 200,
	                      0.0f, 0.0f, 1.0f, 1.0f);

	// Large yellow rectangle (400x400) scissored to bottom-right (200x200)
	create_scissored_rect(&nvgCtx, 350, 150, 400, 400,
	                      550, 350, 200, 200,
	                      1.0f, 1.0f, 0.0f, 1.0f);

	// Center magenta rectangle (300x300) scissored to small center region (100x100)
	create_scissored_rect(&nvgCtx, 250, 200, 300, 300,
	                      350, 250, 100, 100,
	                      1.0f, 0.0f, 1.0f, 1.0f);

	printf("   ✓ Created %d vertices, %d paths, %d calls\n\n", nvgCtx.vertexCount, nvgCtx.pathCount, nvgCtx.callCount);

	// Setup descriptors
	printf("5. Setting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("   ✓ Descriptors set up\n\n");

	// Render
	printf("6. Rendering scissored rectangles...\n");

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

	printf("   ✓ Scissored rectangles rendered\n\n");

	// Save screenshot
	printf("7. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "scissor_test.ppm")) {
		printf("   ✓ Screenshot saved to scissor_test.ppm\n\n");
	}

	// Cleanup
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);

	printf("=== Scissor Test PASSED ===\n");
	return 0;
}
