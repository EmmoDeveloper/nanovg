//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
// Copyright (c) 2025 NanoVG Vulkan Backend Contributors
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef NANOVG_VK_MEMORY_H
#define NANOVG_VK_MEMORY_H

#include "nanovg_vk_internal.h"

static uint32_t vknvg__findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}

static VkResult vknvg__createBuffer(VKNVGcontext* vk, VkDeviceSize size, VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags properties, VKNVGbuffer* buffer)
{
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(vk->device, &bufferInfo, NULL, &buffer->buffer);
	if (result != VK_SUCCESS) return result;

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, buffer->buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryType(vk->physicalDevice, memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(vk->device, &allocInfo, NULL, &buffer->memory);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		return result;
	}

	vkBindBufferMemory(vk->device, buffer->buffer, buffer->memory, 0);

	buffer->size = size;
	buffer->used = 0;

	if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		vkMapMemory(vk->device, buffer->memory, 0, size, 0, &buffer->mapped);
	} else {
		buffer->mapped = NULL;
	}

	return VK_SUCCESS;
}

static void vknvg__destroyBuffer(VKNVGcontext* vk, VKNVGbuffer* buffer)
{
	if (buffer->mapped) {
		vkUnmapMemory(vk->device, buffer->memory);
		buffer->mapped = NULL;
	}
	if (buffer->buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
		buffer->buffer = VK_NULL_HANDLE;
	}
	if (buffer->memory != VK_NULL_HANDLE) {
		vkFreeMemory(vk->device, buffer->memory, NULL);
		buffer->memory = VK_NULL_HANDLE;
	}
}

#endif // NANOVG_VK_MEMORY_H
