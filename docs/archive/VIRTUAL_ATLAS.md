# Virtual Atlas System for CJK Support

## Overview

The virtual atlas system provides on-demand texture atlas management for fonts with large glyph sets (e.g., CJK fonts with 50,000+ glyphs). It uses a demand-paging architecture with LRU eviction to support unlimited glyphs within a fixed-size GPU texture.

## Architecture

### Core Components

1. **Physical Atlas Texture**
   - Size: 4096x4096 pixels
   - Format: VK_FORMAT_R8_UNORM (single-channel grayscale)
   - Storage: Device-local GPU memory
   - Pages: 4,096 pages of 64x64 pixels each

2. **Glyph Cache**
   - Hash table with 8,192 entries
   - O(1) average lookup time
   - LRU eviction policy
   - Tracks: fontID, codepoint, size, atlas position, metrics

3. **Background Loading**
   - Separate pthread for glyph rasterization
   - Lock-free queue (1,024 entries)
   - Non-blocking glyph requests
   - States: EMPTY → LOADING → READY → UPLOADED

4. **Upload Synchronization**
   - Upload queue (256 glyphs/frame max)
   - Staging buffer for CPU→GPU transfers
   - Vulkan image barriers for safe transitions
   - Batch uploads during frame boundaries

### Memory Layout

```
┌─────────────────────────────────────┐
│   Physical Atlas (4096x4096)        │
│  ┌──┬──┬──┬──┐                      │
│  │ P│ P│ P│ P│  P = Page (64x64)    │
│  ├──┼──┼──┼──┤                      │
│  │ P│ P│ P│ P│  4,096 pages total   │
│  └──┴──┴──┴──┘                      │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│   Glyph Cache (Hash Table)          │
│  ┌─────────────────────┐            │
│  │ Entry[0]: font=1    │            │
│  │   codepoint=0x4E00  │            │
│  │   atlasX=4032       │            │
│  │   atlasY=4032       │            │
│  │   state=UPLOADED    │            │
│  ├─────────────────────┤            │
│  │ Entry[1]: ...       │            │
│  └─────────────────────┘            │
│  8,192 entries                      │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│   LRU List (Eviction)               │
│  Head → [Entry A] → [Entry B] → Tail│
│  (Recent)              (Old)        │
└─────────────────────────────────────┘
```

## Usage

### Enabling Virtual Atlas

```c
#include "nanovg_vk.h"

// Create NanoVG context with virtual atlas enabled
NVGVkCreateInfo createInfo = {
    .device = device,
    .physicalDevice = physicalDevice,
    // ... other fields
};

// Enable virtual atlas via flag
int flags = NVG_ANTIALIAS | NVG_VIRTUAL_ATLAS | NVG_SDF_TEXT;
NVGcontext* nvg = nvgCreateVk(&createInfo, flags);

// Virtual atlas is now active and ready for use
```

### Checking Virtual Atlas Status

```c
NVGparams* params = nvgInternalParams(nvg);
VKNVGcontext* vk = (VKNVGcontext*)params->userPtr;

if (vk->useVirtualAtlas) {
    uint32_t hits, misses, evictions, uploads;
    vknvg__getAtlasStats(vk->virtualAtlas, &hits, &misses, &evictions, &uploads);

    printf("Cache hits: %u\n", hits);
    printf("Cache misses: %u\n", misses);
    printf("Evictions: %u\n", evictions);
    printf("Uploads: %u\n", uploads);
}
```

## Implementation Status

### ✅ Completed Components

1. **Core Infrastructure**
   - Virtual atlas data structures
   - Page allocation/deallocation
   - Hash table cache implementation
   - LRU eviction algorithm

2. **Threading**
   - Background loader thread
   - Thread-safe queues (load queue, upload queue)
   - Pthread synchronization (mutexes, condition variables)

3. **GPU Integration**
   - Vulkan texture creation (4096x4096)
   - Staging buffer for uploads
   - Image memory barriers
   - Safe layout transitions

4. **NanoVG Backend Integration**
   - `NVG_VIRTUAL_ATLAS` flag (bit 15)
   - Automatic creation/destruction
   - Context integration (`VKNVGcontext::virtualAtlas`)
   - Build system integration

5. **Testing**
   - Unit tests (create/destroy, glyph requests, caching, eviction)
   - Integration tests (NanoVG flag handling, flag combinations)
   - LRU eviction tested with 5,000 glyphs (904 evictions verified)

### 🚧 Remaining Work for Full CJK Support

1. **Fontstash Integration**
   - Implement rasterization callback
   - Connect to NanoVG's `FONScontext`
   - Extract glyph metrics (width, height, bearings, advance)
   - Rasterize glyphs using `fons__tt_renderGlyphBitmap()`

2. **Text Rendering Integration**
   - Replace fixed atlas lookups with virtual atlas requests
   - Call `vknvg__requestGlyph()` in text rendering paths
   - Handle LOADING state (use placeholder or skip)
   - Call `vknvg__processUploads()` before each frame

3. **Upload Pipeline**
   - Integrate upload command buffer generation
   - Ensure uploads happen during frame boundaries
   - Handle synchronization with rendering

4. **Real-World Testing**
   - Load large CJK fonts (e.g., Noto Sans CJK)
   - Render complex text scenes
   - Measure performance with 10,000+ unique glyphs
   - Verify eviction behavior in production

### Example: Implementing Fontstash Callback

