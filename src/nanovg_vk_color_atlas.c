// nanovg_vk_color_atlas.c - RGBA Color Atlas Implementation

#include "nanovg_vk_color_atlas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Creation/Destruction
// ============================================================================

VKNVGcolorAtlas* vknvg__createColorAtlas(VkDevice device,
                                          VkPhysicalDevice physicalDevice,
                                          VkQueue graphicsQueue,
                                          VkCommandPool commandPool)
{
	VKNVGcolorAtlas* atlas = (VKNVGcolorAtlas*)calloc(1, sizeof(VKNVGcolorAtlas));
	if (!atlas) return NULL;

	atlas->device = device;
	atlas->physicalDevice = physicalDevice;
	atlas->graphicsQueue = graphicsQueue;
	atlas->commandPool = commandPool;

	// Create descriptor pool for atlas textures
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 8; // Max 8 atlases

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = 8;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(device, &poolInfo, NULL, &atlas->descriptorPool) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding binding = {0};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &atlas->descriptorSetLayout) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Create sampler
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	if (vkCreateSampler(device, &samplerInfo, NULL, &atlas->atlasSampler) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Create multi-atlas manager with RGBA format
	atlas->atlasManager = vknvg__createAtlasManager(device, physicalDevice,
	                                                 atlas->descriptorPool,
	                                                 atlas->descriptorSetLayout,
	                                                 atlas->atlasSampler,
	                                                 VKNVG_COLOR_ATLAS_INITIAL_SIZE);
	if (!atlas->atlasManager) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Override format to RGBA
	atlas->atlasManager->format = VK_FORMAT_R8G8B8A8_UNORM;

	// Create staging buffer for uploads (4MB should be enough)
	atlas->stagingSize = 4 * 1024 * 1024;
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = atlas->stagingSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, NULL, &atlas->stagingBuffer) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Allocate staging memory (host visible)
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, atlas->stagingBuffer, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	uint32_t memoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReqs.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == UINT32_MAX) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, NULL, &atlas->stagingMemory) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	vkBindBufferMemory(device, atlas->stagingBuffer, atlas->stagingMemory, 0);

	// Map staging memory
	if (vkMapMemory(device, atlas->stagingMemory, 0, atlas->stagingSize, 0, &atlas->stagingMapped) != VK_SUCCESS) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Allocate glyph cache
	atlas->glyphCacheSize = VKNVG_COLOR_GLYPH_CACHE_SIZE;
	atlas->glyphCache = (VKNVGcolorGlyphCacheEntry*)calloc(atlas->glyphCacheSize,
	                                                        sizeof(VKNVGcolorGlyphCacheEntry));
	if (!atlas->glyphCache) {
		vknvg__destroyColorAtlas(atlas);
		return NULL;
	}

	// Initialize LRU list (all entries start in free list)
	atlas->lruHead = NULL;
	atlas->lruTail = NULL;

	// Initialize upload queue
	atlas->uploadQueueHead = 0;
	atlas->uploadQueueTail = 0;
	atlas->uploadQueueCount = 0;

	// Initialize statistics
	atlas->currentFrame = 0;
	atlas->cacheHits = 0;
	atlas->cacheMisses = 0;
	atlas->evictions = 0;

	return atlas;
}

void vknvg__destroyColorAtlas(VKNVGcolorAtlas* atlas)
{
	if (!atlas) return;

	if (atlas->device) {
		if (atlas->stagingMapped) {
			vkUnmapMemory(atlas->device, atlas->stagingMemory);
		}
		if (atlas->stagingBuffer) {
			vkDestroyBuffer(atlas->device, atlas->stagingBuffer, NULL);
		}
		if (atlas->stagingMemory) {
			vkFreeMemory(atlas->device, atlas->stagingMemory, NULL);
		}
		if (atlas->descriptorPool) {
			vkDestroyDescriptorPool(atlas->device, atlas->descriptorPool, NULL);
		}
		if (atlas->descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(atlas->device, atlas->descriptorSetLayout, NULL);
		}
		if (atlas->atlasSampler) {
			vkDestroySampler(atlas->device, atlas->atlasSampler, NULL);
		}
	}

	if (atlas->atlasManager) {
		vknvg__destroyAtlasManager(atlas->atlasManager);
	}

	free(atlas->glyphCache);
	free(atlas->descriptorSets);
	free(atlas);
}

// ============================================================================
// Frame Management
// ============================================================================

void vknvg__colorAtlasBeginFrame(VKNVGcolorAtlas* atlas)
{
	if (!atlas) return;
	atlas->currentFrame++;
}

