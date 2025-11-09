#include "nvg_vk_context.h"
#include "nvg_vk_buffer.h"
#include "nvg_vk_render.h"
#include "nvg_vk_texture.h"
#include "nvg_vk_color_space_ubo.h"
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

	// Initialize color space descriptor layout (needed before pipeline creation)
	if (!nvgvk_init_color_space_layout(vk)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to initialize color space layout\n");
		nvgvk__destroy_texture_descriptors(vk);
		nvgvk_buffer_destroy(vk, &vk->uniformBuffer);
		nvgvk_buffer_destroy(vk, &vk->vertexBuffer);
		free(vk->vertices);
		vkDestroyFence(vk->device, vk->uploadFence, NULL);
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &vk->commandBuffer);
		return 0;
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

	printf("[FLUSH] inRenderPass=%d, callCount=%d\n", vk->inRenderPass, vk->callCount);

	// Upload vertex data to buffer
	if (vk->vertexCount > 0) {
		VkDeviceSize vertexDataSize = vk->vertexCount * sizeof(NVGvertex);
		printf("[nvgvk_flush] Uploading %d vertices (%zu bytes) to GPU\n",
			vk->vertexCount, vertexDataSize);

		// Debug: print vertices at different offsets
		NVGvertex* verts = (NVGvertex*)vk->vertices;
		printf("[VERTEX DEBUG] Total vertex count: %d\n", vk->vertexCount);
		printf("[VERTEX DEBUG] First 5 vertices (title text):\n");
		for (int i = 18; i < 23 && i < vk->vertexCount; i++) {
			printf("  Vert %d: pos=(%.1f,%.1f) uv=(%.4f,%.4f)\n",
				i, (double)verts[i].x, (double)verts[i].y,
				(double)verts[i].u, (double)verts[i].v);
		}
		printf("[VERTEX DEBUG] First test string vertices (offset ~162):\n");
		for (int i = 162; i < 167 && i < vk->vertexCount; i++) {
			printf("  Vert %d: pos=(%.1f,%.1f) uv=(%.4f,%.4f)\n",
				i, (double)verts[i].x, (double)verts[i].y,
				(double)verts[i].u, (double)verts[i].v);
		}

		nvgvk_buffer_upload(vk, &vk->vertexBuffer, vk->vertices, vertexDataSize);

		// Verify upload: read back vertices from GPU buffer at same offsets
		NVGvertex* gpuVerts = (NVGvertex*)vk->vertexBuffer.mapped;
		printf("[VERIFY GPU] Vert 18 (title): pos=(%.1f,%.1f) uv=(%.4f,%.4f)\n",
			(double)gpuVerts[18].x, (double)gpuVerts[18].y,
			(double)gpuVerts[18].u, (double)gpuVerts[18].v);
		printf("[VERIFY GPU] Vert 162 (test string): pos=(%.1f,%.1f) uv=(%.4f,%.4f)\n",
			(double)gpuVerts[162].x, (double)gpuVerts[162].y,
			(double)gpuVerts[162].u, (double)gpuVerts[162].v);
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
	printf("[FLUSH] Bound vertex buffer %p with %d vertices uploaded\n",
	       (void*)vk->vertexBuffer.buffer, vk->vertexCount);

	// Bind color space UBO (set = 1) if available
	// This is bound once per frame and shared across all draw calls
	if (vk->colorSpaceDescriptorSet != VK_NULL_HANDLE && vk->pipelines[0].layout != VK_NULL_HANDLE) {
		vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        vk->pipelines[0].layout, 1, 1, &vk->colorSpaceDescriptorSet, 0, NULL);
	}

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
