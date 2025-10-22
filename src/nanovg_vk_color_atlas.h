// nanovg_vk_color_atlas.h - RGBA Color Atlas for Emoji
//
// Phase 6.2: RGBA Atlas System
//
// Separate RGBA8 texture atlas for color emoji, alongside existing SDF atlas.
// Reuses Guillotine packing and multi-atlas infrastructure from Phase 3.

#ifndef NANOVG_VK_COLOR_ATLAS_H
#define NANOVG_VK_COLOR_ATLAS_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_atlas_packing.h"
#include "nanovg_vk_multi_atlas.h"

// Configuration
#define VKNVG_COLOR_ATLAS_INITIAL_SIZE 512		// Start small, grow as needed
#define VKNVG_COLOR_ATLAS_MAX_SIZE 4096			// Maximum atlas size
#define VKNVG_COLOR_GLYPH_CACHE_SIZE 2048		// Number of color glyphs to cache
#define VKNVG_COLOR_UPLOAD_QUEUE_SIZE 128		// Max glyphs pending upload per frame

// Forward declarations
typedef struct VKNVGcolorAtlas VKNVGcolorAtlas;
typedef struct VKNVGcolorGlyphCacheEntry VKNVGcolorGlyphCacheEntry;
typedef struct VKNVGcolorUploadRequest VKNVGcolorUploadRequest;

// Color glyph cache entry
struct VKNVGcolorGlyphCacheEntry {
	// Glyph key (font + codepoint + size)
	uint32_t fontID;
	uint32_t codepoint;
	uint32_t size;			// Font size in pixels
	uint32_t hash;			// Precomputed hash

	// Physical location in RGBA atlas
	uint32_t atlasIndex;	// Which atlas (0-7)
	uint16_t atlasX, atlasY;// Pixel coordinates
	uint16_t width, height;	// Glyph dimensions

	// Metrics
	int16_t bearingX, bearingY;
	uint16_t advance;

	// Cache management
	uint32_t lastAccessFrame;
	uint8_t state;			// Loading state
	uint8_t padding[3];

	// LRU list
	VKNVGcolorGlyphCacheEntry* lruPrev;
	VKNVGcolorGlyphCacheEntry* lruNext;
};

// Glyph loading states
enum VKNVGcolorGlyphState {
	VKNVG_COLOR_GLYPH_EMPTY = 0,
	VKNVG_COLOR_GLYPH_LOADING,
	VKNVG_COLOR_GLYPH_READY,
	VKNVG_COLOR_GLYPH_UPLOADED,
};

// Upload request for RGBA data
struct VKNVGcolorUploadRequest {
	uint32_t atlasIndex;
	uint16_t atlasX, atlasY;
	uint16_t width, height;
	uint8_t* rgbaData;		// RGBA8 pixel data (4 bytes per pixel)
	VKNVGcolorGlyphCacheEntry* entry;
};

// Color atlas structure
struct VKNVGcolorAtlas {
	// Vulkan context
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue graphicsQueue;
	VkCommandPool commandPool;

	// Multi-atlas management (reuse from Phase 3)
	VKNVGatlasManager* atlasManager;

	// Descriptor management
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkSampler atlasSampler;
	VkDescriptorSet* descriptorSets;	// One per atlas

	// Staging buffer for RGBA uploads
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	void* stagingMapped;
	VkDeviceSize stagingSize;

	// Glyph cache (hash table)
	VKNVGcolorGlyphCacheEntry* glyphCache;
	uint32_t glyphCacheSize;
	uint32_t glyphCount;

	// LRU list for eviction
	VKNVGcolorGlyphCacheEntry* lruHead;
	VKNVGcolorGlyphCacheEntry* lruTail;

	// Upload queue
	VKNVGcolorUploadRequest uploadQueue[VKNVG_COLOR_UPLOAD_QUEUE_SIZE];
	uint32_t uploadQueueHead;
	uint32_t uploadQueueTail;
	uint32_t uploadQueueCount;

	// Statistics
	uint32_t currentFrame;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t evictions;
};

// ============================================================================
// Atlas Management
// ============================================================================

