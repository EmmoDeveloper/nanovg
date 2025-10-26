#include "nvg_vk_context.h"
#include "nvg_vk_buffer.h"
#include "nvg_vk_render.h"
#include "../nanovg.h"
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

	// Initialize vertex data
	vk->vertexCapacity = NVGVK_INITIAL_VERTEX_COUNT;
	vk->vertices = (float*)malloc(vk->vertexCapacity * sizeof(NVGvertex));
	if (!vk->vertices) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate vertex buffer\n");
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Create vertex buffer
	VkDeviceSize vertexBufferSize = vk->vertexCapacity * sizeof(NVGvertex);
	if (!nvgvk_buffer_create(vk, &vk->vertexBuffer, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create vertex buffer\n");
		free(vk->vertices);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	// Create uniform buffer (for viewSize)
	VkDeviceSize uniformBufferSize = sizeof(float) * 2;
	if (!nvgvk_buffer_create(vk, &vk->uniformBuffer, uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create uniform buffer\n");
		nvgvk_buffer_destroy(vk, &vk->vertexBuffer);
		free(vk->vertices);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
	}

	printf("NanoVG Vulkan: Context created successfully\n");
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

	// Destroy buffers
	nvgvk_buffer_destroy(vk, &vk->uniformBuffer);
	nvgvk_buffer_destroy(vk, &vk->vertexBuffer);

	// Free vertex data
	if (vk->vertices) {
		free(vk->vertices);
		vk->vertices = NULL;
	}

	// Free command buffer
	if (vk->commandBuffer) {
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		vk->commandBuffer = VK_NULL_HANDLE;
	}

	printf("NanoVG Vulkan: Context deleted\n");
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

	printf("NanoVG Vulkan: Flush called (vertices: %d, calls: %d)\n",
	       vk->vertexCount, vk->callCount);

	// Upload vertex data to buffer
	if (vk->vertexCount > 0) {
		VkDeviceSize vertexDataSize = vk->vertexCount * sizeof(NVGvertex);
		nvgvk_buffer_upload(vk, &vk->vertexBuffer, vk->vertices, vertexDataSize);
	}

	// Upload view uniforms (viewSize)
	float viewSize[2] = {vk->viewWidth, vk->viewHeight};
	nvgvk_buffer_upload(vk, &vk->uniformBuffer, viewSize, sizeof(viewSize));

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
