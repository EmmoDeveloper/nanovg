#include "nvg_vk_context.h"
#include "nvg_vk_buffer.h"
#include "nvg_vk_render.h"
#include "nvg_vk_texture.h"
#include "../nanovg.h"
#include "../nanovg_vk_virtual_atlas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int nvgvk_create(void* userPtr, const NVGVkCreateInfo* createInfo)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || !createInfo) {
		return 0;
	}

	memset(vk, 0, sizeof(NVGVkContext));

	// Store Vulkan handles (not owned by us)
	vk->device = createInfo->device;
	vk->physicalDevice = createInfo->physicalDevice;
	vk->queue = createInfo->queue;
	vk->commandPool = createInfo->commandPool;
	vk->flags = createInfo->flags;

	// Allocate command buffer
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = vk->commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vk->device, &cmdAllocInfo, &vk->commandBuffer) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate command buffer\n");
		return 0;
	}

	// Create fence for upload synchronization
	VkFenceCreateInfo fenceInfo = {0};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first use works

	if (vkCreateFence(vk->device, &fenceInfo, NULL, &vk->uploadFence) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create upload fence\n");
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Initialize vertex data
	vk->vertexCapacity = NVGVK_INITIAL_VERTEX_COUNT;
	vk->vertices = (float*)malloc(vk->vertexCapacity * sizeof(NVGvertex));
	if (!vk->vertices) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate vertex buffer\n");
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Create vertex buffer
	VkDeviceSize vertexBufferSize = vk->vertexCapacity * sizeof(NVGvertex);
	if (!nvgvk_buffer_create(vk, &vk->vertexBuffer, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create vertex buffer\n");
		free(vk->vertices);
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Create uniform buffer (for viewSize)
	VkDeviceSize uniformBufferSize = sizeof(float) * 2;
	if (!nvgvk_buffer_create(vk, &vk->uniformBuffer, uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create uniform buffer\n");
		nvgvk_buffer_destroy(vk, &vk->vertexBuffer);
		free(vk->vertices);
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Initialize texture descriptor system
	if (!nvgvk__init_texture_descriptors(vk)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to initialize texture descriptors\n");
		nvgvk_buffer_destroy(vk, &vk->uniformBuffer);
		nvgvk_buffer_destroy(vk, &vk->vertexBuffer);
		free(vk->vertices);
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Initialize virtual atlas (for CJK fonts and large glyph sets)
	// Note: Font context and rasterize callback will be set later via vknvg__setAtlasFontContext
	vk->virtualAtlas = vknvg__createVirtualAtlas(vk->device, vk->physicalDevice, NULL, NULL);
	if (!vk->virtualAtlas) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create virtual atlas\n");
		nvgvk__destroy_texture_descriptors(vk);
		nvgvk_buffer_destroy(vk, &vk->uniformBuffer);
		nvgvk_buffer_destroy(vk, &vk->vertexBuffer);
		free(vk->vertices);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Enable async uploads using the graphics queue
	// TODO: Find dedicated transfer queue for better performance
	// For now, use graphics queue (works but not optimal)
	VkResult result = vknvg__enableAsyncUploads((VKNVGvirtualAtlas*)vk->virtualAtlas,
	                                             vk->queue,  // Use graphics queue
	                                             0);          // Queue family 0 (graphics)

	if (result == VK_SUCCESS) {
		printf("NanoVG Vulkan: Async uploads enabled (using graphics queue)\n");
	} else {
		printf("NanoVG Vulkan: Warning - Failed to enable async uploads (error %d)\n", result);
		// Not a fatal error - continue without async uploads
	}

	// Enable GPU MSDF generation (graphics queue can do compute operations)
	result = vknvg__enableGPUMSDF((VKNVGvirtualAtlas*)vk->virtualAtlas,
	                               vk->queue,  // Use graphics queue for compute
	                               0);          // Queue family 0 (graphics)

	if (result == VK_SUCCESS) {
		printf("NanoVG Vulkan: GPU MSDF generation enabled (using graphics queue)\n");
	} else {
		printf("NanoVG Vulkan: Warning - Failed to enable GPU MSDF (error %d)\n", result);
		// Not a fatal error - will fall back to CPU rasterization
	}

	return 1;
}

void nvgvk_delete(void* userPtr)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk) {
		return;
	}

	// Wait for device to be idle before cleanup
	vkDeviceWaitIdle(vk->device);

	// Destroy virtual atlas
	if (vk->virtualAtlas) {
		vknvg__destroyVirtualAtlas((VKNVGvirtualAtlas*)vk->virtualAtlas);
		vk->virtualAtlas = NULL;
	}

	// Destroy texture descriptor system
	nvgvk__destroy_texture_descriptors(vk);

	// Destroy buffers
	nvgvk_buffer_destroy(vk, &vk->uniformBuffer);
	nvgvk_buffer_destroy(vk, &vk->vertexBuffer);

	// Free vertex data
	if (vk->vertices) {
		free(vk->vertices);
		vk->vertices = NULL;
	}

	// Destroy fence
	if (vk->uploadFence) {
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vk->uploadFence = VK_NULL_HANDLE;
	}

	// Free command buffer
	if (vk->commandBuffer) {
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		vk->commandBuffer = VK_NULL_HANDLE;
	}
}

void nvgvk_viewport(void* userPtr, float width, float height, float devicePixelRatio)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk) {
		return;
	}

	vk->viewWidth = width;
	vk->viewHeight = height;
	vk->devicePixelRatio = devicePixelRatio;
}

void nvgvk_cancel(void* userPtr)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk) {
		return;
	}

	// Reset render state
	vk->vertexCount = 0;
	vk->pathCount = 0;
	vk->callCount = 0;
	vk->uniformCount = 0;
}

void nvgvk_flush(void* userPtr)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || vk->callCount == 0) {
		return;
	}

	// Upload vertex data to buffer
	if (vk->vertexCount > 0) {
		VkDeviceSize vertexDataSize = vk->vertexCount * sizeof(NVGvertex);
		nvgvk_buffer_upload(vk, &vk->vertexBuffer, vk->vertices, vertexDataSize);
	}

	// Upload view uniforms (viewSize)
	float viewSize[2] = {vk->viewWidth, vk->viewHeight};
	printf("[nvg_vk] viewSize = %.1f x %.1f, devicePixelRatio = %.2f\n", viewSize[0], viewSize[1], vk->devicePixelRatio);
	nvgvk_buffer_upload(vk, &vk->uniformBuffer, viewSize, sizeof(viewSize));

	// Update descriptor sets for all pipelines
	nvgvk_setup_render(vk);

	// Bind vertex buffer
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(vk->commandBuffer, 0, 1, &vk->vertexBuffer.buffer, &offset);

	// Process all render calls
	for (int i = 0; i < vk->callCount; i++) {
		NVGVkCall* call = &vk->calls[i];

		switch (call->type) {
			case NVGVK_FILL:
				nvgvk_render_fill(vk, call);
				break;
			case NVGVK_CONVEXFILL:
				nvgvk_render_convex_fill(vk, call);
				break;
			case NVGVK_STROKE:
				nvgvk_render_stroke(vk, call);
				break;
			case NVGVK_TRIANGLES:
				nvgvk_render_triangles(vk, call);
				break;
			default:
				break;
		}
	}

	nvgvk_cancel(userPtr);
}