void vknvg__colorAtlasEndFrame(VKNVGcolorAtlas* atlas)
{
	if (!atlas) return;

	// Process pending uploads
	vknvg__processColorUploads(atlas);
}

// ============================================================================
// Hash and Cache Lookup
// ============================================================================

uint32_t vknvg__hashColorGlyph(uint32_t fontID, uint32_t codepoint, uint32_t size)
{
	// FNV-1a hash
	uint32_t hash = 2166136261u;
	hash = (hash ^ fontID) * 16777619u;
	hash = (hash ^ codepoint) * 16777619u;
	hash = (hash ^ size) * 16777619u;
	return hash;
}

VKNVGcolorGlyphCacheEntry* vknvg__lookupColorGlyph(VKNVGcolorAtlas* atlas,
                                                    uint32_t fontID,
                                                    uint32_t codepoint,
                                                    uint32_t size)
{
	if (!atlas) return NULL;

	uint32_t hash = vknvg__hashColorGlyph(fontID, codepoint, size);
	uint32_t index = hash % atlas->glyphCacheSize;

	// Linear probing
	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probeIndex = (index + i) % atlas->glyphCacheSize;
		VKNVGcolorGlyphCacheEntry* entry = &atlas->glyphCache[probeIndex];

		if (entry->state == VKNVG_COLOR_GLYPH_EMPTY) {
			// Not found
			return NULL;
		}

		if (entry->hash == hash &&
		    entry->fontID == fontID &&
		    entry->codepoint == codepoint &&
		    entry->size == size) {
			// Found
			atlas->cacheHits++;
			vknvg__touchColorGlyph(atlas, entry);
			return entry;
		}
	}

	return NULL;
}

VKNVGcolorGlyphCacheEntry* vknvg__allocColorGlyphEntry(VKNVGcolorAtlas* atlas,
                                                        uint32_t fontID,
                                                        uint32_t codepoint,
                                                        uint32_t size)
{
	if (!atlas) return NULL;

	atlas->cacheMisses++;

	uint32_t hash = vknvg__hashColorGlyph(fontID, codepoint, size);
	uint32_t index = hash % atlas->glyphCacheSize;

	// Find empty slot
	for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
		uint32_t probeIndex = (index + i) % atlas->glyphCacheSize;
		VKNVGcolorGlyphCacheEntry* entry = &atlas->glyphCache[probeIndex];

		if (entry->state == VKNVG_COLOR_GLYPH_EMPTY) {
			// Found empty slot
			memset(entry, 0, sizeof(VKNVGcolorGlyphCacheEntry));
			entry->fontID = fontID;
			entry->codepoint = codepoint;
			entry->size = size;
			entry->hash = hash;
			entry->state = VKNVG_COLOR_GLYPH_LOADING;
			entry->lastAccessFrame = atlas->currentFrame;

			atlas->glyphCount++;
			vknvg__touchColorGlyph(atlas, entry);
			return entry;
		}
	}

	// Cache full, evict LRU
	vknvg__evictColorGlyph(atlas);

	// Try again
	return vknvg__allocColorGlyphEntry(atlas, fontID, codepoint, size);
}

void vknvg__touchColorGlyph(VKNVGcolorAtlas* atlas, VKNVGcolorGlyphCacheEntry* entry)
{
	if (!atlas || !entry) return;

	entry->lastAccessFrame = atlas->currentFrame;

	// Remove from current position in LRU list
	if (entry->lruPrev) {
		entry->lruPrev->lruNext = entry->lruNext;
	} else if (atlas->lruHead == entry) {
		atlas->lruHead = entry->lruNext;
	}

	if (entry->lruNext) {
		entry->lruNext->lruPrev = entry->lruPrev;
	} else if (atlas->lruTail == entry) {
		atlas->lruTail = entry->lruPrev;
	}

	// Move to front
	entry->lruPrev = NULL;
	entry->lruNext = atlas->lruHead;
	if (atlas->lruHead) {
		atlas->lruHead->lruPrev = entry;
	}
	atlas->lruHead = entry;

	if (!atlas->lruTail) {
		atlas->lruTail = entry;
	}
}

