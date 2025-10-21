// nanovg_vk_text_instance.h - Glyph Instancing System for Text Rendering
// Reduces vertex count from 6 vertices/glyph to 1 instance/glyph (75% reduction)

#ifndef NANOVG_VK_TEXT_INSTANCE_H
#define NANOVG_VK_TEXT_INSTANCE_H

#include <stdlib.h>
#include <string.h>

// Glyph instance data (32 bytes per glyph)
// Vertex shader will generate 4 vertices (quad) per instance
typedef struct {
	float position[2];      // Screen position (x, y)
	float size[2];          // Glyph size (width, height)
	float uvOffset[2];      // Atlas UV offset
	float uvSize[2];        // Atlas UV size
} VKNVGglyphInstance;

// Maximum glyphs per batch
#define VKNVG_MAX_GLYPH_INSTANCES 65536

// Glyph instance buffer
struct VKNVGglyphInstanceBuffer {
	VKNVGglyphInstance* instances;
	int count;
	int capacity;
	VKNVGbuffer* buffer;          // Vulkan buffer
	void* mappedData;             // Mapped pointer for updates
};

// Initialize glyph instance buffer
static VkResult vknvg__createGlyphInstanceBuffer(VKNVGcontext* vk, VKNVGglyphInstanceBuffer* instBuffer)
{
	if (vk == NULL || instBuffer == NULL) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	memset(instBuffer, 0, sizeof(VKNVGglyphInstanceBuffer));

	// Allocate CPU-side instance array
	instBuffer->capacity = VKNVG_MAX_GLYPH_INSTANCES;
	instBuffer->instances = (VKNVGglyphInstance*)malloc(instBuffer->capacity * sizeof(VKNVGglyphInstance));
	if (instBuffer->instances == NULL) {
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	instBuffer->count = 0;

	// Create Vulkan buffer (host-visible for dynamic updates)
	instBuffer->buffer = (VKNVGbuffer*)malloc(sizeof(VKNVGbuffer));
	if (instBuffer->buffer == NULL) {
		free(instBuffer->instances);
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	VkDeviceSize bufferSize = instBuffer->capacity * sizeof(VKNVGglyphInstance);
	VkResult result = vknvg__createBuffer(vk, bufferSize,
	                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                       instBuffer->buffer);
	if (result != VK_SUCCESS) {
		free(instBuffer->instances);
		free(instBuffer->buffer);
		return result;
	}

	// Map buffer for persistent mapping
	result = vkMapMemory(vk->device, instBuffer->buffer->memory, 0, bufferSize, 0, &instBuffer->mappedData);
	if (result != VK_SUCCESS) {
		vknvg__destroyBuffer(vk, instBuffer->buffer);
		free(instBuffer->instances);
		free(instBuffer->buffer);
		return result;
	}

	return VK_SUCCESS;
}

// Destroy glyph instance buffer
static void vknvg__destroyGlyphInstanceBuffer(VKNVGcontext* vk, VKNVGglyphInstanceBuffer* instBuffer)
{
	if (vk == NULL || instBuffer == NULL) {
		return;
	}

	if (instBuffer->buffer != NULL) {
		if (instBuffer->mappedData != NULL) {
			vkUnmapMemory(vk->device, instBuffer->buffer->memory);
		}
		vknvg__destroyBuffer(vk, instBuffer->buffer);
		free(instBuffer->buffer);
	}

	if (instBuffer->instances != NULL) {
		free(instBuffer->instances);
	}

	memset(instBuffer, 0, sizeof(VKNVGglyphInstanceBuffer));
}

// Add glyph instance to buffer
static int vknvg__addGlyphInstance(VKNVGglyphInstanceBuffer* instBuffer,
                                    float posX, float posY,
                                    float width, float height,
                                    float uvX, float uvY,
                                    float uvW, float uvH)
{
	if (instBuffer == NULL || instBuffer->count >= instBuffer->capacity) {
		return -1;
	}

	VKNVGglyphInstance* inst = &instBuffer->instances[instBuffer->count];
	inst->position[0] = posX;
	inst->position[1] = posY;
	inst->size[0] = width;
	inst->size[1] = height;
	inst->uvOffset[0] = uvX;
	inst->uvOffset[1] = uvY;
	inst->uvSize[0] = uvW;
	inst->uvSize[1] = uvH;

	return instBuffer->count++;
}

// Upload instance data to GPU
static void vknvg__uploadGlyphInstances(VKNVGglyphInstanceBuffer* instBuffer)
{
	if (instBuffer == NULL || instBuffer->mappedData == NULL || instBuffer->count == 0) {
		return;
	}

	// Copy to mapped buffer (already host-coherent, no flush needed)
	VkDeviceSize copySize = instBuffer->count * sizeof(VKNVGglyphInstance);
	memcpy(instBuffer->mappedData, instBuffer->instances, copySize);
}

// Reset instance buffer for next frame
static void vknvg__resetGlyphInstances(VKNVGglyphInstanceBuffer* instBuffer)
{
	if (instBuffer != NULL) {
		instBuffer->count = 0;
	}
}

// Get vertex input binding description for instances
static VkVertexInputBindingDescription vknvg__getInstanceBindingDescription(void)
{
	VkVertexInputBindingDescription bindingDescription = {0};
	bindingDescription.binding = 1;  // Binding 1 for instances (0 is for regular vertices)
	bindingDescription.stride = sizeof(VKNVGglyphInstance);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	return bindingDescription;
}

// Get vertex input attribute descriptions for instances
static void vknvg__getInstanceAttributeDescriptions(VkVertexInputAttributeDescription* attributes)
{
	// Attribute 2: position (vec2)
	attributes[0].binding = 1;
	attributes[0].location = 2;
	attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[0].offset = offsetof(VKNVGglyphInstance, position);

	// Attribute 3: size (vec2)
	attributes[1].binding = 1;
	attributes[1].location = 3;
	attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[1].offset = offsetof(VKNVGglyphInstance, size);

	// Attribute 4: uvOffset (vec2)
	attributes[2].binding = 1;
	attributes[2].location = 4;
	attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[2].offset = offsetof(VKNVGglyphInstance, uvOffset);

	// Attribute 5: uvSize (vec2)
	attributes[3].binding = 1;
	attributes[3].location = 5;
	attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributes[3].offset = offsetof(VKNVGglyphInstance, uvSize);
}

#endif // NANOVG_VK_TEXT_INSTANCE_H
