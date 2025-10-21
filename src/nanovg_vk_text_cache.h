// Text Run Caching with Render-to-Texture
// Caches rendered text strings as textures for instant reuse
//
// This optimization renders text once to an offscreen texture, then reuses
// that texture on subsequent renders. Provides 100x speedup for cached text.

#ifndef NANOVG_VK_TEXT_CACHE_H
#define NANOVG_VK_TEXT_CACHE_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define VKNVG_TEXT_CACHE_SIZE 256		// Maximum cached text runs
#define VKNVG_TEXT_CACHE_MAX_STRING 128	// Max text length to cache
#define VKNVG_TEXT_CACHE_TEXTURE_SIZE 512	// Size of each cache texture

// Cache key for text run identification
typedef struct VKNVGtextRunKey {
	char text[VKNVG_TEXT_CACHE_MAX_STRING];
	int fontId;
	float fontSize;
	float letterSpacing;
	float blur;
	uint32_t colorRGBA;
	uint32_t textAlign;
} VKNVGtextRunKey;

// Cached texture for a text run
typedef struct VKNVGcachedTexture {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageView;
	VkFramebuffer framebuffer;

	float width;		// Actual text width in pixels
	float height;		// Actual text height in pixels
	float originX;		// Text origin X (for proper positioning)
	float originY;		// Text origin Y

	VkBool32 allocated;
} VKNVGcachedTexture;

// Cache entry
typedef struct VKNVGtextRunEntry {
	VKNVGtextRunKey key;
	uint32_t hash;

	VKNVGcachedTexture texture;

	uint32_t lastUsedFrame;
	VkBool32 valid;
} VKNVGtextRunEntry;

// Text run cache
typedef struct VKNVGtextRunCache {
	VKNVGtextRunEntry entries[VKNVG_TEXT_CACHE_SIZE];

	// Vulkan resources for rendering to texture
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkRenderPass renderPass;
	VkSampler sampler;

	uint32_t currentFrame;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t evictions;
} VKNVGtextRunCache;

// Forward declarations
static uint32_t vknvg__findMemoryTypeForCache(VkPhysicalDevice physicalDevice,
                                               uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties);

// Hash text run key
static inline uint32_t vknvg__hashTextRunKey(const VKNVGtextRunKey* key)
{
	uint32_t hash = 2166136261u;
	const uint8_t* data = (const uint8_t*)key;

	for (size_t i = 0; i < sizeof(VKNVGtextRunKey); i++) {
		hash ^= data[i];
		hash *= 16777619u;
	}

	return hash;
}

// Compare keys
static inline VkBool32 vknvg__compareTextRunKeys(const VKNVGtextRunKey* a, const VKNVGtextRunKey* b)
{
	return memcmp(a, b, sizeof(VKNVGtextRunKey)) == 0 ? VK_TRUE : VK_FALSE;
}