void vknvg__evictColorGlyph(VKNVGcolorAtlas* atlas)
{
	if (!atlas || !atlas->lruTail) return;

	VKNVGcolorGlyphCacheEntry* entry = atlas->lruTail;

	// Free atlas space
	if (entry->state == VKNVG_COLOR_GLYPH_UPLOADED) {
		vknvg__freeColorGlyph(atlas, entry->atlasIndex,
		                      entry->atlasX, entry->atlasY,
		                      entry->width, entry->height);
	}

	// Remove from LRU list
	if (entry->lruPrev) {
		entry->lruPrev->lruNext = NULL;
	}
	atlas->lruTail = entry->lruPrev;

	if (atlas->lruHead == entry) {
		atlas->lruHead = NULL;
	}

	// Clear entry
	memset(entry, 0, sizeof(VKNVGcolorGlyphCacheEntry));
	atlas->glyphCount--;
	atlas->evictions++;
}

// ============================================================================
// Atlas Allocation
// ============================================================================

int vknvg__allocColorGlyph(VKNVGcolorAtlas* atlas,
                           uint32_t width, uint32_t height,
                           uint16_t* outAtlasX, uint16_t* outAtlasY,
                           uint32_t* outAtlasIndex)
{
	if (!atlas || !atlas->atlasManager) return 0;

	// Simplified allocation - use first atlas for now
	// Full multi-atlas will be integrated later
	if (atlas->atlasManager->atlasCount == 0) return 0;

	VKNVGatlasInstance* instance = &atlas->atlasManager->atlases[0];
	VKNVGrect rect;

	if (vknvg__packRect(&instance->packer, (uint16_t)width, (uint16_t)height, &rect,
	                    VKNVG_PACK_BEST_SHORT_SIDE_FIT, VKNVG_SPLIT_SHORTER_AXIS)) {
		*outAtlasX = rect.x;
		*outAtlasY = rect.y;
		*outAtlasIndex = 0;
		return 1;
	}

	return 0;
}

void vknvg__freeColorGlyph(VKNVGcolorAtlas* atlas,
                           uint32_t atlasIndex,
                           uint16_t x, uint16_t y,
                           uint16_t width, uint16_t height)
{
	if (!atlas || !atlas->atlasManager) return;
	if (atlasIndex >= atlas->atlasManager->atlasCount) return;

	// Simplified free - packer doesn't support free yet
	// Will be added with defragmentation
	(void)x;
	(void)y;
	(void)width;
	(void)height;
}

// ============================================================================
// Upload Pipeline
// ============================================================================

int vknvg__queueColorUpload(VKNVGcolorAtlas* atlas,
                            uint32_t atlasIndex,
                            uint16_t x, uint16_t y,
                            uint16_t width, uint16_t height,
                            const uint8_t* rgbaData,
                            VKNVGcolorGlyphCacheEntry* entry)
{
	if (!atlas || !rgbaData) return 0;

	if (atlas->uploadQueueCount >= VKNVG_COLOR_UPLOAD_QUEUE_SIZE) {
		// Queue full, process now
		vknvg__processColorUploads(atlas);
		if (atlas->uploadQueueCount >= VKNVG_COLOR_UPLOAD_QUEUE_SIZE) {
			return 0; // Still full
		}
	}

	VKNVGcolorUploadRequest* req = &atlas->uploadQueue[atlas->uploadQueueTail];
	req->atlasIndex = atlasIndex;
	req->atlasX = x;
	req->atlasY = y;
	req->width = width;
	req->height = height;

	// Copy RGBA data
	uint32_t dataSize = width * height * 4;
	req->rgbaData = (uint8_t*)malloc(dataSize);
	if (!req->rgbaData) return 0;
	memcpy(req->rgbaData, rgbaData, dataSize);

	req->entry = entry;

	atlas->uploadQueueTail = (atlas->uploadQueueTail + 1) % VKNVG_COLOR_UPLOAD_QUEUE_SIZE;
	atlas->uploadQueueCount++;

	return 1;
}

void vknvg__processColorUploads(VKNVGcolorAtlas* atlas)
{
	if (!atlas || atlas->uploadQueueCount == 0) return;

	// Process all pending uploads
	while (atlas->uploadQueueCount > 0) {
		VKNVGcolorUploadRequest* req = &atlas->uploadQueue[atlas->uploadQueueHead];

		// Upload immediately
		vknvg__uploadColorGlyphImmediate(atlas, req->atlasIndex,
		                                 req->atlasX, req->atlasY,
		                                 req->width, req->height,
		                                 req->rgbaData);

		// Update entry state
		if (req->entry) {
			req->entry->state = VKNVG_COLOR_GLYPH_UPLOADED;
		}

		// Free data
		free(req->rgbaData);
		req->rgbaData = NULL;

		atlas->uploadQueueHead = (atlas->uploadQueueHead + 1) % VKNVG_COLOR_UPLOAD_QUEUE_SIZE;
		atlas->uploadQueueCount--;
	}
}