```c
// In nanovg_vk_render.h or similar

static uint8_t* vknvg__rasterizeGlyph(void* fontContext, VKNVGglyphKey key,
                                       uint16_t* width, uint16_t* height,
                                       int16_t* bearingX, int16_t* bearingY,
                                       uint16_t* advance)
{
    FONScontext* fs = (FONScontext*)fontContext;

    // Extract size from key (fixed point 16.16)
    float size = (float)(key.size >> 16) + (float)(key.size & 0xFFFF) / 65536.0f;

    // Get glyph from fontstash
    FONSglyph* glyph = fons__getGlyph(fs, font, key.codepoint, size, 0,
                                       FONS_GLYPH_BITMAP_REQUIRED);
    if (!glyph) return NULL;

    // Extract metrics
    *width = glyph->x1 - glyph->x0;
    *height = glyph->y1 - glyph->y0;
    *bearingX = glyph->xoff;
    *bearingY = glyph->yoff;
    *advance = glyph->xadv;

    // Copy pixel data
    size_t dataSize = (*width) * (*height);
    uint8_t* pixelData = (uint8_t*)malloc(dataSize);
    if (pixelData) {
        // Copy from fontstash texture atlas
        // (implementation depends on fontstash internals)
    }

    return pixelData;
}

// During virtual atlas creation:
vk->virtualAtlas = vknvg__createVirtualAtlas(
    vk->device,
    vk->physicalDevice,
    nvg->fs,  // Pass fontstash context
    vknvg__rasterizeGlyph  // Rasterization callback
);
```

## Performance Characteristics

### Tested Scenarios

| Scenario | Glyphs | Cache Hits | Cache Misses | Evictions | Result |
|----------|--------|------------|--------------|-----------|--------|
| Single glyph | 1 | 2 | 1 | 0 | ✓ Pass |
| 100 glyphs | 100 | 0 | 100 | 0 | ✓ Pass |
| 5,000 glyphs | 5,000 | 0 | 5,000 | 904 | ✓ Pass |

### Expected Performance

- **Cache hit**: O(1) hash table lookup
- **Cache miss**: Background thread rasterizes asynchronously
- **Eviction**: O(1) LRU removal
- **Upload**: Batched uploads (up to 256/frame)

### Memory Usage

- Physical atlas: 16 MB (4096×4096 × 1 byte)
- Cache entries: ~400 KB (8,192 × 48 bytes)
- Staging buffer: Variable (sized for batch uploads)
- Total: ~17 MB

## Files Modified/Created

### New Files

- `src/nanovg_vk_virtual_atlas.h` - Header with data structures and API
- `src/nanovg_vk_virtual_atlas.c` - Implementation (~700 lines)
- `tests/test_virtual_atlas.c` - Unit tests (4 tests, all passing)
- `tests/test_nvg_virtual_atlas.c` - Integration tests (3 tests, all passing)
- `VIRTUAL_ATLAS.md` - This documentation

### Modified Files

- `src/nanovg_vk_types.h` - Added `NVG_VIRTUAL_ATLAS` flag
- `src/nanovg_vk_internal.h` - Added `virtualAtlas` pointer to context
- `src/nanovg_vk_render.h` - Added virtual atlas initialization
- `src/nanovg_vk_cleanup.h` - Added virtual atlas cleanup
- `Makefile` - Added virtual atlas compilation and linking

## API Reference

### Creation/Destruction

```c
VKNVGvirtualAtlas* vknvg__createVirtualAtlas(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    void* fontContext,
    VKNVGglyphRasterizeFunc rasterizeCallback
);

void vknvg__destroyVirtualAtlas(VKNVGvirtualAtlas* atlas);
```

### Glyph Operations

```c
// Lookup existing glyph (returns NULL if not cached)
VKNVGglyphCacheEntry* vknvg__lookupGlyph(
    VKNVGvirtualAtlas* atlas,
    VKNVGglyphKey key
);

// Request glyph (loads if not cached, non-blocking)
VKNVGglyphCacheEntry* vknvg__requestGlyph(
    VKNVGvirtualAtlas* atlas,
    VKNVGglyphKey key
);
```

### Frame Management

```c
// Process pending uploads (call before frame rendering)
void vknvg__processUploads(
    VKNVGvirtualAtlas* atlas,
    VkCommandBuffer cmd
);

// Advance frame counter
void vknvg__atlasNextFrame(VKNVGvirtualAtlas* atlas);
```

### Statistics

```c
void vknvg__getAtlasStats(
    VKNVGvirtualAtlas* atlas,
    uint32_t* cacheHits,
    uint32_t* cacheMisses,
    uint32_t* evictions,
    uint32_t* uploads
);
```

## Testing

### Running Tests

```bash
# Build all tests
make tests

# Run virtual atlas unit tests
./build/test_virtual_atlas

# Run NanoVG integration tests
./build/test_nvg_virtual_atlas

# Run all tests
make run-tests
```

### Expected Output

```
==========================================
  Virtual Atlas Tests (CJK Support)
==========================================

Virtual atlas created: 4096x4096 physical, 4096 pages, 8192 cache entries
  ✓ Virtual atlas created
  ✓ Virtual atlas destroyed
  ✓ PASS test_virtual_atlas_create_destroy

...

All tests passed!
```

## Future Enhancements

1. **Multi-threaded Rasterization**
   - Use thread pool for parallel glyph loading
   - Prioritize visible glyphs

2. **Compression**
   - Use BC4/BC5 compressed formats
   - Reduce memory footprint by 4×

3. **Mipmap Generation**
   - Generate mipmaps for better minification
   - Improve text quality at small sizes

4. **Advanced Eviction**
   - Frequency-based eviction (not just LRU)
   - Keep frequently-used glyphs longer

5. **Persistent Cache**
   - Serialize atlas to disk
   - Fast startup with pre-loaded glyphs

## License

Same as NanoVG (zlib license). See main project LICENSE file.
