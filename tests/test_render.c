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

int main(void)
{
	printf("=== NanoVG Vulkan Backend - Phase 4 Render Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG Vulkan Render Test");
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
	printf("3. Setting viewport...\n");
	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);
	printf("   ✓ Viewport set\n\n");

	// Create dummy texture (needed for descriptor sets)
	printf("4. Creating dummy texture...\n");
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
	printf("5. Creating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete_texture(&nvgCtx, dummyTex);
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Pipelines created\n\n");

	// Setup vertex data for a triangle
	printf("6. Setting up triangle geometry...\n");
	nvgCtx.vertexCount = 3;

	// Use the already allocated vertex buffer
	NVGvertex* verts = (NVGvertex*)nvgCtx.vertices;
	// Triangle vertices (screen coordinates)
	verts[0].x = 400.0f; verts[0].y = 150.0f; verts[0].u = 0.5f; verts[0].v = 0.0f;
	verts[1].x = 250.0f; verts[1].y = 450.0f; verts[1].u = 0.0f; verts[1].v = 1.0f;
	verts[2].x = 550.0f; verts[2].y = 450.0f; verts[2].u = 1.0f; verts[2].v = 1.0f;

	// Setup render call
	nvgCtx.callCount = 1;
	nvgCtx.calls[0].type = NVGVK_TRIANGLES;
	nvgCtx.calls[0].image = -1; // No texture
	nvgCtx.calls[0].triangleOffset = 0;
	nvgCtx.calls[0].triangleCount = 3;
	nvgCtx.calls[0].uniformOffset = 0;
	nvgCtx.calls[0].blendFunc = 0;

	// Setup uniforms (use NVGVkUniforms, not NVGVkFragUniforms)
	nvgCtx.uniformCount = 1;
	memset(&nvgCtx.uniforms[0], 0, sizeof(NVGVkUniforms));
	nvgCtx.uniforms[0].viewSize[0] = 800.0f;
	nvgCtx.uniforms[0].viewSize[1] = 600.0f;
	nvgCtx.uniforms[0].innerCol[0] = 1.0f; // Red
	nvgCtx.uniforms[0].innerCol[1] = 0.0f; // Green
	nvgCtx.uniforms[0].innerCol[2] = 0.0f; // Blue
	nvgCtx.uniforms[0].innerCol[3] = 1.0f; // Alpha

	printf("   ✓ Triangle geometry ready\n\n");

	// Begin render pass and draw
	printf("7. Rendering triangle to framebuffer...\n");

	// Create semaphore for image acquisition
	VkSemaphore imageAvailableSemaphore;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(nvgCtx.commandBuffer, &beginInfo);

	// Begin render pass
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

	// Set viewport and scissor
	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(nvgCtx.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(nvgCtx.commandBuffer, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgvk_begin_render_pass(&nvgCtx, renderPassInfo.renderPass, renderPassInfo.framebuffer,
	                        renderPassInfo.renderArea, clearValues, 2, viewport, scissor);

	// Flush will perform the rendering
	nvgvk_flush(&nvgCtx);

	vkCmdEndRenderPass(nvgCtx.commandBuffer);
	vkEndCommandBuffer(nvgCtx.commandBuffer);

	// Submit
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &nvgCtx.commandBuffer;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);
	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

	printf("   ✓ Triangle rendered\n\n");

	// Save screenshot
	printf("8. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/render_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/render_test.ppm\n\n");
	} else {
		printf("   ✗ Failed to save screenshot\n\n");
	}

	// Cleanup
	printf("9. Cleaning up...\n");
	nvgvk_destroy_pipelines(&nvgCtx);
	nvgvk_delete_texture(&nvgCtx, dummyTex);
	nvgvk_delete(&nvgCtx); // This will free vertices
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Phase 4 Render Test PASSED ===\n");
	return 0;
}