void vknvg__uploadColorGlyphImmediate(VKNVGcolorAtlas* atlas,
                                      uint32_t atlasIndex,
                                      uint16_t x, uint16_t y,
                                      uint16_t width, uint16_t height,
                                      const uint8_t* rgbaData)
{
	if (!atlas || !atlas->atlasManager || !rgbaData) return;
	if (atlasIndex >= atlas->atlasManager->atlasCount) return;

	VKNVGatlasInstance* targetAtlas = &atlas->atlasManager->atlases[atlasIndex];
	if (!targetAtlas->image) return;

	// Copy to staging buffer
	uint32_t dataSize = width * height * 4;
	if (dataSize > atlas->stagingSize) {
		fprintf(stderr, "Upload too large: %u bytes\n", dataSize);
		return;
	}

	memcpy(atlas->stagingMapped, rgbaData, dataSize);

	// Create command buffer for upload
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = atlas->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuf;
	if (vkAllocateCommandBuffers(atlas->device, &allocInfo, &cmdBuf) != VK_SUCCESS) {
		return;
	}

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	// Transition to transfer dst
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = targetAtlas->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmdBuf,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy buffer to image
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

	vkCmdCopyBufferToImage(cmdBuf, atlas->stagingBuffer, targetAtlas->image,
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition back to shader read
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuf,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(cmdBuf);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	vkQueueSubmit(atlas->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(atlas->graphicsQueue);

	vkFreeCommandBuffers(atlas->device, atlas->commandPool, 1, &cmdBuf);
}

// ============================================================================
// Statistics
// ============================================================================

void vknvg__getColorAtlasStats(VKNVGcolorAtlas* atlas,
                               uint32_t* outCacheHits,
                               uint32_t* outCacheMisses,
                               uint32_t* outEvictions,
                               uint32_t* outGlyphCount)
{
	if (!atlas) return;
	if (outCacheHits) *outCacheHits = atlas->cacheHits;
	if (outCacheMisses) *outCacheMisses = atlas->cacheMisses;
	if (outEvictions) *outEvictions = atlas->evictions;
	if (outGlyphCount) *outGlyphCount = atlas->glyphCount;
}

void vknvg__resetColorAtlasStats(VKNVGcolorAtlas* atlas)
{
	if (!atlas) return;
	atlas->cacheHits = 0;
	atlas->cacheMisses = 0;
	atlas->evictions = 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

int vknvg__colorAtlasNeedsGrowth(VKNVGcolorAtlas* atlas, uint32_t atlasIndex)
{
	if (!atlas || !atlas->atlasManager) return 0;
	if (atlasIndex >= atlas->atlasManager->atlasCount) return 0;

	// Check utilization via packer stats
	VKNVGatlasInstance* instance = &atlas->atlasManager->atlases[atlasIndex];
	float utilization = vknvg__getPackingEfficiency(&instance->packer);
	return utilization > 0.85f; // Grow at 85% full
}

int vknvg__growColorAtlas(VKNVGcolorAtlas* atlas, uint32_t atlasIndex)
{
	if (!atlas || !atlas->atlasManager) return 0;
	if (atlasIndex >= atlas->atlasManager->atlasCount) return 0;

	// Grow atlas (512 → 1024 → 2048 → 4096)
	uint32_t newSize = atlas->atlasManager->atlasSize * 2;

	if (newSize > VKNVG_COLOR_ATLAS_MAX_SIZE) {
		return 0; // Can't grow further
	}

	// Allocate command buffer for resize
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = atlas->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuf;
	if (vkAllocateCommandBuffers(atlas->device, &allocInfo, &cmdBuf) != VK_SUCCESS) {
		return 0;
	}

	// Use resize function from multi-atlas
	VkResult result = vknvg__resizeAtlasInstance(atlas->atlasManager, atlasIndex, newSize, cmdBuf);

	vkFreeCommandBuffers(atlas->device, atlas->commandPool, 1, &cmdBuf);

	return result == VK_SUCCESS;
}

VkDescriptorSet vknvg__getColorAtlasDescriptor(VKNVGcolorAtlas* atlas, uint32_t atlasIndex)
{
	if (!atlas || !atlas->atlasManager) return VK_NULL_HANDLE;
	if (atlasIndex >= atlas->atlasManager->atlasCount) return VK_NULL_HANDLE;

	// Get descriptor from atlas instance
	return atlas->atlasManager->atlases[atlasIndex].descriptorSet;
}
