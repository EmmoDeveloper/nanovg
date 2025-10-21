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

#ifndef NANOVG_VK_IMAGE_H
#define NANOVG_VK_IMAGE_H

#include <stdlib.h>
#include <string.h>
#include "nanovg_vk_internal.h"
#include "nanovg_vk_memory.h"

static VkResult vknvg__createImage(VKNVGcontext* vk, int w, int h, VkFormat format,
                                   VkImageUsageFlags usage, VKNVGtexture* tex)
{
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = w;
	imageInfo.extent.height = h;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VkResult result = vkCreateImage(vk->device, &imageInfo, NULL, &tex->image);
	if (result != VK_SUCCESS) return result;

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(vk->device, tex->image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryType(vk->physicalDevice, memRequirements.memoryTypeBits,
	                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(vk->device, &allocInfo, NULL, &tex->memory);
	if (result != VK_SUCCESS) {
		vkDestroyImage(vk->device, tex->image, NULL);
		return result;
	}

	vkBindImageMemory(vk->device, tex->image, tex->memory, 0);

	return VK_SUCCESS;
}

static VkResult vknvg__createImageView(VKNVGcontext* vk, VkImage image, VkFormat format, VkImageView* imageView)
{
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	return vkCreateImageView(vk->device, &viewInfo, NULL, imageView);
}

static VkResult vknvg__createSampler(VKNVGcontext* vk, int imageFlags, VkSampler* sampler)
{
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = (imageFlags & (1<<5)) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.minFilter = (imageFlags & (1<<5)) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.addressModeU = (imageFlags & (1<<1)) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = (imageFlags & (1<<2)) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	return vkCreateSampler(vk->device, &samplerInfo, NULL, sampler);
}

static void vknvg__transitionImageLayout(VKNVGcontext* vk, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vk->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->queue);

	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &commandBuffer);
}

static void vknvg__copyBufferToImage(VKNVGcontext* vk, VkBuffer buffer, VkImage image,
                                       int x, int y, uint32_t width, uint32_t height)
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vk->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = x;
	region.imageOffset.y = y;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->queue);

	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &commandBuffer);
}

static int vknvg__allocTexture(VKNVGcontext* vk)
{
	VKNVGtexture* tex = NULL;
	int i;

	for (i = 0; i < vk->ntextures; i++) {
		if (vk->textures[i].id == 0) {
			tex = &vk->textures[i];
			break;
		}
	}

	if (tex == NULL) {
		if (vk->ntextures + 1 > vk->ctextures) {
			VKNVGtexture* textures;
			int ctextures = (vk->ctextures == 0) ? 4 : vk->ctextures * 2;
			textures = (VKNVGtexture*)realloc(vk->textures, sizeof(VKNVGtexture) * ctextures);
			if (textures == NULL) return 0;
			vk->textures = textures;
			vk->ctextures = ctextures;
		}
		tex = &vk->textures[vk->ntextures++];
	}

	memset(tex, 0, sizeof(VKNVGtexture));
	tex->id = ++vk->textureId;

	return tex->id;
}

static VKNVGtexture* vknvg__findTexture(VKNVGcontext* vk, int id)
{
	for (int i = 0; i < vk->ntextures; i++) {
		if (vk->textures[i].id == id) {
			return &vk->textures[i];
		}
	}
	return NULL;
}

#endif // NANOVG_VK_IMAGE_H
