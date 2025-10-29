// nanovg_vk_virtual_atlas.h - Virtual Atlas for Large Glyph Sets (CJK Support)
//
// Implements a virtualized texture atlas with demand paging for fonts with
// tens of thousands of glyphs (e.g., full CJK character sets).
//
// Architecture:
// - Physical atlas texture (e.g., 4096x4096) stored on GPU
// - Virtual glyph space (unlimited) with on-demand loading
// - LRU cache eviction for managing limited atlas space
// - Background thread for glyph rasterization
// - Safe upload synchronization between frames

#ifndef NANOVG_VK_VIRTUAL_ATLAS_H
#define NANOVG_VK_VIRTUAL_ATLAS_H

#include <vulkan/vulkan.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_atlas_packing.h"
#include "nanovg_vk_multi_atlas.h"
#include "nanovg_vk_atlas_defrag.h"

// Configuration
#define VKNVG_ATLAS_PHYSICAL_SIZE 4096		// Default atlas texture size
#define VKNVG_GLYPH_CACHE_SIZE 8192			// Number of glyphs to cache
#define VKNVG_UPLOAD_QUEUE_SIZE 256			// Max glyphs pending upload per frame
#define VKNVG_LOAD_QUEUE_SIZE 1024			// Background loading queue size

// Phase 1: Legacy page system constants (will be removed in Phase 3)
#define VKNVG_ATLAS_PAGE_SIZE 64			// 64x64 pixel pages
#define VKNVG_ATLAS_MAX_PAGES ((VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE) * \
                               (VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE))

// Phase 3 integration flags
#define VKNVG_USE_GUILLOTINE_PACKING 1		// Use Guillotine packing instead of pages
#define VKNVG_USE_MULTI_ATLAS 1				// Enable multi-atlas support
#define VKNVG_USE_DYNAMIC_GROWTH 1			// Enable dynamic atlas growth
#define VKNVG_USE_DEFRAGMENTATION 1			// Enable idle-frame defragmentation

// Forward declarations
typedef struct VKNVGvirtualAtlas VKNVGvirtualAtlas;
typedef struct VKNVGglyphCacheEntry VKNVGglyphCacheEntry;
typedef struct VKNVGglyphLoadRequest VKNVGglyphLoadRequest;
typedef struct VKNVGcomputeRaster VKNVGcomputeRaster;
typedef struct VKNVGasyncUpload VKNVGasyncUpload;

// Phase 1: Legacy page structure (will be removed in Phase 3)
typedef struct VKNVGatlasPage {
	uint16_t x, y;
	uint8_t used;
	uint8_t flags;
	uint32_t lastAccessFrame;
} VKNVGatlasPage;

// Glyph identifier (font + codepoint + size)
typedef struct VKNVGglyphKey {
	uint32_t fontID;
	uint32_t codepoint;
	uint32_t size;			// Font size in pixels (fixed point 16.16)
	uint32_t padding;		// For alignment
} VKNVGglyphKey;

// Glyph cache entry
struct VKNVGglyphCacheEntry {
	VKNVGglyphKey key;

	// Physical location in atlas
	uint32_t atlasIndex;		// Which atlas this glyph is in (Phase 3: multi-atlas)
	uint16_t atlasX, atlasY;	// Pixel coordinates in physical atlas
	uint16_t width, height;		// Glyph dimensions

	// Metrics
	int16_t bearingX, bearingY;	// Glyph bearing
	uint16_t advance;			// Horizontal advance

	// Cache management
	uint32_t lastAccessFrame;	// Frame number of last access
	uint32_t loadFrame;			// Frame when loaded
	uint8_t state;				// Loading state
	uint8_t padding[3];			// Alignment

	// Phase 1: Legacy page system (will be removed in Phase 3)
	uint16_t pageIndex;			// Page index in legacy system

	// LRU list pointers
	VKNVGglyphCacheEntry* lruPrev;
	VKNVGglyphCacheEntry* lruNext;
};

// Glyph loading states
enum VKNVGglyphState {
	VKNVG_GLYPH_EMPTY = 0,		// Not loaded
	VKNVG_GLYPH_LOADING,		// Background thread is loading
	VKNVG_GLYPH_READY,			// Loaded, pending upload
	VKNVG_GLYPH_UPLOADED,		// Uploaded to GPU, ready to render
};