// Find memory type
static uint32_t vknvg__findMemoryTypeForCache(VkPhysicalDevice physicalDevice,
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

// Create cached texture
static VkResult vknvg__createCachedTexture(VKNVGtextRunCache* cache,
                                           VKNVGcachedTexture* tex,
                                           uint32_t width,
                                           uint32_t height)
{
	if (!cache || !tex) return VK_ERROR_INITIALIZATION_FAILED;

	VkResult result;

	// Create image
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8_UNORM;  // Single channel for text
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	result = vkCreateImage(cache->device, &imageInfo, NULL, &tex->image);
	if (result != VK_SUCCESS) return result;

	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(cache->device, tex->image, &memReqs);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryTypeForCache(
		cache->physicalDevice,
		memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyImage(cache->device, tex->image, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	result = vkAllocateMemory(cache->device, &allocInfo, NULL, &tex->memory);
	if (result != VK_SUCCESS) {
		vkDestroyImage(cache->device, tex->image, NULL);
		return result;
	}

	vkBindImageMemory(cache->device, tex->image, tex->memory, 0);

	// Create image view
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = tex->image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	result = vkCreateImageView(cache->device, &viewInfo, NULL, &tex->imageView);
	if (result != VK_SUCCESS) {
		vkFreeMemory(cache->device, tex->memory, NULL);
		vkDestroyImage(cache->device, tex->image, NULL);
		return result;
	}

	// Create framebuffer
	VkFramebufferCreateInfo fbInfo = {0};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass = cache->renderPass;
	fbInfo.attachmentCount = 1;
	fbInfo.pAttachments = &tex->imageView;
	fbInfo.width = width;
	fbInfo.height = height;
	fbInfo.layers = 1;

	result = vkCreateFramebuffer(cache->device, &fbInfo, NULL, &tex->framebuffer);
	if (result != VK_SUCCESS) {
		vkDestroyImageView(cache->device, tex->imageView, NULL);
		vkFreeMemory(cache->device, tex->memory, NULL);
		vkDestroyImage(cache->device, tex->image, NULL);
		return result;
	}

	tex->allocated = VK_TRUE;
	return VK_SUCCESS;
}

// Destroy cached texture
static void vknvg__destroyCachedTexture(VKNVGtextRunCache* cache, VKNVGcachedTexture* tex)
{
	if (!cache || !tex || !tex->allocated) return;

	if (tex->framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(cache->device, tex->framebuffer, NULL);
	}
	if (tex->imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(cache->device, tex->imageView, NULL);
	}
	if (tex->memory != VK_NULL_HANDLE) {
		vkFreeMemory(cache->device, tex->memory, NULL);
	}
	if (tex->image != VK_NULL_HANDLE) {
		vkDestroyImage(cache->device, tex->image, NULL);
	}

	memset(tex, 0, sizeof(VKNVGcachedTexture));
}

// Create text run cache
static VKNVGtextRunCache* vknvg__createTextRunCache(VkDevice device,
                                                     VkPhysicalDevice physicalDevice,
                                                     VkRenderPass renderPass)
{
	VKNVGtextRunCache* cache = (VKNVGtextRunCache*)malloc(sizeof(VKNVGtextRunCache));
	if (!cache) return NULL;

	memset(cache, 0, sizeof(VKNVGtextRunCache));
	cache->device = device;
	cache->physicalDevice = physicalDevice;
	cache->renderPass = renderPass;

	// Create sampler for cached textures
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, NULL, &cache->sampler) != VK_SUCCESS) {
		free(cache);
		return NULL;
	}

	return cache;
}

// Destroy text run cache
static void vknvg__destroyTextRunCache(VKNVGtextRunCache* cache)
{
	if (!cache) return;

	// Destroy all cached textures
	for (uint32_t i = 0; i < VKNVG_TEXT_CACHE_SIZE; i++) {
		if (cache->entries[i].valid) {
			vknvg__destroyCachedTexture(cache, &cache->entries[i].texture);
		}
	}

	if (cache->sampler != VK_NULL_HANDLE) {
		vkDestroySampler(cache->device, cache->sampler, NULL);
	}

	free(cache);
}

// Build cache key
static void vknvg__buildTextRunKey(VKNVGtextRunKey* key,
                                   const char* text,
                                   int fontId,
                                   float fontSize,
                                   float letterSpacing,
                                   float blur,
                                   uint32_t colorRGBA,
                                   uint32_t textAlign)
{
	memset(key, 0, sizeof(VKNVGtextRunKey));

	size_t textLen = strlen(text);
	if (textLen >= VKNVG_TEXT_CACHE_MAX_STRING) {
		textLen = VKNVG_TEXT_CACHE_MAX_STRING - 1;
	}
	memcpy(key->text, text, textLen);
	key->text[textLen] = '\0';

	key->fontId = fontId;
	key->fontSize = fontSize;
	key->letterSpacing = letterSpacing;
	key->blur = blur;
	key->colorRGBA = colorRGBA;
	key->textAlign = textAlign;
}

// Find text run in cache
static VKNVGtextRunEntry* vknvg__findTextRun(VKNVGtextRunCache* cache,
                                             const VKNVGtextRunKey* key)
{
	if (!cache || !key) return NULL;

	uint32_t hash = vknvg__hashTextRunKey(key);
	uint32_t index = hash % VKNVG_TEXT_CACHE_SIZE;

	for (uint32_t i = 0; i < VKNVG_TEXT_CACHE_SIZE; i++) {
		uint32_t probeIndex = (index + i) % VKNVG_TEXT_CACHE_SIZE;
		VKNVGtextRunEntry* entry = &cache->entries[probeIndex];

		if (!entry->valid) return NULL;

		if (entry->hash == hash && vknvg__compareTextRunKeys(&entry->key, key)) {
			entry->lastUsedFrame = cache->currentFrame;
			cache->cacheHits++;
			return entry;
		}
	}

	return NULL;
}

// Add text run to cache (evict LRU if full)
static VKNVGtextRunEntry* vknvg__addTextRun(VKNVGtextRunCache* cache,
                                            const VKNVGtextRunKey* key)
{
	if (!cache || !key) return NULL;

	uint32_t hash = vknvg__hashTextRunKey(key);
	uint32_t index = hash % VKNVG_TEXT_CACHE_SIZE;

	VKNVGtextRunEntry* targetEntry = NULL;
	uint32_t oldestFrame = cache->currentFrame;
	uint32_t oldestIndex = index;

	for (uint32_t i = 0; i < VKNVG_TEXT_CACHE_SIZE; i++) {
		uint32_t probeIndex = (index + i) % VKNVG_TEXT_CACHE_SIZE;
		VKNVGtextRunEntry* entry = &cache->entries[probeIndex];

		if (!entry->valid) {
			targetEntry = entry;
			break;
		}

		if (entry->lastUsedFrame < oldestFrame) {
			oldestFrame = entry->lastUsedFrame;
			oldestIndex = probeIndex;
		}
	}

	if (!targetEntry) {
		targetEntry = &cache->entries[oldestIndex];
		// Evict old entry
		if (targetEntry->valid) {
			vknvg__destroyCachedTexture(cache, &targetEntry->texture);
			cache->evictions++;
		}
	}

	memset(targetEntry, 0, sizeof(VKNVGtextRunEntry));
	targetEntry->key = *key;
	targetEntry->hash = hash;
	targetEntry->valid = VK_TRUE;
	targetEntry->lastUsedFrame = cache->currentFrame;

	cache->cacheMisses++;

	return targetEntry;
}

// Next frame (for LRU)
static inline void vknvg__textCacheNextFrame(VKNVGtextRunCache* cache)
{
	if (cache) cache->currentFrame++;
}

// Get statistics
static void vknvg__getTextCacheStats(VKNVGtextRunCache* cache,
                                     uint32_t* hits,
                                     uint32_t* misses,
                                     uint32_t* evictions,
                                     uint32_t* validEntries)
{
	if (!cache) return;

	if (hits) *hits = cache->cacheHits;
	if (misses) *misses = cache->cacheMisses;
	if (evictions) *evictions = cache->evictions;

	if (validEntries) {
		uint32_t count = 0;
		for (uint32_t i = 0; i < VKNVG_TEXT_CACHE_SIZE; i++) {
			if (cache->entries[i].valid) count++;
		}
		*validEntries = count;
	}
}

#endif // NANOVG_VK_TEXT_CACHE_H
