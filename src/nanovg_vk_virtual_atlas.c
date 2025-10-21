// nanovg_vk_virtual_atlas.c - Implementation of Virtual Atlas System

#include "nanovg_vk_virtual_atlas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
		return atlas->rasterizeGlyph(atlas->fontContext, key,
		                             width, height, bearingX, bearingY, advance);
	}

	return NULL;
}

// Background loader thread function
static void* vknvg__loaderThreadFunc(void* arg)
{
	VKNVGvirtualAtlas* atlas = (VKNVGvirtualAtlas*)arg;

	while (atlas->loaderThreadRunning) {
		pthread_mutex_lock(&atlas->loadQueueMutex);

		// Wait for work
		while (atlas->loadQueueCount == 0 && atlas->loaderThreadRunning) {
			pthread_cond_wait(&atlas->loadQueueCond, &atlas->loadQueueMutex);
		}

		if (!atlas->loaderThreadRunning) {
			pthread_mutex_unlock(&atlas->loadQueueMutex);
			break;
		}

		// Get work item
		VKNVGglyphLoadRequest req = atlas->loadQueue[atlas->loadQueueHead];
		atlas->loadQueueHead = (atlas->loadQueueHead + 1) % VKNVG_LOAD_QUEUE_SIZE;
		atlas->loadQueueCount--;

		pthread_mutex_unlock(&atlas->loadQueueMutex);

		// Rasterize glyph
		uint16_t width, height, advance;
		int16_t bearingX, bearingY;

		if (atlas->useComputeRaster && atlas->computeRaster) {
			// Use GPU compute shader rasterization
			req.pixelData = vknvg__rasterizeGlyphCompute(atlas, req.key,
			                                              &width, &height,
			                                              &bearingX, &bearingY, &advance);
		} else if (atlas->rasterizeGlyph) {
			// Use CPU rasterization callback
			req.pixelData = atlas->rasterizeGlyph(atlas->fontContext, req.key,
			                                       &width, &height,
			                                       &bearingX, &bearingY, &advance);
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

		if (req.pixelData) {
			// Update entry
			req.entry->width = width;
			req.entry->height = height;
			req.entry->bearingX = bearingX;
			req.entry->bearingY = bearingY;
			req.entry->advance = advance;
			req.entry->state = VKNVG_GLYPH_READY;

			// Add to upload queue
			pthread_mutex_lock(&atlas->uploadQueueMutex);
			if (atlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
				VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[atlas->uploadQueueCount++];
				upload->atlasX = req.entry->atlasX;
				upload->atlasY = req.entry->atlasY;
				upload->width = width;
				upload->height = height;
				upload->pixelData = req.pixelData;
				upload->entry = req.entry;
			}
			pthread_mutex_unlock(&atlas->uploadQueueMutex);
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

	// Initialize free page list
	atlas->freePageList = (uint16_t*)malloc(VKNVG_ATLAS_MAX_PAGES * sizeof(uint16_t));
	if (!atlas->freePageList) {
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	atlas->freePageCount = VKNVG_ATLAS_MAX_PAGES;
	for (uint16_t i = 0; i < VKNVG_ATLAS_MAX_PAGES; i++) {
		atlas->freePageList[i] = i;
		atlas->pages[i].x = (i % (VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE));
		atlas->pages[i].y = (i / (VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE));
		atlas->pages[i].used = 0;
		atlas->pages[i].flags = 0;
	}

	// Create physical atlas texture
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8_UNORM;
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
		free(atlas->freePageList);
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
		free(atlas->freePageList);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Create image view
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = atlas->atlasImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, NULL, &atlas->atlasImageView) != VK_SUCCESS) {
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->freePageList);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Create sampler
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
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
		free(atlas->freePageList);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

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
		free(atlas->freePageList);
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
		free(atlas->freePageList);
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
		free(atlas->freePageList);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	// Initialize mutexes and condition variable
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
		vkUnmapMemory(device, atlas->stagingMemory);
		vkFreeMemory(device, atlas->stagingMemory, NULL);
		vkDestroyBuffer(device, atlas->stagingBuffer, NULL);
		vkDestroySampler(device, atlas->atlasSampler, NULL);
		vkDestroyImageView(device, atlas->atlasImageView, NULL);
		vkFreeMemory(device, atlas->atlasMemory, NULL);
		vkDestroyImage(device, atlas->atlasImage, NULL);
		free(atlas->freePageList);
		free(atlas->glyphCache);
		free(atlas);
		return NULL;
	}

	printf("Virtual atlas created: %dx%d physical, %d pages, %d cache entries\n",
	       VKNVG_ATLAS_PHYSICAL_SIZE, VKNVG_ATLAS_PHYSICAL_SIZE,
	       VKNVG_ATLAS_MAX_PAGES, VKNVG_GLYPH_CACHE_SIZE);

	// Initialize compute rasterization (disabled by default)
	atlas->computeRaster = NULL;
	atlas->useComputeRaster = VK_FALSE;
	atlas->computeQueue = VK_NULL_HANDLE;
	atlas->computeQueueFamily = 0;

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

	// Free load queue
	free(atlas->loadQueue);

	// Destroy Vulkan resources
	vkUnmapMemory(atlas->device, atlas->stagingMemory);
	vkFreeMemory(atlas->device, atlas->stagingMemory, NULL);
	vkDestroyBuffer(atlas->device, atlas->stagingBuffer, NULL);
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

	// Free cache
	free(atlas->freePageList);
	free(atlas->glyphCache);
	free(atlas);

	printf("Virtual atlas destroyed\n");
}

// Lookup glyph in cache
VKNVGglyphCacheEntry* vknvg__lookupGlyph(VKNVGvirtualAtlas* atlas, VKNVGglyphKey key)
{
	uint32_t hash = vknvg__hashGlyphKey(key);
	uint32_t index = hash % atlas->glyphCacheSize;

	// Linear probing
	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probe = (index + i) % atlas->glyphCacheSize;
		VKNVGglyphCacheEntry* entry = &atlas->glyphCache[probe];

		if (entry->state == VKNVG_GLYPH_EMPTY) {
			return NULL;	// Not found
		}

		if (vknvg__glyphKeyEqual(entry->key, key)) {
			return entry;
		}
	}

	return NULL;	// Cache full or not found
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

static void vknvg__evictLRU(VKNVGvirtualAtlas* atlas)
{
	if (!atlas->lruTail) {
		return;	// No entries to evict
	}

	VKNVGglyphCacheEntry* victim = atlas->lruTail;

	// Free the page
	if (victim->pageIndex != UINT16_MAX) {
		atlas->freePageList[atlas->freePageCount++] = victim->pageIndex;
		atlas->pages[victim->pageIndex].used = 0;
	}

	// Remove from LRU list
	vknvg__lruRemove(atlas, victim);

	// Mark entry as empty
	victim->state = VKNVG_GLYPH_EMPTY;
	victim->pageIndex = UINT16_MAX;

	atlas->evictions++;
	atlas->glyphCount--;
}

// Allocate a free page
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

// Request glyph (loads if not in cache)
VKNVGglyphCacheEntry* vknvg__requestGlyph(VKNVGvirtualAtlas* atlas, VKNVGglyphKey key)
{
	if (!atlas) return NULL;

	// Check cache first
	VKNVGglyphCacheEntry* entry = vknvg__lookupGlyph(atlas, key);
	if (entry && entry->state != VKNVG_GLYPH_EMPTY) {
		// Already exists in cache (LOADING, READY, or UPLOADED)
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
			// Allocate page
			uint16_t pageIndex = vknvg__allocatePage(atlas);
			if (pageIndex == UINT16_MAX) {
				return NULL;	// Atlas full
			}

			// Initialize entry
			entry->key = key;
			entry->atlasX = atlas->pages[pageIndex].x * VKNVG_ATLAS_PAGE_SIZE;
			entry->atlasY = atlas->pages[pageIndex].y * VKNVG_ATLAS_PAGE_SIZE;
			entry->pageIndex = pageIndex;
			entry->state = VKNVG_GLYPH_LOADING;
			entry->loadFrame = atlas->currentFrame;
			entry->lastAccessFrame = atlas->currentFrame;

			// Add to load queue
			pthread_mutex_lock(&atlas->loadQueueMutex);
			if (atlas->loadQueueCount < VKNVG_LOAD_QUEUE_SIZE) {
				VKNVGglyphLoadRequest* req = &atlas->loadQueue[atlas->loadQueueTail];
				req->key = key;
				req->entry = entry;
				req->pixelData = NULL;
				req->timestamp = atlas->currentFrame;

				atlas->loadQueueTail = (atlas->loadQueueTail + 1) % VKNVG_LOAD_QUEUE_SIZE;
				atlas->loadQueueCount++;

				pthread_cond_signal(&atlas->loadQueueCond);
			}
			pthread_mutex_unlock(&atlas->loadQueueMutex);

			atlas->glyphCount++;
			vknvg__lruMoveToHead(atlas, entry);
			return entry;	// Return entry in LOADING state
		}
	}

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
			// Allocate page
			uint16_t pageIndex = vknvg__allocatePage(atlas);
			if (pageIndex == UINT16_MAX) {
				free(pixelData);
				return NULL;	// Atlas full
			}

			// Initialize entry
			entry->key = key;
			entry->atlasX = atlas->pages[pageIndex].x * VKNVG_ATLAS_PAGE_SIZE;
			entry->atlasY = atlas->pages[pageIndex].y * VKNVG_ATLAS_PAGE_SIZE;
			entry->width = width;
			entry->height = height;
			entry->bearingX = bearingX;
			entry->bearingY = bearingY;
			entry->advance = advance;
			entry->pageIndex = pageIndex;
			entry->state = VKNVG_GLYPH_READY;	// Skip LOADING, go directly to READY
			entry->loadFrame = atlas->currentFrame;
			entry->lastAccessFrame = atlas->currentFrame;

			// Add directly to upload queue (bypass background loading)
			pthread_mutex_lock(&atlas->uploadQueueMutex);
			if (atlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
				VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[atlas->uploadQueueCount++];
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
void vknvg__processUploads(VKNVGvirtualAtlas* atlas, VkCommandBuffer cmd)
{
	if (!atlas || !cmd) return;

	pthread_mutex_lock(&atlas->uploadQueueMutex);

	printf("DEBUG: processUploads called with %u items in queue\n", atlas->uploadQueueCount);

	if (atlas->uploadQueueCount == 0) {
		pthread_mutex_unlock(&atlas->uploadQueueMutex);
		return;
	}

	// Transition atlas to transfer dst layout
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	// On first upload, transition from UNDEFINED; otherwise from SHADER_READ_ONLY
	barrier.oldLayout = atlas->imageInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	                                             : VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = atlas->atlasImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	// When transitioning from UNDEFINED, srcAccessMask must be 0
	barrier.srcAccessMask = atlas->imageInitialized ? VK_ACCESS_SHADER_READ_BIT : 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd,
	                     atlas->imageInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	                                              : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Upload all pending glyphs
	VkDeviceSize stagingOffset = 0;
	for (uint32_t i = 0; i < atlas->uploadQueueCount; i++) {
		VKNVGglyphUploadRequest* upload = &atlas->uploadQueue[i];

		// Copy pixel data to staging buffer
		size_t dataSize = upload->width * upload->height;
		printf("DEBUG: Uploading glyph %u: pos(%u,%u) size(%ux%u) dataSize=%zu ptr=%p\n",
		       i, upload->atlasX, upload->atlasY, upload->width, upload->height, dataSize, (void*)upload->pixelData);
		if (stagingOffset + dataSize > atlas->stagingSize) {
			break;	// Staging buffer full
		}

		// Check pixelData before copy
		int nonZeroBefore = 0;
		for (size_t j = 0; j < dataSize && j < 100; j++) {
			if (upload->pixelData[j] != 0) nonZeroBefore++;
		}

		memcpy((uint8_t*)atlas->stagingMapped + stagingOffset, upload->pixelData, dataSize);

		// Check staging buffer after copy
		int nonZeroAfter = 0;
		for (size_t j = 0; j < 100 && j < dataSize; j++) {
			if (((uint8_t*)atlas->stagingMapped)[stagingOffset + j] != 0) nonZeroAfter++;
		}
		printf("DEBUG: Upload %u: before=%d after=%d (in staging)\n", i, nonZeroBefore, nonZeroAfter);

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

		if (i < 2) printf("DEBUG: vkCmdCopyBufferToImage region: bufOffset=%llu atlasPos=(%d,%d) size=(%dx%d)\n",
		                  (unsigned long long)region.bufferOffset, region.imageOffset.x, region.imageOffset.y,
		                  region.imageExtent.width, region.imageExtent.height);
		vkCmdCopyBufferToImage(cmd, atlas->stagingBuffer, atlas->atlasImage,
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// Update entry state
		upload->entry->state = VKNVG_GLYPH_UPLOADED;

		// Free pixel data
		free(upload->pixelData);

		stagingOffset += dataSize;
		atlas->uploads++;
	}

	// Transition back to shader read layout
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Mark image as initialized (now in SHADER_READ_ONLY layout)
	atlas->imageInitialized = true;

	atlas->uploadQueueCount = 0;
	pthread_mutex_unlock(&atlas->uploadQueueMutex);
}

// Advance frame counter
void vknvg__atlasNextFrame(VKNVGvirtualAtlas* atlas)
{
	if (!atlas) return;
	atlas->currentFrame++;
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
                                 VKNVGglyphRasterizeFunc rasterizeCallback)
{
	if (atlas) {
		atlas->fontContext = fontContext;
		atlas->rasterizeGlyph = rasterizeCallback;
	}
}
