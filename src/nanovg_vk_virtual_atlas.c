// nanovg_vk_virtual_atlas.c - Implementation of Virtual Atlas System

#include "nanovg_vk_virtual_atlas.h"
#include "nanovg_vk_async_upload.h"
#include "nanovg_vk_compute.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void vknvg__defragUpdateCallback(void* userData, const VKNVGglyphMove* moves,
                                         uint32_t moveCount, uint32_t atlasIndex);

// Memory utility functions
static uint32_t vknvg__findMemoryType(VkPhysicalDevice physicalDevice,
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

// Rasterize glyph using GPU compute shader
// Note: This is a placeholder for compute rasterization integration
// Full implementation requires:
// 1. Access to FT_Outline from fontstash/FreeType
// 2. Outline conversion via vknvg__buildGlyphOutline()
// 3. Descriptor set allocation and updates
// 4. Compute dispatch and synchronization
// 5. Image readback to CPU memory
static uint8_t* vknvg__rasterizeGlyphCompute(VKNVGvirtualAtlas* atlas,
                                              VKNVGglyphKey key,
                                              uint16_t* width,
                                              uint16_t* height,
                                              int16_t* bearingX,
                                              int16_t* bearingY,
                                              uint16_t* advance)
{
	// For now, fall back to CPU rasterization
	// TODO: Implement full compute shader path:
	// 1. Get FT_Outline from fontstash
	// 2. Call vknvg__buildGlyphOutline()
	// 3. Upload outline to GPU
	// 4. Call vknvg__rasterizeGlyphCompute() from compute_raster.h
	// 5. Read back result

	if (atlas->rasterizeGlyph) {
		uint8_t bpp = 1;  // Will be filled by callback
		uint8_t* result = atlas->rasterizeGlyph(atlas->fontContext, key,
		                                         width, height, bearingX, bearingY, advance, &bpp);
		// Note: bytesPerPixel from compute fallback is ignored - uses default of 1
		return result;
	}

	return NULL;
}

// Background loader thread function
static void* vknvg__loaderThreadFunc(void* arg)
{
	VKNVGvirtualAtlas* atlas = (VKNVGvirtualAtlas*)arg;
	printf("[VA Loader] Background thread started\n");

	while (atlas->loaderThreadRunning) {
		pthread_mutex_lock(&atlas->loadQueueMutex);

		// Wait for work
		while (atlas->loadQueueCount == 0 && atlas->loaderThreadRunning) {
			printf("[VA Loader] Waiting for work...\n");
			pthread_cond_wait(&atlas->loadQueueCond, &atlas->loadQueueMutex);
			printf("[VA Loader] Woke up! Count=%u\n", atlas->loadQueueCount);
		}

		if (!atlas->loaderThreadRunning) {
			pthread_mutex_unlock(&atlas->loadQueueMutex);
			break;
		}

		// Get work item
		VKNVGglyphLoadRequest req = atlas->loadQueue[atlas->loadQueueHead];
		atlas->loadQueueHead = (atlas->loadQueueHead + 1) % VKNVG_LOAD_QUEUE_SIZE;
		atlas->loadQueueCount--;

		printf("[VA Loader] Processing glyph: font=%u codepoint=%u size=%u\n",
		       req.key.fontID, req.key.codepoint, req.key.size >> 16);

		pthread_mutex_unlock(&atlas->loadQueueMutex);

		// Extract outline or rasterize glyph
		uint16_t width, height, advance;
		int16_t bearingX, bearingY;
		uint8_t bytesPerPixel = 1;  // Default to grayscale
		req.outline = NULL;

		printf("[VA Loader] useComputeRaster=%d computeRaster=%p rasterizeGlyph=%p extractOutline=%p\n",
		       atlas->useComputeRaster, (void*)atlas->computeRaster,
		       (void*)atlas->rasterizeGlyph, (void*)atlas->extractOutline);

		// Try outline extraction first (for GPU MSDF)
		if (atlas->extractOutline) {
			req.outline = atlas->extractOutline(atlas->fontContext, req.key);
			if (req.outline) {
				printf("[VA Loader] Extracted outline: contours=%u unitsPerEM=%u\n",
				       req.outline->contourCount, req.outline->unitsPerEM);
				width = req.outline->width;
				height = req.outline->height;
				bearingX = req.outline->bearingX;
				bearingY = req.outline->bearingY;
				advance = req.outline->advance;
				bytesPerPixel = 3;  // MSDF is RGB
			}
		}

		// Fall back to rasterization if no outline
		if (!req.outline) {
			if (atlas->useComputeRaster && atlas->computeRaster) {
				// Use GPU compute shader rasterization
				req.pixelData = vknvg__rasterizeGlyphCompute(atlas, req.key,
				                                              &width, &height,
				                                              &bearingX, &bearingY, &advance);
			} else if (atlas->rasterizeGlyph) {
				// Use CPU rasterization callback
				req.pixelData = atlas->rasterizeGlyph(atlas->fontContext, req.key,
				                                       &width, &height,
				                                       &bearingX, &bearingY, &advance, &bytesPerPixel);
			} else {
				// Fallback: Create placeholder gradient for testing
				width = 32;
				height = 32;
				req.pixelData = (uint8_t*)malloc(width * height);
				if (req.pixelData) {
					for (int y = 0; y < height; y++) {
						for (int x = 0; x < width; x++) {
							req.pixelData[y * width + x] = (uint8_t)((x + y) * 2);
						}
					}
				}
				bearingX = 0;
				bearingY = height;
				advance = width;
			}
		}

		if (req.pixelData || req.outline) {
			// Update entry (thread-safe)
			pthread_mutex_lock(&atlas->cacheMutex);
			req.entry->width = width;
			req.entry->height = height;
			req.entry->bearingX = bearingX;
			req.entry->bearingY = bearingY;
			req.entry->advance = advance;
			req.entry->state = VKNVG_GLYPH_READY;
			pthread_mutex_unlock(&atlas->cacheMutex);

			// Add to upload queue
			pthread_mutex_lock(&atlas->uploadQueueMutex);
			if (atlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
				VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[atlas->uploadQueueCount++];
				upload->atlasIndex = req.entry->atlasIndex;	// Phase 3 Advanced
				upload->atlasX = req.entry->atlasX;
				upload->atlasY = req.entry->atlasY;
				upload->width = width;
				upload->height = height;
				upload->pixelData = req.pixelData;
				upload->outline = req.outline;
				upload->bytesPerPixel = bytesPerPixel;
				upload->useGPUMSDF = (req.outline != NULL) ? 1 : 0;
				upload->entry = req.entry;
			}
			pthread_mutex_unlock(&atlas->uploadQueueMutex);

			// Fire callback if set
			if (atlas->glyphReadyCallback) {
				atlas->glyphReadyCallback(atlas->callbackUserdata,
				                           req.key.fontID,
				                           req.key.codepoint,
				                           req.key.size);
			}
		}
	}

	return NULL;
}

// Create virtual atlas
VKNVGvirtualAtlas* vknvg__createVirtualAtlas(VkDevice device,
                                              VkPhysicalDevice physicalDevice,
                                              void* fontContext,
                                              VKNVGglyphRasterizeFunc rasterizeCallback)
{
	VKNVGvirtualAtlas* atlas = (VKNVGvirtualAtlas*)calloc(1, sizeof(VKNVGvirtualAtlas));
	if (!atlas) return NULL;

	atlas->device = device;
	atlas->physicalDevice = physicalDevice;
	atlas->fontContext = fontContext;
	atlas->rasterizeGlyph = rasterizeCallback;
	atlas->glyphCacheSize = VKNVG_GLYPH_CACHE_SIZE;
	atlas->currentFrame = 0;

	// Allocate glyph cache (hash table)
	atlas->glyphCache = (VKNVGglyphCacheEntry*)calloc(VKNVG_GLYPH_CACHE_SIZE,
	                                                   sizeof(VKNVGglyphCacheEntry));
	if (!atlas->glyphCache) {
		free(atlas);
		return NULL;
	}

	// Phase 3: Page system removed - no longer initialized

	// Create physical atlas texture
	// RGBA format - RGB channels for MSDF, A unused (RGB8 has alignment issues)
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent.width = VKNVG_ATLAS_PHYSICAL_SIZE;
	imageInfo.extent.height = VKNVG_ATLAS_PHYSICAL_SIZE;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(device, &imageInfo, NULL, &atlas->atlasImage) != VK_SUCCESS) {
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Allocate image memory
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, atlas->atlasImage, &memReqs);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryType(physicalDevice, memReqs.memoryTypeBits,
	                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device, &allocInfo, NULL, &atlas->atlasMemory) != VK_SUCCESS ||
	    vkBindImageMemory(device, atlas->atlasImage, atlas->atlasMemory, 0) != VK_SUCCESS) {
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Create image view
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = atlas->atlasImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, NULL, &atlas->atlasImageView) != VK_SUCCESS) {
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Create sampler with nearest filtering for crisp text
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(device, &samplerInfo, NULL, &atlas->atlasSampler) != VK_SUCCESS) {
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Phase 1: Create descriptor pool for multi-atlas support
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = VKNVG_MAX_ATLASES;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = VKNVG_MAX_ATLASES;

	if (vkCreateDescriptorPool(device, &poolInfo, NULL, &atlas->descriptorPool) != VK_SUCCESS) {
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Phase 1: Create descriptor set layout for atlas textures
	VkDescriptorSetLayoutBinding binding = {0};
	binding.binding = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &atlas->descriptorSetLayout) != VK_SUCCESS) {
		vkDestroyDescriptorPool(device, atlas->descriptorPool, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Phase 1: Initialize atlas manager (Guillotine packing + multi-atlas)
	// Phase 4 Advanced: Start with minimum size for dynamic growth (512x512 = 256KB vs 4096x4096 = 16MB)
	atlas->atlasManager = vknvg__createAtlasManager(device, physicalDevice,
	                                                 atlas->descriptorPool,
	                                                 atlas->descriptorSetLayout,
	                                                 atlas->atlasSampler,
	                                                 VKNVG_MIN_ATLAS_SIZE);
	if (!atlas->atlasManager) {
		vkDestroyDescriptorSetLayout(device, atlas->descriptorSetLayout, NULL);
		vkDestroyDescriptorPool(device, atlas->descriptorPool, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Phase 3: Initialize defragmentation context with callback
	memset(&atlas->defragContext, 0, sizeof(VKNVGdefragContext));
	atlas->defragContext.updateCallback = vknvg__defragUpdateCallback;
	atlas->defragContext.callbackUserData = atlas;
	atlas->enableDefrag = 1;	// Enabled for cache space reclamation

	// Compute context will be initialized later via separate function
	// that has access to VkQueue and queue family index
	atlas->computeContext = NULL;
	atlas->useComputeDefrag = VK_FALSE;

	// Create staging buffer for uploads
	atlas->stagingSize = VKNVG_ATLAS_PAGE_SIZE * VKNVG_ATLAS_PAGE_SIZE * VKNVG_UPLOAD_QUEUE_SIZE;

	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = atlas->stagingSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, NULL, &atlas->stagingBuffer) != VK_SUCCESS) {
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	vkGetBufferMemoryRequirements(device, atlas->stagingBuffer, &memReqs);

	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryType(physicalDevice, memReqs.memoryTypeBits,
	                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(device, &allocInfo, NULL, &atlas->stagingMemory) != VK_SUCCESS ||
	    vkBindBufferMemory(device, atlas->stagingBuffer, atlas->stagingMemory, 0) != VK_SUCCESS ||
	    vkMapMemory(device, atlas->stagingMemory, 0, atlas->stagingSize, 0, &atlas->stagingMapped) != VK_SUCCESS) {
		vkDestroyBuffer(device, atlas->stagingBuffer, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Initialize load queue
	atlas->loadQueue = (VKNVGglyphLoadRequest*)calloc(VKNVG_LOAD_QUEUE_SIZE,
	                                                    sizeof(VKNVGglyphLoadRequest));
	if (!atlas->loadQueue) {
		vkUnmapMemory(device, atlas->stagingMemory);
		vkFreeMemory(device, atlas->stagingMemory, NULL);
		vkDestroyBuffer(device, atlas->stagingBuffer, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Initialize mutexes and condition variable
	pthread_mutex_init(&atlas->cacheMutex, NULL);
	pthread_mutex_init(&atlas->loadQueueMutex, NULL);
	pthread_cond_init(&atlas->loadQueueCond, NULL);
	pthread_mutex_init(&atlas->uploadQueueMutex, NULL);

	// Start background loader thread
	atlas->loaderThreadRunning = true;
	if (pthread_create(&atlas->loaderThread, NULL, vknvg__loaderThreadFunc, atlas) != 0) {
		free(atlas->loadQueue);
		pthread_mutex_destroy(&atlas->uploadQueueMutex);
		pthread_cond_destroy(&atlas->loadQueueCond);
		pthread_mutex_destroy(&atlas->loadQueueMutex);
		pthread_mutex_destroy(&atlas->cacheMutex);
		vkUnmapMemory(device, atlas->stagingMemory);
		vkFreeMemory(device, atlas->stagingMemory, NULL);
		vkDestroyBuffer(device, atlas->stagingBuffer, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	printf("Virtual atlas created: %dx%d physical, Guillotine packing, %d cache entries\n",
	       VKNVG_ATLAS_PHYSICAL_SIZE, VKNVG_ATLAS_PHYSICAL_SIZE,
	       VKNVG_GLYPH_CACHE_SIZE);
	printf("Phase 3: Pure Guillotine allocation (page system removed)\n");

	// Initialize compute rasterization (disabled by default)
	atlas->computeRaster = NULL;
	atlas->useComputeRaster = VK_FALSE;
	atlas->computeQueue = VK_NULL_HANDLE;
	atlas->computeQueueFamily = 0;

	// Initialize async upload (disabled by default)
	atlas->asyncUpload = NULL;
	atlas->useAsyncUpload = VK_FALSE;
	atlas->transferQueue = VK_NULL_HANDLE;
	atlas->transferQueueFamily = 0;

	return atlas;
}

// Destroy virtual atlas
void vknvg__destroyVirtualAtlas(VKNVGvirtualAtlas* atlas)
{
	if (!atlas) return;

	// Stop loader thread
	atlas->loaderThreadRunning = false;
	pthread_cond_signal(&atlas->loadQueueCond);
	pthread_join(atlas->loaderThread, NULL);

	// Free any pending upload pixel data
	pthread_mutex_lock(&atlas->uploadQueueMutex);
	for (uint32_t i = 0; i < atlas->uploadQueueCount; i++) {
		free(atlas->uploadQueue[i].pixelData);
	}
	pthread_mutex_unlock(&atlas->uploadQueueMutex);

	// Destroy synchronization primitives
	pthread_mutex_destroy(&atlas->uploadQueueMutex);
	pthread_cond_destroy(&atlas->loadQueueCond);
	pthread_mutex_destroy(&atlas->loadQueueMutex);
	pthread_mutex_destroy(&atlas->cacheMutex);

	// Free load queue
	free(atlas->loadQueue);

	// Destroy Vulkan resources
	vkUnmapMemory(atlas->device, atlas->stagingMemory);
	vkFreeMemory(atlas->device, atlas->stagingMemory, NULL);
	vkDestroyBuffer(atlas->device, atlas->stagingBuffer, NULL);

	// Phase 1: Destroy atlas manager
	if (atlas->atlasManager) {
		vknvg__destroyAtlasManager(atlas->atlasManager);
		atlas->atlasManager = NULL;
	}

	// Destroy compute context
	if (atlas->computeContext) {
		vknvg__destroyComputeContext(atlas->computeContext);
		free(atlas->computeContext);
		atlas->computeContext = NULL;
	}

	// Phase 1: Destroy descriptor resources
	if (atlas->descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(atlas->device, atlas->descriptorSetLayout, NULL);
	}
	if (atlas->descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(atlas->device, atlas->descriptorPool, NULL);
	}

	vkDestroySampler(atlas->device, atlas->atlasSampler, NULL);
	vkDestroyImageView(atlas->device, atlas->atlasImageView, NULL);
	vkFreeMemory(atlas->device, atlas->atlasMemory, NULL);
	vkDestroyImage(atlas->device, atlas->atlasImage, NULL);

	// Destroy compute rasterization (if enabled)
	if (atlas->computeRaster) {
		// Note: vknvg__destroyComputeRaster() is in nanovg_vk_compute_raster.h
		// We would need to include it here to call the cleanup function
		// For now, the compute raster context is managed externally
		atlas->computeRaster = NULL;
	}

	// Destroy async upload (if enabled)
	if (atlas->asyncUpload) {
		vknvg__destroyAsyncUpload(atlas->asyncUpload);
		atlas->asyncUpload = NULL;
	}

	// Free cache
	free(atlas->glyphCache);
	free(atlas);

	printf("Virtual atlas destroyed\n");
}

// Lookup glyph in cache (thread-safe)
VKNVGglyphCacheEntry* vknvg__lookupGlyph(VKNVGvirtualAtlas* atlas, VKNVGglyphKey key)
{
	pthread_mutex_lock(&atlas->cacheMutex);

	uint32_t hash = vknvg__hashGlyphKey(key);
	uint32_t index = hash % atlas->glyphCacheSize;
	VKNVGglyphCacheEntry* result = NULL;

	// Linear probing
	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probe = (index + i) % atlas->glyphCacheSize;
		VKNVGglyphCacheEntry* entry = &atlas->glyphCache[probe];

		if (entry->state == VKNVG_GLYPH_EMPTY) {
			break;	// Not found
		}

		if (vknvg__glyphKeyEqual(entry->key, key)) {
			result = entry;
			break;
		}
	}

	pthread_mutex_unlock(&atlas->cacheMutex);
	return result;
}

// LRU list management
static void vknvg__lruRemove(VKNVGvirtualAtlas* atlas, VKNVGglyphCacheEntry* entry)
{
	if (entry->lruPrev) {
		entry->lruPrev->lruNext = entry->lruNext;
	} else {
		atlas->lruHead = entry->lruNext;
	}

	if (entry->lruNext) {
		entry->lruNext->lruPrev = entry->lruPrev;
	} else {
		atlas->lruTail = entry->lruPrev;
	}

	entry->lruPrev = NULL;
	entry->lruNext = NULL;
}

static void vknvg__lruMoveToHead(VKNVGvirtualAtlas* atlas, VKNVGglyphCacheEntry* entry)
{
	// Remove from current position if in list
	if (entry->lruPrev || entry->lruNext || atlas->lruHead == entry) {
		vknvg__lruRemove(atlas, entry);
	}

	// Add to head
	entry->lruNext = atlas->lruHead;
	entry->lruPrev = NULL;

	if (atlas->lruHead) {
		atlas->lruHead->lruPrev = entry;
	}
	atlas->lruHead = entry;

	if (!atlas->lruTail) {
		atlas->lruTail = entry;
	}
}

// Phase 3: LRU eviction (no page system)
// Note: With Guillotine packing, we can't easily reclaim space from evicted glyphs
// Defragmentation will handle this in the future
static void vknvg__evictLRU(VKNVGvirtualAtlas* atlas)
{
	if (!atlas->lruTail) {
		return;	// No entries to evict
	}

	VKNVGglyphCacheEntry* victim = atlas->lruTail;

	// Remove from LRU list
	vknvg__lruRemove(atlas, victim);

	// Mark entry as empty
	victim->state = VKNVG_GLYPH_EMPTY;
	victim->pageIndex = UINT16_MAX;

	atlas->evictions++;
	atlas->glyphCount--;

	// Phase 3: Space is not immediately reclaimed with Guillotine packing
	// Defragmentation (Phase 3 advanced) will compact the atlas to reclaim space
}

// Update glyph cache entries after defragmentation moves
// Called after defragmentation completes to update atlas positions
static void vknvg__updateGlyphCacheAfterDefrag(VKNVGvirtualAtlas* atlas,
                                                 const VKNVGglyphMove* moves,
                                                 uint32_t moveCount,
                                                 uint32_t atlasIndex)
{
	if (!atlas || !moves || moveCount == 0) {
		return;
	}

	// Iterate through all cache entries and update positions if they match a move
	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		VKNVGglyphCacheEntry* entry = &atlas->glyphCache[i];

		// Skip empty entries or entries in different atlas
		if (entry->state == VKNVG_GLYPH_EMPTY || entry->atlasIndex != atlasIndex) {
			continue;
		}

		// Check if this entry was moved
		for (uint32_t m = 0; m < moveCount; m++) {
			const VKNVGglyphMove* move = &moves[m];

			// Match by source position and dimensions
			if (entry->atlasX == move->srcX && entry->atlasY == move->srcY &&
			    entry->width == move->width && entry->height == move->height) {
				// Update to new position
				entry->atlasX = move->dstX;
				entry->atlasY = move->dstY;
				break;
			}
		}
	}
}

// Callback wrapper for defragmentation system
static void vknvg__defragUpdateCallback(void* userData, const VKNVGglyphMove* moves,
                                         uint32_t moveCount, uint32_t atlasIndex)
{
	VKNVGvirtualAtlas* atlas = (VKNVGvirtualAtlas*)userData;
	vknvg__updateGlyphCacheAfterDefrag(atlas, moves, moveCount, atlasIndex);
}

// Allocate a free page (legacy - Phase 2: kept as fallback)
static uint16_t vknvg__allocatePage(VKNVGvirtualAtlas* atlas)
{
	if (atlas->freePageCount == 0) {
		// Evict LRU entry to free a page
		vknvg__evictLRU(atlas);

		if (atlas->freePageCount == 0) {
			return UINT16_MAX;	// Still no free pages
		}
	}

	uint16_t pageIndex = atlas->freePageList[--atlas->freePageCount];
	atlas->pages[pageIndex].used = 1;
	atlas->pages[pageIndex].lastAccessFrame = atlas->currentFrame;
	return pageIndex;
}

// Phase 3: Pure Guillotine allocation (page system removed)
// Returns 1 on success, 0 on failure
// Fills atlasIndex, atlasX, atlasY
// Phase 4 Advanced: Helper function to resize atlas with temporary command buffer
static VkResult vknvg__resizeAtlasImmediate(VKNVGvirtualAtlas* atlas, uint32_t atlasIndex, uint16_t newSize)
{
	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = 0;	// Graphics queue

	VkCommandPool cmdPool;
	VkResult result = vkCreateCommandPool(atlas->device, &poolInfo, NULL, &cmdPool);
	if (result != VK_SUCCESS) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Allocate command buffer
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = cmdPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	result = vkAllocateCommandBuffers(atlas->device, &allocInfo, &cmdBuffer);
	if (result != VK_SUCCESS) {
		vkDestroyCommandPool(atlas->device, cmdPool, NULL);
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
	if (result != VK_SUCCESS) {
		vkFreeCommandBuffers(atlas->device, cmdPool, 1, &cmdBuffer);
		vkDestroyCommandPool(atlas->device, cmdPool, NULL);
		return result;
	}

	// Perform resize
	result = vknvg__resizeAtlasInstance(atlas->atlasManager, atlasIndex, newSize, cmdBuffer);

	vkEndCommandBuffer(cmdBuffer);

	if (result == VK_SUCCESS) {
		// Create fence for synchronization
		VkFence resizeFence;
		VkFenceCreateInfo fenceInfo = {0};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		result = vkCreateFence(atlas->device, &fenceInfo, NULL, &resizeFence);

		if (result == VK_SUCCESS) {
			// Submit with fence
			VkSubmitInfo submitInfo = {0};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			// Get graphics queue (assuming queue index 0)
			VkQueue graphicsQueue;
			vkGetDeviceQueue(atlas->device, 0, 0, &graphicsQueue);

			result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, resizeFence);
			if (result == VK_SUCCESS) {
				// Wait for fence instead of queue
				vkWaitForFences(atlas->device, 1, &resizeFence, VK_TRUE, UINT64_MAX);
			}

			vkDestroyFence(atlas->device, resizeFence, NULL);
		}
	}

	// Cleanup
	vkFreeCommandBuffers(atlas->device, cmdPool, 1, &cmdBuffer);
	vkDestroyCommandPool(atlas->device, cmdPool, NULL);

	return result;
}

static int vknvg__allocateSpace(VKNVGvirtualAtlas* atlas,
                                 uint16_t width, uint16_t height,
                                 uint32_t* outAtlasIndex,
                                 uint16_t* outAtlasX, uint16_t* outAtlasY)
{
	if (!atlas || !outAtlasIndex || !outAtlasX || !outAtlasY) {
		return 0;
	}

	// Phase 3: Use Guillotine packing exclusively
	if (!atlas->atlasManager) {
		return 0;	// No atlas manager
	}

	VKNVGrect rect;
	if (vknvg__multiAtlasAlloc(atlas->atlasManager, width, height,
	                            outAtlasIndex, &rect)) {
		*outAtlasX = rect.x;
		*outAtlasY = rect.y;
		return 1;
	}

	// Phase 4 Advanced: Dynamic growth - try resizing atlases if possible
	if (atlas->atlasManager->enableDynamicGrowth &&
	    atlas->atlasManager->atlasCount > 0) {
		// Try resizing any full atlas (prioritize current atlas first)
		for (uint32_t attempt = 0; attempt < atlas->atlasManager->atlasCount; attempt++) {
			uint32_t atlasIdx = (atlas->atlasManager->currentAtlas + attempt) % atlas->atlasManager->atlasCount;

			if (vknvg__shouldResizeAtlas(atlas->atlasManager, atlasIdx)) {
				VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlasIdx];
				uint16_t currentSize = atlasInst->packer.atlasWidth;
				uint16_t nextSize = vknvg__getNextAtlasSize(currentSize, atlas->atlasManager->maxAtlasSize);

				if (nextSize > 0) {
					// Resize atlas (512 -> 1024 -> 2048 -> 4096)
					if (vknvg__resizeAtlasImmediate(atlas, atlasIdx, nextSize) == VK_SUCCESS) {
						// Retry allocation after resize
						if (vknvg__multiAtlasAlloc(atlas->atlasManager, width, height,
						                            outAtlasIndex, &rect)) {
							*outAtlasX = rect.x;
							*outAtlasY = rect.y;
							return 1;
						}
					}
				}
			}
		}
	}

	return 0;	// Atlas full
}

// Request glyph (loads if not in cache)
VKNVGglyphCacheEntry* vknvg__requestGlyph(VKNVGvirtualAtlas* atlas, VKNVGglyphKey key)
{
	if (!atlas) return NULL;

	// Check cache first (vknvg__lookupGlyph handles its own locking)
	VKNVGglyphCacheEntry* entry = vknvg__lookupGlyph(atlas, key);
	if (entry && entry->state != VKNVG_GLYPH_EMPTY) {
		// Already exists in cache (LOADING, READY, or UPLOADED)
		// Update access tracking (thread-safe)
		pthread_mutex_lock(&atlas->cacheMutex);
		atlas->cacheHits++;
		entry->lastAccessFrame = atlas->currentFrame;
		vknvg__lruMoveToHead(atlas, entry);
		pthread_mutex_unlock(&atlas->cacheMutex);
		return entry;
	}

	atlas->cacheMisses++;

	// Find empty slot in cache (need lock for allocation)
	pthread_mutex_lock(&atlas->cacheMutex);

	uint32_t hash = vknvg__hashGlyphKey(key);
	uint32_t index = hash % atlas->glyphCacheSize;

	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probe = (index + i) % atlas->glyphCacheSize;
		entry = &atlas->glyphCache[probe];

		if (entry->state == VKNVG_GLYPH_EMPTY) {
			// Phase 3: Allocate space using pure Guillotine packing
			// Use fixed glyph size for now (will use actual size after rasterization in future)
			uint32_t atlasIndex;
			uint16_t atlasX, atlasY;
			if (!vknvg__allocateSpace(atlas, VKNVG_ATLAS_PAGE_SIZE, VKNVG_ATLAS_PAGE_SIZE,
			                           &atlasIndex, &atlasX, &atlasY)) {
				pthread_mutex_unlock(&atlas->cacheMutex);
				return NULL;	// Atlas full
			}

			// Initialize entry
			entry->key = key;
			entry->atlasIndex = atlasIndex;
			entry->atlasX = atlasX;
			entry->atlasY = atlasY;
			entry->pageIndex = UINT16_MAX;	// Phase 3: No longer used
			entry->state = VKNVG_GLYPH_LOADING;
			entry->loadFrame = atlas->currentFrame;
			entry->lastAccessFrame = atlas->currentFrame;
			atlas->glyphCount++;
			vknvg__lruMoveToHead(atlas, entry);

			pthread_mutex_unlock(&atlas->cacheMutex);

			// Add to load queue (separate lock)
			pthread_mutex_lock(&atlas->loadQueueMutex);
			if (atlas->loadQueueCount < VKNVG_LOAD_QUEUE_SIZE) {
				VKNVGglyphLoadRequest* req = &atlas->loadQueue[atlas->loadQueueTail];
				req->key = key;
				req->entry = entry;
				req->pixelData = NULL;
				req->timestamp = atlas->currentFrame;

				atlas->loadQueueTail = (atlas->loadQueueTail + 1) % VKNVG_LOAD_QUEUE_SIZE;
				atlas->loadQueueCount++;

				printf("[VA] Added glyph to load queue: font=%u codepoint=%u size=%u (queue=%u)\n",
				       key.fontID, key.codepoint, key.size >> 16, atlas->loadQueueCount);

				pthread_cond_signal(&atlas->loadQueueCond);
			}
			pthread_mutex_unlock(&atlas->loadQueueMutex);

			return entry;	// Return entry in LOADING state
		}
	}

	pthread_mutex_unlock(&atlas->cacheMutex);
	return NULL;	// Cache full
}

// Add glyph directly with pre-rasterized data (bypasses background loading)
VKNVGglyphCacheEntry* vknvg__addGlyphDirect(VKNVGvirtualAtlas* atlas,
                                             VKNVGglyphKey key,
                                             uint8_t* pixelData,
                                             uint16_t width, uint16_t height,
                                             int16_t bearingX, int16_t bearingY,
                                             uint16_t advance)
{
	if (!atlas || !pixelData) return NULL;

	// Check if already in cache
	VKNVGglyphCacheEntry* entry = vknvg__lookupGlyph(atlas, key);
	if (entry && entry->state != VKNVG_GLYPH_EMPTY) {
		// Already exists - don't add duplicate
		// Free the provided pixel data since we're not using it
		free(pixelData);
		atlas->cacheHits++;
		entry->lastAccessFrame = atlas->currentFrame;
		vknvg__lruMoveToHead(atlas, entry);
		return entry;
	}

	atlas->cacheMisses++;

	// Find empty slot in cache
	uint32_t hash = vknvg__hashGlyphKey(key);
	uint32_t index = hash % atlas->glyphCacheSize;

	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probe = (index + i) % atlas->glyphCacheSize;
		entry = &atlas->glyphCache[probe];

		if (entry->state == VKNVG_GLYPH_EMPTY) {
			// Phase 3: Allocate space using pure Guillotine packing
			uint32_t atlasIndex;
			uint16_t atlasX, atlasY;
			if (!vknvg__allocateSpace(atlas, width, height, &atlasIndex, &atlasX, &atlasY)) {
				free(pixelData);
				return NULL;	// Atlas full
			}

			// Initialize entry
			entry->key = key;
			entry->atlasIndex = atlasIndex;
			entry->atlasX = atlasX;
			entry->atlasY = atlasY;
			entry->width = width;
			entry->height = height;
			entry->bearingX = bearingX;
			entry->bearingY = bearingY;
			entry->advance = advance;
			entry->pageIndex = UINT16_MAX;	// Phase 3: No longer used
			entry->state = VKNVG_GLYPH_READY;	// Skip LOADING, go directly to READY
			entry->loadFrame = atlas->currentFrame;
			entry->lastAccessFrame = atlas->currentFrame;

			// Add directly to upload queue (bypass background loading)
			pthread_mutex_lock(&atlas->uploadQueueMutex);
			if (atlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
				VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[atlas->uploadQueueCount++];
				upload->atlasIndex = entry->atlasIndex;	// Phase 3 Advanced
				upload->atlasX = entry->atlasX;
				upload->atlasY = entry->atlasY;
				upload->width = width;
				upload->height = height;
				upload->pixelData = pixelData;	// Takes ownership
				upload->entry = entry;
			} else {
				// Upload queue full - free pixel data
				free(pixelData);
			}
			pthread_mutex_unlock(&atlas->uploadQueueMutex);

			atlas->glyphCount++;
			vknvg__lruMoveToHead(atlas, entry);
			return entry;
		}
	}

	// Cache full - free pixel data
	free(pixelData);
	return NULL;
}

// Process upload queue
// Create compute descriptor set for defragmentation
static VkDescriptorSet vknvg__createDefragDescriptorSet(VKNVGvirtualAtlas* atlas,
                                                         uint32_t atlasIndex)
{
	if (!atlas->computeContext || atlasIndex >= atlas->atlasManager->atlasCount) {
		return VK_NULL_HANDLE;
	}

	VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlasIndex];
	VKNVGcomputePipeline* pipeline = &atlas->computeContext->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG];

	// Allocate descriptor set
	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pipeline->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &pipeline->descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(atlas->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	// Update descriptor set with atlas image (same image for both src and dst)
	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.imageView = atlasInst->imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet writes[2] = {0};

	// Binding 0: Source atlas
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = descriptorSet;
	writes[0].dstBinding = 0;
	writes[0].dstArrayElement = 0;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &imageInfo;

	// Binding 1: Destination atlas (same image)
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = descriptorSet;
	writes[1].dstBinding = 1;
	writes[1].dstArrayElement = 0;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[1].descriptorCount = 1;
	writes[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(atlas->device, 2, writes, 0, NULL);

	return descriptorSet;
}

// Generate MSDF on GPU using compute shader
static VkResult vknvg__generateMSDFCompute(VKNVGvirtualAtlas* atlas,
                                            VKNVGglyphOutline* outline,
                                            VkImage targetImage,
                                            uint32_t atlasX, uint32_t atlasY,
                                            uint32_t width, uint32_t height)
{
	if (!atlas->computeContext || !outline) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Create buffer for outline data
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(VKNVGglyphOutline);
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer outlineBuffer;
	if (vkCreateBuffer(atlas->device, &bufferInfo, NULL, &outlineBuffer) != VK_SUCCESS) {
		fprintf(stderr, "[VA MSDF] Failed to create outline buffer\n");
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	// Allocate and bind memory
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(atlas->device, outlineBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;

	// Find HOST_VISIBLE memory type
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(atlas->physicalDevice, &memProps);
	uint32_t memTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReq.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memTypeIndex = i;
			break;
		}
	}
	if (memTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(atlas->device, outlineBuffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}
	allocInfo.memoryTypeIndex = memTypeIndex;

	VkDeviceMemory outlineMemory;
	if (vkAllocateMemory(atlas->device, &allocInfo, NULL, &outlineMemory) != VK_SUCCESS) {
		vkDestroyBuffer(atlas->device, outlineBuffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	vkBindBufferMemory(atlas->device, outlineBuffer, outlineMemory, 0);

	// Copy outline data to buffer
	void* mapped;
	vkMapMemory(atlas->device, outlineMemory, 0, sizeof(VKNVGglyphOutline), 0, &mapped);
	memcpy(mapped, outline, sizeof(VKNVGglyphOutline));
	vkUnmapMemory(atlas->device, outlineMemory);

	// Create descriptor set for MSDF pipeline
	VkDescriptorSetAllocateInfo descAllocInfo = {0};
	descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descAllocInfo.descriptorPool = atlas->computeContext->pipelines[VKNVG_COMPUTE_MSDF_GENERATE].descriptorPool;
	descAllocInfo.descriptorSetCount = 1;
	descAllocInfo.pSetLayouts = &atlas->computeContext->pipelines[VKNVG_COMPUTE_MSDF_GENERATE].descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(atlas->device, &descAllocInfo, &descriptorSet) != VK_SUCCESS) {
		vkFreeMemory(atlas->device, outlineMemory, NULL);
		vkDestroyBuffer(atlas->device, outlineBuffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	// Create image view for target region (entire image for now)
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = targetImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(atlas->device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
		vkFreeMemory(atlas->device, outlineMemory, NULL);
		vkDestroyBuffer(atlas->device, outlineBuffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	// Update descriptor set
	VkDescriptorBufferInfo bufferDescInfo = {0};
	bufferDescInfo.buffer = outlineBuffer;
	bufferDescInfo.offset = 0;
	bufferDescInfo.range = sizeof(VKNVGglyphOutline);

	VkDescriptorImageInfo imageDescInfo = {0};
	imageDescInfo.imageView = imageView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet writes[2] = {0};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = descriptorSet;
	writes[0].dstBinding = 0;
	writes[0].dstArrayElement = 0;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[0].descriptorCount = 1;
	writes[0].pBufferInfo = &bufferDescInfo;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = descriptorSet;
	writes[1].dstBinding = 1;
	writes[1].dstArrayElement = 0;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[1].descriptorCount = 1;
	writes[1].pImageInfo = &imageDescInfo;

	vkUpdateDescriptorSets(atlas->device, 2, writes, 0, NULL);

	// Setup push constants
	VKNVGmsdfPushConstants pushConstants;
	pushConstants.outputWidth = width;
	pushConstants.outputHeight = height;
	pushConstants.pxRange = 4.0f;  // Typical MSDF range
	pushConstants.padding = 0.0f;

	// Dispatch compute shader
	vknvg__dispatchMSDFCompute(atlas->computeContext, descriptorSet, &pushConstants, width, height);

	// Cleanup
	vkDestroyImageView(atlas->device, imageView, NULL);
	vkFreeMemory(atlas->device, outlineMemory, NULL);
	vkDestroyBuffer(atlas->device, outlineBuffer, NULL);

	printf("[VA MSDF] Generated %ux%u MSDF at (%u,%u)\n", width, height, atlasX, atlasY);
	return VK_SUCCESS;
}

void vknvg__processUploads(VKNVGvirtualAtlas* atlas, VkCommandBuffer cmd)
{
	if (!atlas || !cmd) return;

	pthread_mutex_lock(&atlas->uploadQueueMutex);

	if (atlas->uploadQueueCount == 0) {
		pthread_mutex_unlock(&atlas->uploadQueueMutex);
		return;
	}

	// Route to async upload path if enabled
	if (atlas->useAsyncUpload && atlas->asyncUpload) {
		// Phase 3 Advanced: Queue uploads to correct atlas images
		for (uint32_t i = 0; i < atlas->uploadQueueCount; i++) {
			VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[i];
			size_t dataSize = upload->width * upload->height;

			// Get target atlas image
			VKNVGatlasInstance* targetAtlas = &atlas->atlasManager->atlases[upload->atlasIndex];

			VkResult result = vknvg__queueAsyncUpload(atlas->asyncUpload,
			                                           targetAtlas->image,
			                                           upload->pixelData,
			                                           dataSize,
			                                           upload->atlasX, upload->atlasY,
			                                           upload->width, upload->height);

			if (result == VK_SUCCESS) {
				// Update entry state
				upload->entry->state = VKNVG_GLYPH_UPLOADED;
				atlas->uploads++;
			}

			// Free pixel data (async upload made a copy to staging buffer)
			free(upload->pixelData);
		}

		// Submit all queued uploads
		vknvg__submitAsyncUploads(atlas->asyncUpload);

		atlas->uploadQueueCount = 0;
		atlas->imageInitialized = true;
		pthread_mutex_unlock(&atlas->uploadQueueMutex);
		return;
	}

	// Phase 3 Advanced: Multi-atlas upload path
	// Group uploads by atlas index and upload to each atlas

	// Track which atlases need barriers
	uint8_t atlasUsed[VKNVG_MAX_ATLASES] = {0};
	for (uint32_t i = 0; i < atlas->uploadQueueCount; i++) {
		uint32_t idx = atlas->uploadQueue[i].atlasIndex;
		if (idx < VKNVG_MAX_ATLASES) {
			atlasUsed[idx] = 1;
		}
	}

	// Transition all used atlases to transfer dst layout
	for (uint32_t atlasIdx = 0; atlasIdx < atlas->atlasManager->atlasCount; atlasIdx++) {
		if (!atlasUsed[atlasIdx]) continue;

		VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlasIdx];

		VkImageMemoryBarrier barrier = {0};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = atlas->imageInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		                                             : VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = atlasInst->image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = atlas->imageInitialized ? VK_ACCESS_SHADER_READ_BIT : 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd,
		                     atlas->imageInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		                                              : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);
	}

	// Upload all pending glyphs
	VkDeviceSize stagingOffset = 0;
	for (uint32_t i = 0; i < atlas->uploadQueueCount; i++) {
		VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[i];

		// Phase 3 GPU MSDF: Check if this is an outline-based upload
		if (upload->useGPUMSDF && upload->outline && atlas->computeContext) {
			// GPU MSDF generation path
			printf("[VA Upload] GPU MSDF for glyph %ux%u\n", upload->width, upload->height);

			// Get target atlas
			VKNVGatlasInstance* targetAtlas = &atlas->atlasManager->atlases[upload->atlasIndex];

			// Generate MSDF using compute shader
			VkResult result = vknvg__generateMSDFCompute(atlas, upload->outline,
			                                              targetAtlas->image,
			                                              upload->atlasX, upload->atlasY,
			                                              upload->width, upload->height);

			if (result == VK_SUCCESS) {
				upload->entry->state = VKNVG_GLYPH_UPLOADED;
				atlas->uploads++;
			} else {
				fprintf(stderr, "[VA Upload] MSDF generation failed\n");
			}

			free(upload->outline);
			continue;
		}

		// Fallback: CPU rasterized pixel upload path
		if (!upload->pixelData) {
			printf("[VA Upload] WARNING: No pixel data and no outline for glyph\n");
			upload->entry->state = VKNVG_GLYPH_UPLOADED;
			continue;
		}

		// Copy pixel data to staging buffer, converting to RGBA format
		// Atlas is RGBA, but source can be GRAY (1), RGB (3), or RGBA (4)
		size_t pixelCount = upload->width * upload->height;
		size_t dataSize = pixelCount * 4;  // Atlas is always RGBA
		if (stagingOffset + dataSize > atlas->stagingSize) {
			break;	// Staging buffer full
		}

		// Convert source format to RGBA
		uint8_t* dst = (uint8_t*)atlas->stagingMapped + stagingOffset;
		uint8_t* src = upload->pixelData;

		if (upload->bytesPerPixel == 1) {
			// GRAY → RGBA: replicate gray to RGB, alpha=255
			for (size_t p = 0; p < pixelCount; p++) {
				dst[p*4 + 0] = src[p];
				dst[p*4 + 1] = src[p];
				dst[p*4 + 2] = src[p];
				dst[p*4 + 3] = 255;
			}
		} else if (upload->bytesPerPixel == 3) {
			// RGB → RGBA: copy RGB, alpha=255
			for (size_t p = 0; p < pixelCount; p++) {
				dst[p*4 + 0] = src[p*3 + 0];
				dst[p*4 + 1] = src[p*3 + 1];
				dst[p*4 + 2] = src[p*3 + 2];
				dst[p*4 + 3] = 255;
			}
		} else {
			// RGBA → RGBA: direct copy
			memcpy(dst, src, dataSize);
		}

		// Copy from staging to atlas
		VkBufferImageCopy region = {0};
		region.bufferOffset = stagingOffset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = upload->atlasX;
		region.imageOffset.y = upload->atlasY;
		region.imageOffset.z = 0;
		region.imageExtent.width = upload->width;
		region.imageExtent.height = upload->height;
		region.imageExtent.depth = 1;

		// Phase 3 Advanced: Upload to correct atlas based on atlasIndex
		VKNVGatlasInstance* targetAtlas = &atlas->atlasManager->atlases[upload->atlasIndex];

		vkCmdCopyBufferToImage(cmd, atlas->stagingBuffer, targetAtlas->image,
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// Update entry state
		upload->entry->state = VKNVG_GLYPH_UPLOADED;

		// Free pixel data
		free(upload->pixelData);

		stagingOffset += dataSize;
		atlas->uploads++;
	}

	// Phase 3 Advanced: Transition all used atlases back to shader read layout
	for (uint32_t atlasIdx = 0; atlasIdx < atlas->atlasManager->atlasCount; atlasIdx++) {
		if (!atlasUsed[atlasIdx]) continue;

		VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlasIdx];

		VkImageMemoryBarrier barrier = {0};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = atlasInst->image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT,
		                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);
	}

	// Mark image as initialized (now in SHADER_READ_ONLY layout)
	atlas->imageInitialized = true;

	atlas->uploadQueueCount = 0;
	pthread_mutex_unlock(&atlas->uploadQueueMutex);

	// Phase 4 Advanced: Continue defragmentation if in progress
	if (atlas->enableDefrag && atlas->defragContext.state != VKNVG_DEFRAG_IDLE) {
		// Enable compute defragmentation if compute context is available
		if (atlas->computeContext && atlas->useComputeDefrag &&
		    atlas->defragContext.atlasIndex < atlas->atlasManager->atlasCount) {

			VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlas->defragContext.atlasIndex];

			// Transition atlas image to GENERAL layout for compute shader access
			VkImageMemoryBarrier barrier = {0};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = atlasInst->image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd,
			                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                     0, 0, NULL, 0, NULL, 1, &barrier);

			// Create descriptor set for this atlas
			VkDescriptorSet defragDescriptorSet = vknvg__createDefragDescriptorSet(atlas, atlas->defragContext.atlasIndex);

			if (defragDescriptorSet != VK_NULL_HANDLE) {
				// Enable compute defragmentation
				vknvg__enableComputeDefrag(&atlas->defragContext, atlas->computeContext, defragDescriptorSet);
				printf("[ATLAS] Compute defragmentation enabled for atlas %u\n", atlas->defragContext.atlasIndex);
				printf("[ATLAS] Descriptor set: %p\n", (void*)defragDescriptorSet);
			} else {
				printf("[ATLAS] WARNING: Failed to create descriptor set for compute defrag\n");
			}
		}

		// Execute incremental defragmentation with 2ms time budget
		// Glyph cache integration is now complete via callback
		printf("[ATLAS] Executing defragmentation (state=%d, compute=%d)\n",
		       atlas->defragContext.state, atlas->useComputeDefrag);
		vknvg__updateDefragmentation(&atlas->defragContext, atlas->atlasManager, cmd, 2.0f);

		// Transition atlas back to SHADER_READ_ONLY_OPTIMAL after defrag
		if (atlas->computeContext && atlas->useComputeDefrag &&
		    atlas->defragContext.atlasIndex < atlas->atlasManager->atlasCount) {

			VKNVGatlasInstance* atlasInst = &atlas->atlasManager->atlases[atlas->defragContext.atlasIndex];

			VkImageMemoryBarrier barrier = {0};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = atlasInst->image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
			                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			                     0, 0, NULL, 0, NULL, 1, &barrier);
		}
	}
}

// Advance frame counter
void vknvg__atlasNextFrame(VKNVGvirtualAtlas* atlas)
{
	if (!atlas) return;
	atlas->currentFrame++;

	// Phase 4 Advanced: Incremental defragmentation during idle frames
	if (atlas->enableDefrag && atlas->atlasManager) {
		// Check if currently defragmenting
		if (atlas->defragContext.state == VKNVG_DEFRAG_IDLE) {
			// Look for an atlas that needs defragmentation
			for (uint32_t i = 0; i < atlas->atlasManager->atlasCount; i++) {
				if (vknvg__shouldDefragmentAtlas(&atlas->atlasManager->atlases[i])) {
					// Start defragmentation for this atlas
					vknvg__initDefragContext(&atlas->defragContext, i, 2.0f);	// 2ms time budget
					vknvg__startDefragmentation(&atlas->defragContext, atlas->atlasManager);
					break;
				}
			}
		} else if (atlas->defragContext.state != VKNVG_DEFRAG_IDLE) {
			// Continue ongoing defragmentation
			// Note: This requires a command buffer, which we don't have in atlasNextFrame()
			// In a real implementation, this would be called from processUploads() or a render function
			// For now, we just check the state
			// vknvg__updateDefragmentation(&atlas->defragContext, atlas->atlasManager, NULL, 2.0f);
		}
	}
}

// Get statistics
void vknvg__getAtlasStats(VKNVGvirtualAtlas* atlas,
                          uint32_t* cacheHits, uint32_t* cacheMisses,
                          uint32_t* evictions, uint32_t* uploads)
{
	if (!atlas) return;
	if (cacheHits) *cacheHits = atlas->cacheHits;
	if (cacheMisses) *cacheMisses = atlas->cacheMisses;
	if (evictions) *evictions = atlas->evictions;
	if (uploads) *uploads = atlas->uploads;
}

// Set font context and rasterization callback
void vknvg__setAtlasFontContext(VKNVGvirtualAtlas* atlas,
                                 void* fontContext,
                                 VKNVGglyphRasterizeFunc rasterizeCallback,
                                 VKNVGglyphExtractOutlineFunc extractOutlineCallback)
{
	if (atlas) {
		atlas->fontContext = fontContext;
		atlas->rasterizeGlyph = rasterizeCallback;
		atlas->extractOutline = extractOutlineCallback;
	}
}

// Set glyph ready callback
void vknvg__setGlyphReadyCallback(VKNVGvirtualAtlas* atlas,
                                   VKNVGglyphReadyCallback callback,
                                   void* userdata)
{
	if (atlas) {
		atlas->glyphReadyCallback = callback;
		atlas->callbackUserdata = userdata;
	}
}

// Enable async uploads
VkResult vknvg__enableAsyncUploads(VKNVGvirtualAtlas* atlas,
                                    VkQueue transferQueue,
                                    uint32_t transferQueueFamily)
{
	if (!atlas) return VK_ERROR_INITIALIZATION_FAILED;

	// Already enabled
	if (atlas->asyncUpload) return VK_SUCCESS;

	// Create async upload context
	// Use 1MB staging buffer per frame
	VkDeviceSize stagingBufferSize = 1024 * 1024;

	atlas->asyncUpload = vknvg__createAsyncUpload(atlas->device,
	                                               atlas->physicalDevice,
	                                               transferQueue,
	                                               transferQueueFamily,
	                                               stagingBufferSize);

	if (!atlas->asyncUpload) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	atlas->useAsyncUpload = VK_TRUE;
	atlas->transferQueue = transferQueue;
	atlas->transferQueueFamily = transferQueueFamily;

	printf("Async uploads enabled: 3 frames, 1MB staging buffer per frame\n");

	return VK_SUCCESS;
}

// Get upload semaphore for graphics queue synchronization
VkSemaphore vknvg__getUploadSemaphore(VKNVGvirtualAtlas* atlas)
{
	if (!atlas || !atlas->asyncUpload) {
		return VK_NULL_HANDLE;
	}

	return vknvg__getUploadCompleteSemaphore(atlas->asyncUpload);
}

// Enable compute shader defragmentation
VkResult vknvg__enableComputeDefragmentation(VKNVGvirtualAtlas* atlas,
                                              VkQueue computeQueue,
                                              uint32_t computeQueueFamily)
{
	if (!atlas) return VK_ERROR_INITIALIZATION_FAILED;

	// Already enabled
	if (atlas->computeContext) return VK_SUCCESS;

	// Allocate compute context
	atlas->computeContext = (VKNVGcomputeContext*)malloc(sizeof(VKNVGcomputeContext));
	if (!atlas->computeContext) {
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Initialize compute context
	if (!vknvg__initComputeContext(atlas->computeContext,
	                                atlas->device,
	                                computeQueue,
	                                computeQueueFamily)) {
		free(atlas->computeContext);
		atlas->computeContext = NULL;
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	atlas->useComputeDefrag = VK_TRUE;
	printf("Compute shader defragmentation enabled\n");

	return VK_SUCCESS;
}

// Enable GPU MSDF generation
VkResult vknvg__enableGPUMSDF(VKNVGvirtualAtlas* atlas,
                               VkQueue computeQueue,
                               uint32_t computeQueueFamily)
{
	if (!atlas) return VK_ERROR_INITIALIZATION_FAILED;

	// If compute context already exists (e.g., from defrag), reuse it
	if (atlas->computeContext) {
		printf("GPU MSDF generation enabled (reusing existing compute context)\n");
		return VK_SUCCESS;
	}

	// Allocate compute context
	atlas->computeContext = (VKNVGcomputeContext*)malloc(sizeof(VKNVGcomputeContext));
	if (!atlas->computeContext) {
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	// Initialize compute context
	if (!vknvg__initComputeContext(atlas->computeContext,
	                                atlas->device,
	                                computeQueue,
	                                computeQueueFamily)) {
		free(atlas->computeContext);
		atlas->computeContext = NULL;
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	printf("GPU MSDF generation enabled (compute context created)\n");

	return VK_SUCCESS;
}