// Glyph load request (for background thread)
struct VKNVGglyphLoadRequest {
	VKNVGglyphKey key;
	VKNVGglyphCacheEntry* entry;	// Pointer to cache entry to fill
	uint8_t* pixelData;				// Rasterized pixel data (allocated by loader)
	uint32_t timestamp;				// Request timestamp for prioritization
};

// Upload request (for GPU upload between frames)
typedef struct VKNVGglyphUploadRequest {
	uint32_t atlasIndex;			// Phase 3 Advanced: Which atlas to upload to
	uint16_t atlasX, atlasY;
	uint16_t width, height;
	uint8_t* pixelData;
	VKNVGglyphCacheEntry* entry;	// To update state after upload
} VKNVGglyphUploadRequest;

// Virtual atlas structure
struct VKNVGvirtualAtlas {
	// Vulkan context
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	// Phase 3: Multi-atlas management
	VKNVGatlasManager* atlasManager;	// Manages multiple atlases with Guillotine packing
	VkDescriptorPool descriptorPool;	// For atlas descriptor sets
	VkDescriptorSetLayout descriptorSetLayout;	// Atlas texture descriptor layout
	VkSampler atlasSampler;				// Shared sampler for all atlases

	// Phase 3: Defragmentation
	VKNVGdefragContext defragContext;	// Defragmentation state
	uint8_t enableDefrag;				// Enable idle-frame defragmentation
	uint8_t padding[3];					// Alignment

	// Compute shader support for defragmentation
	VKNVGcomputeContext* computeContext;	// NULL if compute shaders disabled
	VkBool32 useComputeDefrag;				// Enable GPU-side defragmentation

	// Staging buffer for uploads
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	void* stagingMapped;
	VkDeviceSize stagingSize;

	// Glyph cache (hash table)
	VKNVGglyphCacheEntry* glyphCache;	// Array of cache entries
	uint32_t glyphCacheSize;
	uint32_t glyphCount;				// Number of cached glyphs

	// LRU list for eviction
	VKNVGglyphCacheEntry* lruHead;
	VKNVGglyphCacheEntry* lruTail;

	// Background loading
	pthread_t loaderThread;
	pthread_mutex_t loadQueueMutex;
	pthread_cond_t loadQueueCond;
	VKNVGglyphLoadRequest* loadQueue;
	uint32_t loadQueueHead;
	uint32_t loadQueueTail;
	uint32_t loadQueueCount;
	bool loaderThreadRunning;

	// Upload queue (GPU uploads happen between frames)
	pthread_mutex_t uploadQueueMutex;
	VKNVGglyphUploadRequest uploadQueue[VKNVG_UPLOAD_QUEUE_SIZE];
	uint32_t uploadQueueCount;

	// Statistics
	uint32_t currentFrame;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t evictions;
	uint32_t uploads;

	// Image layout tracking
	bool imageInitialized;	// True after first upload (image transitioned to SHADER_READ_ONLY)

	// Font context (for rasterization)
	void* fontContext;				// FONScontext* (opaque)

	// Glyph rasterization callback
	// Returns pixel data (caller must free), fills width/height/bearingX/bearingY/advance
	uint8_t* (*rasterizeGlyph)(void* fontContext, VKNVGglyphKey key,
	                           uint16_t* width, uint16_t* height,
	                           int16_t* bearingX, int16_t* bearingY,
	                           uint16_t* advance);

	// Optional GPU compute rasterization
	VKNVGcomputeRaster* computeRaster;		// NULL if using CPU rasterization
	VkBool32 useComputeRaster;				// Enable GPU rasterization
	VkQueue computeQueue;					// Compute queue for GPU rasterization
	uint32_t computeQueueFamily;			// Queue family index

	// Optional async upload system
	VKNVGasyncUpload* asyncUpload;			// NULL if using synchronous uploads
	VkBool32 useAsyncUpload;				// Enable async uploads
	VkQueue transferQueue;					// Transfer queue for async uploads
	uint32_t transferQueueFamily;			// Transfer queue family index

	// Phase 1: Legacy page system (will be removed in Phase 3)
	VKNVGatlasPage pages[VKNVG_ATLAS_MAX_PAGES];	// Page allocation array
	uint16_t* freePageList;					// List of free page indices
	uint16_t freePageCount;					// Number of free pages

