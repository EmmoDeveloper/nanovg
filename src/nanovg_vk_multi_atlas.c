// Multi-Atlas Support - Scalable Atlas Management Implementation

#include "nanovg_vk_multi_atlas.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Find memory type for atlas
uint32_t vknvg__findAtlasMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	return UINT32_MAX;
}

// Create a single atlas instance with specific size
VkResult vknvg__createAtlasInstanceWithSize(VKNVGatlasManager* manager,
                                            uint32_t index,
                                            uint16_t atlasSize)
{
	if (!manager || index >= VKNVG_MAX_ATLASES) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[index];
	VkResult result;

	// Initialize packer
	vknvg__initAtlasPacker(&atlas->packer, atlasSize, atlasSize);

	// Create image
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = manager->format;
	imageInfo.extent.width = atlasSize;
	imageInfo.extent.height = atlasSize;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	result = vkCreateImage(manager->device, &imageInfo, NULL, &atlas->image);
	if (result != VK_SUCCESS) {
		return result;
	}

	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(manager->device, atlas->image, &memReqs);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findAtlasMemoryType(
		manager->physicalDevice,
		memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyImage(manager->device, atlas->image, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	result = vkAllocateMemory(manager->device, &allocInfo, NULL, &atlas->memory);
	if (result != VK_SUCCESS) {
		vkDestroyImage(manager->device, atlas->image, NULL);
		return result;
	}

	vkBindImageMemory(manager->device, atlas->image, atlas->memory, 0);

	// Create image view
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = atlas->image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = manager->format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	result = vkCreateImageView(manager->device, &viewInfo, NULL, &atlas->imageView);
	if (result != VK_SUCCESS) {
		vkFreeMemory(manager->device, atlas->memory, NULL);
		vkDestroyImage(manager->device, atlas->image, NULL);
		return result;
	}

	// Allocate descriptor set
	VkDescriptorSetAllocateInfo descAllocInfo = {0};
	descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descAllocInfo.descriptorPool = manager->descriptorPool;
	descAllocInfo.descriptorSetCount = 1;
	descAllocInfo.pSetLayouts = &manager->descriptorSetLayout;

	result = vkAllocateDescriptorSets(manager->device, &descAllocInfo, &atlas->descriptorSet);
	if (result != VK_SUCCESS) {
		vkDestroyImageView(manager->device, atlas->imageView, NULL);
		vkFreeMemory(manager->device, atlas->memory, NULL);
		vkDestroyImage(manager->device, atlas->image, NULL);
		return result;
	}

	// Update descriptor set
	VkDescriptorImageInfo imageDescInfo = {0};
	imageDescInfo.sampler = manager->sampler;
	imageDescInfo.imageView = atlas->imageView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writeDesc = {0};
	writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDesc.dstSet = atlas->descriptorSet;
	writeDesc.dstBinding = 1;	// Texture binding
	writeDesc.dstArrayElement = 0;
	writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDesc.descriptorCount = 1;
	writeDesc.pImageInfo = &imageDescInfo;

	vkUpdateDescriptorSets(manager->device, 1, &writeDesc, 0, NULL);

	atlas->active = 1;

	return VK_SUCCESS;
}

// Destroy atlas instance
void vknvg__destroyAtlasInstance(VKNVGatlasManager* manager,
                                 uint32_t index)
{
	if (!manager || index >= VKNVG_MAX_ATLASES) {
		return;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[index];

	if (!atlas->active) {
		return;
	}

	// Note: descriptor set is freed automatically when pool is destroyed

	if (atlas->imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(manager->device, atlas->imageView, NULL);
	}
	if (atlas->memory != VK_NULL_HANDLE) {
		vkFreeMemory(manager->device, atlas->memory, NULL);
	}
	if (atlas->image != VK_NULL_HANDLE) {
		vkDestroyImage(manager->device, atlas->image, NULL);
	}

	memset(atlas, 0, sizeof(VKNVGatlasInstance));
}

// Create atlas instance with manager's default size
VkResult vknvg__createAtlasInstance(VKNVGatlasManager* manager,
                                    uint32_t index)
{
	return vknvg__createAtlasInstanceWithSize(manager, index, manager->atlasSize);
}

// Resize an existing atlas to a larger size
// This creates a new atlas and copies content using a command buffer
VkResult vknvg__resizeAtlasInstance(VKNVGatlasManager* manager,
                                    uint32_t index,
                                    uint16_t newSize,
                                    VkCommandBuffer cmdBuffer)
{
	if (!manager || index >= manager->atlasCount) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	VKNVGatlasInstance* oldAtlas = &manager->atlases[index];
	uint16_t oldSize = oldAtlas->packer.atlasWidth;

	if (newSize <= oldSize) {
		return VK_ERROR_INITIALIZATION_FAILED;	// Must grow
	}

	// Save old atlas state
	VkImage oldImage = oldAtlas->image;
	VkImageView oldImageView = oldAtlas->imageView;
	VkDeviceMemory oldMemory = oldAtlas->memory;
	VKNVGatlasPacker oldPacker = oldAtlas->packer;

	// Create new atlas (reusing the same slot)
	memset(oldAtlas, 0, sizeof(VKNVGatlasInstance));
	VkResult result = vknvg__createAtlasInstanceWithSize(manager, index, newSize);
	if (result != VK_SUCCESS) {
		// Restore old atlas on failure
		oldAtlas->image = oldImage;
		oldAtlas->imageView = oldImageView;
		oldAtlas->memory = oldMemory;
		oldAtlas->packer = oldPacker;
		oldAtlas->active = 1;
		return result;
	}

	// Copy old packer allocations to new packer
	// Note: The packer state is preserved by copying allocated regions
	oldAtlas->packer.allocatedArea = oldPacker.allocatedArea;
	oldAtlas->packer.allocationCount = oldPacker.allocationCount;

	// Transition old image to TRANSFER_SRC
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = oldImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Transition new image to TRANSFER_DST
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.image = oldAtlas->image;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy old atlas content to new atlas
	VkImageCopy copyRegion = {0};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent.width = oldSize;
	copyRegion.extent.height = oldSize;
	copyRegion.extent.depth = 1;

	vkCmdCopyImage(cmdBuffer,
	               oldImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               oldAtlas->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &copyRegion);

	// Transition new image to SHADER_READ
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.image = oldAtlas->image;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Destroy old atlas resources
	vkDestroyImageView(manager->device, oldImageView, NULL);
	vkFreeMemory(manager->device, oldMemory, NULL);
	vkDestroyImage(manager->device, oldImage, NULL);

	manager->resizeCount++;

	return VK_SUCCESS;
}

// Initialize atlas manager
VKNVGatlasManager* vknvg__createAtlasManager(VkDevice device,
                                             VkPhysicalDevice physicalDevice,
                                             VkDescriptorPool descriptorPool,
                                             VkDescriptorSetLayout descriptorSetLayout,
                                             VkSampler sampler,
                                             uint16_t atlasSize)
{
	VKNVGatlasManager* manager = (VKNVGatlasManager*)malloc(sizeof(VKNVGatlasManager));
	if (!manager) {
		return NULL;
	}

	memset(manager, 0, sizeof(VKNVGatlasManager));
	manager->device = device;
	manager->physicalDevice = physicalDevice;
	manager->descriptorPool = descriptorPool;
	manager->descriptorSetLayout = descriptorSetLayout;
	manager->sampler = sampler;
	manager->atlasSize = atlasSize;
	manager->format = VK_FORMAT_R8G8B8A8_UNORM;  // RGBA to support MSDF (RGB) and grayscale (R only)
	manager->packingHeuristic = VKNVG_PACK_BEST_AREA_FIT;
	manager->splitRule = VKNVG_SPLIT_SHORTER_AXIS;

	// Resizing configuration
	manager->enableDynamicGrowth = 1;
	manager->resizeThreshold = VKNVG_RESIZE_THRESHOLD;
	manager->minAtlasSize = VKNVG_MIN_ATLAS_SIZE;
	manager->maxAtlasSize = VKNVG_DEFAULT_ATLAS_SIZE;

	// Create initial atlas
	if (vknvg__createAtlasInstance(manager, 0) != VK_SUCCESS) {
		free(manager);
		return NULL;
	}

	manager->atlasCount = 1;
	manager->currentAtlas = 0;

	return manager;
}

// Destroy atlas manager
void vknvg__destroyAtlasManager(VKNVGatlasManager* manager)
{
	if (!manager) {
		return;
	}

	// Destroy all atlases
	for (uint32_t i = 0; i < manager->atlasCount; i++) {
		vknvg__destroyAtlasInstance(manager, i);
	}

	free(manager);
}

// Get next size in growth progression
uint16_t vknvg__getNextAtlasSize(uint16_t currentSize, uint16_t maxSize)
{
	if (currentSize >= maxSize) {
		return 0;	// Already at max
	}

	// Progressive growth: 512 -> 1024 -> 2048 -> 4096
	uint16_t nextSize = currentSize * 2;
	if (nextSize > maxSize) {
		nextSize = maxSize;
	}

	return nextSize;
}

// Check if atlas should be resized based on utilization
int vknvg__shouldResizeAtlas(VKNVGatlasManager* manager, uint32_t atlasIndex)
{
	if (!manager->enableDynamicGrowth) {
		return 0;
	}

	if (atlasIndex >= manager->atlasCount) {
		return 0;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[atlasIndex];
	float efficiency = vknvg__getPackingEfficiency(&atlas->packer);

	// Check if over threshold and can grow
	if (efficiency >= manager->resizeThreshold) {
		uint16_t currentSize = atlas->packer.atlasWidth;
		uint16_t nextSize = vknvg__getNextAtlasSize(currentSize, manager->maxAtlasSize);
		return nextSize > 0;
	}

	return 0;
}

// Allocate space in atlas system
// Returns atlas index in atlasIndex, position in outRect
int vknvg__multiAtlasAlloc(VKNVGatlasManager* manager,
                           uint16_t width,
                           uint16_t height,
                           uint32_t* atlasIndex,
                           VKNVGrect* outRect)
{
	if (!manager || !atlasIndex || !outRect) {
		return 0;
	}

	// Try current atlas first
	VKNVGatlasInstance* atlas = &manager->atlases[manager->currentAtlas];
	if (vknvg__packRect(&atlas->packer, width, height, outRect,
	                     manager->packingHeuristic, manager->splitRule)) {
		*atlasIndex = manager->currentAtlas;
		atlas->glyphCount++;
		atlas->allocationCount++;
		manager->totalAllocations++;
		return 1;
	}

	// Try other existing atlases
	for (uint32_t i = 0; i < manager->atlasCount; i++) {
		if (i == manager->currentAtlas) continue;	// Already tried

		atlas = &manager->atlases[i];
		if (vknvg__packRect(&atlas->packer, width, height, outRect,
		                     manager->packingHeuristic, manager->splitRule)) {
			*atlasIndex = i;
			atlas->glyphCount++;
			atlas->allocationCount++;
			manager->totalAllocations++;
			return 1;
		}
	}

	// Need new atlas
	if (manager->atlasCount >= VKNVG_MAX_ATLASES) {
		manager->failedAllocations++;
		return 0;	// Max atlases reached
	}

	// Create new atlas
	if (vknvg__createAtlasInstance(manager, manager->atlasCount) != VK_SUCCESS) {
		manager->failedAllocations++;
		return 0;
	}

	manager->currentAtlas = manager->atlasCount;
	manager->atlasCount++;

	// Allocate in new atlas
	atlas = &manager->atlases[manager->currentAtlas];
	if (vknvg__packRect(&atlas->packer, width, height, outRect,
	                     manager->packingHeuristic, manager->splitRule)) {
		*atlasIndex = manager->currentAtlas;
		atlas->glyphCount++;
		atlas->allocationCount++;
		manager->totalAllocations++;
		return 1;
	}

	manager->failedAllocations++;
	return 0;	// Shouldn't happen with empty atlas
}

// Get atlas descriptor set
VkDescriptorSet vknvg__getAtlasDescriptorSet(VKNVGatlasManager* manager,
                                             uint32_t atlasIndex)
{
	if (!manager || atlasIndex >= manager->atlasCount) {
		return VK_NULL_HANDLE;
	}

	return manager->atlases[atlasIndex].descriptorSet;
}

// Get total space utilization across all atlases
float vknvg__getMultiAtlasEfficiency(VKNVGatlasManager* manager)
{
	if (!manager || manager->atlasCount == 0) {
		return 0.0f;
	}

	uint32_t totalAllocated = 0;
	uint32_t totalCapacity = 0;

	for (uint32_t i = 0; i < manager->atlasCount; i++) {
		VKNVGatlasInstance* atlas = &manager->atlases[i];
		totalAllocated += atlas->packer.allocatedArea;
		totalCapacity += atlas->packer.totalArea;
	}

	return totalCapacity > 0 ? (float)totalAllocated / (float)totalCapacity : 0.0f;
}