// Create RGBA color atlas
VKNVGcolorAtlas* vknvg__createColorAtlas(VkDevice device,
                                          VkPhysicalDevice physicalDevice,
                                          VkQueue graphicsQueue,
                                          VkCommandPool commandPool);

// Destroy color atlas
void vknvg__destroyColorAtlas(VKNVGcolorAtlas* atlas);

// Begin frame (increment frame counter)
void vknvg__colorAtlasBeginFrame(VKNVGcolorAtlas* atlas);

// End frame (process uploads, cleanup)
void vknvg__colorAtlasEndFrame(VKNVGcolorAtlas* atlas);

// ============================================================================
// Glyph Cache
// ============================================================================

// Hash glyph key
uint32_t vknvg__hashColorGlyph(uint32_t fontID, uint32_t codepoint, uint32_t size);

// Lookup glyph in cache
VKNVGcolorGlyphCacheEntry* vknvg__lookupColorGlyph(VKNVGcolorAtlas* atlas,
                                                    uint32_t fontID,
                                                    uint32_t codepoint,
                                                    uint32_t size);

// Allocate cache entry (may evict LRU)
VKNVGcolorGlyphCacheEntry* vknvg__allocColorGlyphEntry(VKNVGcolorAtlas* atlas,
                                                        uint32_t fontID,
                                                        uint32_t codepoint,
                                                        uint32_t size);

// Update LRU (move to front)
void vknvg__touchColorGlyph(VKNVGcolorAtlas* atlas, VKNVGcolorGlyphCacheEntry* entry);

// Evict least recently used glyph
void vknvg__evictColorGlyph(VKNVGcolorAtlas* atlas);

// ============================================================================
// Atlas Allocation
// ============================================================================

// Allocate space in RGBA atlas
int vknvg__allocColorGlyph(VKNVGcolorAtlas* atlas,
                           uint32_t width, uint32_t height,
                           uint16_t* outAtlasX, uint16_t* outAtlasY,
                           uint32_t* outAtlasIndex);

// Free space in atlas (for defragmentation)
void vknvg__freeColorGlyph(VKNVGcolorAtlas* atlas,
                           uint32_t atlasIndex,
                           uint16_t x, uint16_t y,
                           uint16_t width, uint16_t height);

// ============================================================================
// Upload Pipeline
// ============================================================================

// Queue RGBA upload
int vknvg__queueColorUpload(VKNVGcolorAtlas* atlas,
                            uint32_t atlasIndex,
                            uint16_t x, uint16_t y,
                            uint16_t width, uint16_t height,
                            const uint8_t* rgbaData,
                            VKNVGcolorGlyphCacheEntry* entry);

// Process upload queue (called at end of frame)
void vknvg__processColorUploads(VKNVGcolorAtlas* atlas);

// Upload single glyph immediately (for testing)
void vknvg__uploadColorGlyphImmediate(VKNVGcolorAtlas* atlas,
                                      uint32_t atlasIndex,
                                      uint16_t x, uint16_t y,
                                      uint16_t width, uint16_t height,
                                      const uint8_t* rgbaData);

// ============================================================================
// Statistics
// ============================================================================

// Get cache statistics
void vknvg__getColorAtlasStats(VKNVGcolorAtlas* atlas,
                               uint32_t* outCacheHits,
                               uint32_t* outCacheMisses,
                               uint32_t* outEvictions,
                               uint32_t* outGlyphCount);

// Reset statistics
void vknvg__resetColorAtlasStats(VKNVGcolorAtlas* atlas);

// ============================================================================
// Utility Functions
// ============================================================================

// Check if atlas needs growth
int vknvg__colorAtlasNeedsGrowth(VKNVGcolorAtlas* atlas, uint32_t atlasIndex);

// Grow atlas to next size (512 → 1024 → 2048 → 4096)
int vknvg__growColorAtlas(VKNVGcolorAtlas* atlas, uint32_t atlasIndex);

// Get descriptor set for atlas
VkDescriptorSet vknvg__getColorAtlasDescriptor(VKNVGcolorAtlas* atlas, uint32_t atlasIndex);

#endif // NANOVG_VK_COLOR_ATLAS_H