	// Phase 1: Legacy single atlas resources (will be removed with multi-atlas)
	VkImage atlasImage;						// Single physical atlas image
	VkImageView atlasImageView;				// Image view for atlas
	VkDeviceMemory atlasMemory;				// Device memory for atlas
};

// API Functions

// Glyph rasterization callback type
typedef uint8_t* (*VKNVGglyphRasterizeFunc)(void* fontContext, VKNVGglyphKey key,
                                             uint16_t* width, uint16_t* height,
                                             int16_t* bearingX, int16_t* bearingY,
                                             uint16_t* advance);

// Create virtual atlas
VKNVGvirtualAtlas* vknvg__createVirtualAtlas(VkDevice device,
                                              VkPhysicalDevice physicalDevice,
                                              void* fontContext,
                                              VKNVGglyphRasterizeFunc rasterizeCallback);

// Destroy virtual atlas
void vknvg__destroyVirtualAtlas(VKNVGvirtualAtlas* atlas);

// Lookup glyph in cache, returns NULL if not loaded
VKNVGglyphCacheEntry* vknvg__lookupGlyph(VKNVGvirtualAtlas* atlas,
                                          VKNVGglyphKey key);

// Request glyph load (adds to background queue if not in cache)
VKNVGglyphCacheEntry* vknvg__requestGlyph(VKNVGvirtualAtlas* atlas,
                                           VKNVGglyphKey key);

// Process upload queue (call between frames)
void vknvg__processUploads(VKNVGvirtualAtlas* atlas, VkCommandBuffer cmd);

// Advance frame counter
void vknvg__atlasNextFrame(VKNVGvirtualAtlas* atlas);

// Get statistics
void vknvg__getAtlasStats(VKNVGvirtualAtlas* atlas,
                          uint32_t* cacheHits, uint32_t* cacheMisses,
                          uint32_t* evictions, uint32_t* uploads);

// Set font context and rasterization callback (call after font initialization)
void vknvg__setAtlasFontContext(VKNVGvirtualAtlas* atlas,
                                 void* fontContext,
                                 VKNVGglyphRasterizeFunc rasterizeCallback);

// Add glyph directly with pre-rasterized data (bypasses background loading)
// Returns cache entry if successful, NULL if atlas is full
// Caller must NOT free pixelData - virtual atlas takes ownership
VKNVGglyphCacheEntry* vknvg__addGlyphDirect(VKNVGvirtualAtlas* atlas,
                                             VKNVGglyphKey key,
                                             uint8_t* pixelData,
                                             uint16_t width, uint16_t height,
                                             int16_t bearingX, int16_t bearingY,
                                             uint16_t advance);

// Enable async uploads (creates async upload context)
// Call after virtual atlas creation, before any glyph uploads
// Returns VK_SUCCESS on success, error code otherwise
VkResult vknvg__enableAsyncUploads(VKNVGvirtualAtlas* atlas,
                                    VkQueue transferQueue,
                                    uint32_t transferQueueFamily);

// Get semaphore to wait on for async uploads (for graphics queue synchronization)
// Returns VK_NULL_HANDLE if no uploads are pending
VkSemaphore vknvg__getUploadSemaphore(VKNVGvirtualAtlas* atlas);

// Enable compute shader defragmentation (creates compute context)
// Call after virtual atlas creation to enable GPU-accelerated defragmentation
// Returns VK_SUCCESS on success, error code otherwise
VkResult vknvg__enableComputeDefragmentation(VKNVGvirtualAtlas* atlas,
                                              VkQueue computeQueue,
                                              uint32_t computeQueueFamily);

// Internal functions

// Hash function for glyph key
static inline uint32_t vknvg__hashGlyphKey(VKNVGglyphKey key)
{
	// FNV-1a hash
	uint32_t hash = 2166136261u;
	hash ^= key.fontID;
	hash *= 16777619u;
	hash ^= key.codepoint;
	hash *= 16777619u;
	hash ^= key.size;
	hash *= 16777619u;
	return hash;
}

// Compare glyph keys
static inline bool vknvg__glyphKeyEqual(VKNVGglyphKey a, VKNVGglyphKey b)
{
	return a.fontID == b.fontID &&
	       a.codepoint == b.codepoint &&
	       a.size == b.size;
}

#endif // NANOVG_VK_VIRTUAL_ATLAS_H
