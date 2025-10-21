// Multi-Atlas Support - Scalable Atlas Management
//
// Allows the system to use multiple texture atlases instead of a single fixed-size atlas.
// When one atlas fills up, a new one is automatically allocated.
//
// Architecture:
// - VKNVGatlasManager: Central manager for multiple atlases
// - Each atlas has its own VkImage, packer, and descriptor set
// - Glyphs track which atlas they belong to (atlas index)
// - Rendering switches atlases as needed

#ifndef NANOVG_VK_MULTI_ATLAS_H
#define NANOVG_VK_MULTI_ATLAS_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg_vk_atlas_packing.h"

#define VKNVG_MAX_ATLASES 8			// Maximum number of atlases
#define VKNVG_DEFAULT_ATLAS_SIZE 4096	// Default atlas size (4096x4096)

// Single atlas instance
typedef struct VKNVGatlasInstance {
	// Vulkan resources
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
	VkDescriptorSet descriptorSet;	// Descriptor set for this atlas

	// Packing state
	VKNVGatlasPacker packer;

	// Statistics
	uint32_t glyphCount;
	uint32_t allocationCount;

	// State
	uint8_t active;		// Is this atlas active?
	uint8_t padding[3];
} VKNVGatlasInstance;

// Multi-atlas manager
typedef struct VKNVGatlasManager {
	// Vulkan context
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkSampler sampler;

	// Atlas instances
	VKNVGatlasInstance atlases[VKNVG_MAX_ATLASES];
	uint32_t atlasCount;
	uint32_t currentAtlas;		// Current atlas for new allocations

	// Configuration
	uint16_t atlasSize;			// Size of each atlas (width = height)
	VkFormat format;			// Atlas texture format
	VKNVGpackingHeuristic packingHeuristic;
	VKNVGsplitRule splitRule;

	// Statistics
	uint32_t totalGlyphs;
	uint32_t totalAllocations;
	uint32_t failedAllocations;
} VKNVGatlasManager;

// Find memory type for atlas
static uint32_t vknvg__findAtlasMemoryType(VkPhysicalDevice physicalDevice,
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

// Create a single atlas instance
static VkResult vknvg__createAtlasInstance(VKNVGatlasManager* manager,
                                            uint32_t index)
{
	if (!manager || index >= VKNVG_MAX_ATLASES) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	VKNVGatlasInstance* atlas = &manager->atlases[index];
	VkResult result;

	// Initialize packer
	vknvg__initAtlasPacker(&atlas->packer, manager->atlasSize, manager->atlasSize);

	// Create image
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = manager->format;
	imageInfo.extent.width = manager->atlasSize;
	imageInfo.extent.height = manager->atlasSize;
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
static void vknvg__destroyAtlasInstance(VKNVGatlasManager* manager,
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

// Initialize atlas manager
static VKNVGatlasManager* vknvg__createAtlasManager(VkDevice device,
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
	manager->format = VK_FORMAT_R8_UNORM;
	manager->packingHeuristic = VKNVG_PACK_BEST_AREA_FIT;
	manager->splitRule = VKNVG_SPLIT_SHORTER_AXIS;

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
static void vknvg__destroyAtlasManager(VKNVGatlasManager* manager)
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

// Allocate space in atlas system
// Returns atlas index in atlasIndex, position in outRect
static int vknvg__multiAtlasAlloc(VKNVGatlasManager* manager,
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
static VkDescriptorSet vknvg__getAtlasDescriptorSet(VKNVGatlasManager* manager,
                                                      uint32_t atlasIndex)
{
	if (!manager || atlasIndex >= manager->atlasCount) {
		return VK_NULL_HANDLE;
	}

	return manager->atlases[atlasIndex].descriptorSet;
}

// Get total space utilization across all atlases
static float vknvg__getMultiAtlasEfficiency(VKNVGatlasManager* manager)
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

#endif // NANOVG_VK_MULTI_ATLAS_H
