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

// Configuration
#define VKNVG_ATLAS_PAGE_SIZE 64			// Size of each atlas page (64x64 pixels)
#define VKNVG_ATLAS_PHYSICAL_SIZE 4096		// Physical atlas texture size
#define VKNVG_ATLAS_MAX_PAGES ((VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE) * (VKNVG_ATLAS_PHYSICAL_SIZE / VKNVG_ATLAS_PAGE_SIZE))
#define VKNVG_GLYPH_CACHE_SIZE 8192			// Number of glyphs to cache
#define VKNVG_UPLOAD_QUEUE_SIZE 256			// Max glyphs pending upload per frame
#define VKNVG_LOAD_QUEUE_SIZE 1024			// Background loading queue size

// Forward declarations
typedef struct VKNVGvirtualAtlas VKNVGvirtualAtlas;
typedef struct VKNVGglyphCacheEntry VKNVGglyphCacheEntry;
typedef struct VKNVGatlasPage VKNVGatlasPage;
typedef struct VKNVGglyphLoadRequest VKNVGglyphLoadRequest;
typedef struct VKNVGcomputeRaster VKNVGcomputeRaster;

// Glyph identifier (font + codepoint + size)
typedef struct VKNVGglyphKey {
	uint32_t fontID;
	uint32_t codepoint;
	uint32_t size;			// Font size in pixels (fixed point 16.16)
	uint32_t padding;		// For alignment
} VKNVGglyphKey;

// Atlas page (64x64 region in physical texture)
struct VKNVGatlasPage {
	uint16_t x, y;				// Position in physical atlas (in pages)
	uint8_t used;				// Number of glyphs using this page
	uint8_t flags;				// Page flags
	uint32_t lastAccessFrame;	// For LRU eviction
};

// Glyph cache entry
struct VKNVGglyphCacheEntry {
	VKNVGglyphKey key;

	// Physical location in atlas
	uint16_t atlasX, atlasY;	// Pixel coordinates in physical atlas
	uint16_t width, height;		// Glyph dimensions

	// Metrics
	int16_t bearingX, bearingY;	// Glyph bearing
	uint16_t advance;			// Horizontal advance

	// Cache management
	uint32_t lastAccessFrame;	// Frame number of last access
	uint32_t loadFrame;			// Frame when loaded
	uint16_t pageIndex;			// Which page this glyph occupies
	uint8_t state;				// Loading state
	uint8_t padding;

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
	uint16_t atlasX, atlasY;
	uint16_t width, height;
	uint8_t* pixelData;
	VKNVGglyphCacheEntry* entry;	// To update state after upload
} VKNVGglyphUploadRequest;

// Virtual atlas structure
struct VKNVGvirtualAtlas {
	// Physical GPU atlas
	VkImage atlasImage;
	VkImageView atlasImageView;
	VkDeviceMemory atlasMemory;
	VkSampler atlasSampler;
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	// Staging buffer for uploads
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	void* stagingMapped;
	VkDeviceSize stagingSize;

	// Page management
	VKNVGatlasPage pages[VKNVG_ATLAS_MAX_PAGES];
	uint16_t freePageCount;
	uint16_t* freePageList;			// Stack of free page indices

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
