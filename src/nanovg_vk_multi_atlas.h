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
#include "nanovg_vk_atlas_packing.h"

#define VKNVG_MAX_ATLASES 8			// Maximum number of atlases
#define VKNVG_DEFAULT_ATLAS_SIZE 4096	// Default atlas size (4096x4096)
#define VKNVG_MIN_ATLAS_SIZE 512		// Minimum atlas size (512x512)
#define VKNVG_RESIZE_THRESHOLD 0.85f	// Resize when 85% full

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

	// Resizing configuration
	uint8_t enableDynamicGrowth;	// Enable automatic atlas growth
	float resizeThreshold;			// Utilization threshold for resize (0.0-1.0)
	uint16_t minAtlasSize;			// Minimum atlas size
	uint16_t maxAtlasSize;			// Maximum atlas size

	// Statistics
	uint32_t totalGlyphs;
	uint32_t totalAllocations;
	uint32_t failedAllocations;
	uint32_t resizeCount;			// Number of times atlases were resized
} VKNVGatlasManager;

// Function declarations
uint32_t vknvg__findAtlasMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);

VkResult vknvg__createAtlasInstanceWithSize(VKNVGatlasManager* manager,
                                            uint32_t index,
                                            uint16_t atlasSize);

void vknvg__destroyAtlasInstance(VKNVGatlasManager* manager,
                                 uint32_t index);

VkResult vknvg__createAtlasInstance(VKNVGatlasManager* manager,
                                    uint32_t index);

VkResult vknvg__resizeAtlasInstance(VKNVGatlasManager* manager,
                                    uint32_t index,
                                    uint16_t newSize,
                                    VkCommandBuffer cmdBuffer);

VKNVGatlasManager* vknvg__createAtlasManager(VkDevice device,
                                             VkPhysicalDevice physicalDevice,
                                             VkDescriptorPool descriptorPool,
                                             VkDescriptorSetLayout descriptorSetLayout,
                                             VkSampler sampler,
                                             uint16_t atlasSize);

void vknvg__destroyAtlasManager(VKNVGatlasManager* manager);

uint16_t vknvg__getNextAtlasSize(uint16_t currentSize, uint16_t maxSize);

int vknvg__shouldResizeAtlas(VKNVGatlasManager* manager, uint32_t atlasIndex);

int vknvg__multiAtlasAlloc(VKNVGatlasManager* manager,
                           uint16_t width,
                           uint16_t height,
                           uint32_t* atlasIndex,
                           VKNVGrect* outRect);

VkDescriptorSet vknvg__getAtlasDescriptorSet(VKNVGatlasManager* manager,
                                             uint32_t atlasIndex);

float vknvg__getMultiAtlasEfficiency(VKNVGatlasManager* manager);

#endif // NANOVG_VK_MULTI_ATLAS_H
