#include "nvg_vk_buffer.h"
#include <stdio.h>
#include <string.h>

// Helper function to find suitable memory type
static uint32_t nvgvk__find_memory_type(NVGVkContext* vk, uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	fprintf(stderr, "NanoVG Vulkan: Failed to find suitable memory type\n");
	return UINT32_MAX;
}

int nvgvk_buffer_create(NVGVkContext* vk, NVGVkBuffer* buffer,
                        VkDeviceSize size, VkBufferUsageFlags usage)
{
	if (!vk || !buffer || size == 0) {
		return 0;
	}

	memset(buffer, 0, sizeof(NVGVkBuffer));
	buffer->usage = usage;
	buffer->capacity = size;

	// Create buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vk->device, &bufferInfo, NULL, &buffer->buffer) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create buffer\n");
		return 0;
	}

	// Get memory requirements
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, buffer->buffer, &memRequirements);

	// Allocate memory (host-visible and coherent for easy CPU access)
	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = nvgvk__find_memory_type(vk,
		memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		return 0;
	}

	if (vkAllocateMemory(vk->device, &allocInfo, NULL, &buffer->memory) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate buffer memory\n");
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		return 0;
	}

	// Bind buffer memory
	vkBindBufferMemory(vk->device, buffer->buffer, buffer->memory, 0);

	// Map memory persistently
	if (vkMapMemory(vk->device, buffer->memory, 0, size, 0, &buffer->mapped) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to map buffer memory\n");
		vkFreeMemory(vk->device, buffer->memory, NULL);
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		return 0;
	}

	return 1;
}

void nvgvk_buffer_destroy(NVGVkContext* vk, NVGVkBuffer* buffer)
{
	if (!vk || !buffer) {
		return;
	}

	if (buffer->mapped) {
		vkUnmapMemory(vk->device, buffer->memory);
		buffer->mapped = NULL;
	}

	if (buffer->memory) {
		vkFreeMemory(vk->device, buffer->memory, NULL);
		buffer->memory = VK_NULL_HANDLE;
	}

	if (buffer->buffer) {
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		buffer->buffer = VK_NULL_HANDLE;
	}

	buffer->size = 0;
	buffer->capacity = 0;
}

int nvgvk_buffer_upload(NVGVkContext* vk, NVGVkBuffer* buffer,
                        const void* data, VkDeviceSize size)
{
	if (!vk || !buffer || !data || size == 0) {
		return 0;
	}

	// Ensure buffer has enough capacity
	if (size > buffer->capacity) {
		if (!nvgvk_buffer_reserve(vk, buffer, size)) {
			return 0;
		}
	}

	// Copy data to mapped memory
	memcpy(buffer->mapped, data, size);
	buffer->size = size;

	return 1;
}

int nvgvk_buffer_reserve(NVGVkContext* vk, NVGVkBuffer* buffer,
                         VkDeviceSize size)
{
	if (!vk || !buffer) {
		return 0;
	}

	// If already large enough, do nothing
	if (size <= buffer->capacity) {
		return 1;
	}

	// Round up to next power of 2
	VkDeviceSize newCapacity = buffer->capacity;
	if (newCapacity == 0) {
		newCapacity = 4096;
	}
	while (newCapacity < size) {
		newCapacity *= 2;
	}

	// Create new buffer
	NVGVkBuffer newBuffer;
	if (!nvgvk_buffer_create(vk, &newBuffer, newCapacity, buffer->usage)) {
		return 0;
	}

	// Copy existing data if any
	if (buffer->size > 0 && buffer->mapped && newBuffer.mapped) {
		memcpy(newBuffer.mapped, buffer->mapped, buffer->size);
		newBuffer.size = buffer->size;
	}

	// Destroy old buffer
	nvgvk_buffer_destroy(vk, buffer);

	// Replace with new buffer
	*buffer = newBuffer;

	return 1;
}

void nvgvk_buffer_reset(NVGVkBuffer* buffer)
{
	if (buffer) {
		buffer->size = 0;
	}
}
