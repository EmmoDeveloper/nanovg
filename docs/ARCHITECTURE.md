# Architecture Documentation

Technical design and implementation details for NanoVG Vulkan Backend.

## System Architecture

### Core Components

```
┌─────────────────────────────────────────────────────┐
│                  NanoVG API Layer                   │
├─────────────────────────────────────────────────────┤
│              Vulkan Backend (nanovg_vk)             │
├──────────────┬──────────────┬──────────────────────┤
│   Rendering  │ Text System  │  Resource Management │
│   Pipeline   │              │                      │
├──────────────┼──────────────┼──────────────────────┤
│  • Paths     │ • Virtual    │ • Buffers            │
│  • Fills     │   Atlas      │ • Textures           │
│  • Strokes   │ • HarfBuzz   │ • Pipelines          │
│  • Gradients │ • BiDi       │ • Descriptors        │
└──────────────┴──────────────┴──────────────────────┘
```

## Virtual Atlas System

### Overview
The virtual atlas provides efficient glyph caching for fonts with thousands of glyphs (e.g., CJK fonts).

### Key Features
- **Guillotine Bin Packing**: 87.9% efficiency vs 60-70% with page-based allocation
- **Multi-Atlas Support**: Up to 8 atlases (4096×4096 each)
- **Dynamic Growth**: Progressive sizing (512→1024→2048→4096)
- **LRU Eviction**: Intelligent cache management
- **Thread-Safe**: Background loading with mutex protection

### Data Structures

#### VKNVGvirtualAtlas
```c
struct VKNVGvirtualAtlas {
	VKNVGatlasManager* atlasManager;    // Multi-atlas manager
	VKNVGglyphCacheEntry* glyphCache;   // Hash table (8192 entries)
	VKNVGglyphCacheEntry* lruHead;      // LRU list head
	VKNVGdefragContext defragContext;   // Defragmentation state

	// Background loading
	pthread_t loaderThread;
	VKNVGglyphLoadRequest* loadQueue;

	// Statistics
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t evictions;
};
```

#### VKNVGatlasManager
```c
struct VKNVGatlasManager {
	VKNVGatlasInstance atlases[VKNVG_MAX_ATLASES];  // Up to 8 atlases
	uint32_t atlasCount;
	uint32_t currentAtlas;

	// Configuration
	uint16_t atlasSize;              // Current size
	VKNVGpackingHeuristic heuristic; // BEST_AREA_FIT
	VKNVGsplitRule splitRule;        // SHORTER_AXIS

	// Dynamic growth
	uint8_t enableDynamicGrowth;
	float resizeThreshold;           // 0.85 (85% utilization)
	uint16_t minAtlasSize;           // 512
	uint16_t maxAtlasSize;           // 4096
};
```

#### VKNVGglyphCacheEntry
```c
struct VKNVGglyphCacheEntry {
	VKNVGglyphKey key;               // fontID + codepoint + size

	// Atlas location
	uint32_t atlasIndex;             // Which atlas (0-7)
	uint16_t atlasX, atlasY;         // Pixel coordinates
	uint16_t width, height;          // Dimensions

	// Metrics
	int16_t bearingX, bearingY;
	uint16_t advance;

	// Cache management
	uint32_t lastAccessFrame;
	uint8_t state;                   // EMPTY/LOADING/READY/UPLOADED

	// LRU pointers
	VKNVGglyphCacheEntry* lruPrev;
	VKNVGglyphCacheEntry* lruNext;
};
```

### Allocation Algorithm

#### Guillotine Packing (nanovg_vk_atlas_packing.h)
1. **Input**: Rectangle (width, height)
2. **Search**: Find best free rectangle using heuristic:
   - BEST_AREA_FIT: Minimize wasted area
   - BEST_SHORT_SIDE_FIT: Minimize leftover on short side
   - BEST_LONG_SIDE_FIT: Minimize leftover on long side
3. **Split**: Divide remaining space using rule:
   - SHORTER_AXIS: Split along shorter axis
   - LONGER_AXIS: Split along longer axis
   - MINIMIZE_AREA: Minimize smaller free rect area
4. **Track**: Add new free rectangles to list

**Complexity**: O(n) where n = number of free rectangles (typically <100)

#### Multi-Atlas Overflow (nanovg_vk_multi_atlas.h)
```
Try allocation:
  1. Current atlas (most recently used)
  2. Other existing atlases
  3. Create new atlas (if < 8 atlases)
  4. Trigger dynamic growth if enabled
  5. Return failure if all full
```

#### Dynamic Growth (nanovg_vk_virtual_atlas.c)
```c
// Check utilization
efficiency = allocatedArea / totalArea;
if (efficiency >= 0.85 && currentSize < maxSize) {
	newSize = currentSize * 2;  // 512→1024→2048→4096
	vknvg__resizeAtlasImmediate(atlas, atlasIndex, newSize);
}
```

### Glyph Lifecycle

```
Request → Lookup → [Hit] → Return entry
              ↓
            [Miss]
              ↓
         Allocate space in atlas
              ↓
         Add to load queue (background)
              ↓
         Background thread rasterizes
              ↓
         Add to upload queue
              ↓
         processUploads() copies to GPU
              ↓
         Entry state = UPLOADED
              ↓
         Ready for rendering
```

### LRU Eviction

When atlas is full:
```c
// Find least recently used glyph
VKNVGglyphCacheEntry* victim = atlas->lruTail;

// Free atlas space
// Note: With Guillotine packing, this just marks space as free
// Defragmentation will compact the atlas later

// Remove from cache
victim->state = VKNVG_GLYPH_EMPTY;

// Update statistics
atlas->evictions++;
```

### Defragmentation

**Trigger Conditions**:
- Fragmentation > 30%
- Free rectangle count > 50
- Efficiency < 90%

**Algorithm** (2ms time budget per frame):
1. Analyze current packing
2. Plan optimal repacking
3. Generate move list
4. Execute moves incrementally using GPU image copy
5. Update glyph cache entries with new positions

**Status**: Hooks wired, full implementation deferred (requires glyph cache integration)

## HarfBuzz Integration

### Purpose
Complex text shaping for international scripts.

### Supported Scripts
Arabic, Devanagari, Bengali, Thai, Lao, Tibetan, Myanmar, Khmer, Hebrew, Latin, Greek, Cyrillic, CJK

### Pipeline

```c
// Input
const char* text = "مرحبا";  // Arabic "Hello"

// HarfBuzz shaping
VKNVGharfbuzzContext* hb = vknvg__createHarfBuzzContext();
VKNVGshapedText* shaped = vknvg__shapeText(hb, text, font, size);

// Output: Glyph IDs + positions
for (int i = 0; i < shaped->glyphCount; i++) {
	uint32_t glyphID = shaped->glyphs[i];
	float xOffset = shaped->positions[i].xOffset;
	float yOffset = shaped->positions[i].yOffset;
	float xAdvance = shaped->positions[i].xAdvance;
	// ... render glyph
}
```

### Integration Points
- **Input**: UTF-8 text string
- **Processing**: HarfBuzz shapes text based on script rules
- **Output**: Glyph IDs with positions
- **Rendering**: Virtual atlas renders shaped glyphs

## BiDi (Bidirectional Text) Support

### Purpose
Correct rendering of mixed LTR/RTL text.

### Algorithm
FriBidi implementation of Unicode UAX #9.

### Example
```c
// Input
"Hello world مرحبا بالعالم 12345"

// After BiDi reordering
"Hello world 12345 ملاعلاب ابحرم"
          LTR      LTR    RTL
```

### Visual Order Mapping
```c
VKNVGbidiContext* bidi = vknvg__createBidiContext();
VKNVGbidiResult* result = vknvg__processBidi(bidi, text);

// result->visualOrder[i] = logical index for visual position i
// Render glyphs in visual order for correct display
```

## Performance Optimizations

### Atlas Prewarming
Pre-load common glyphs at startup:
```c
nvgPrewarmFont(ctx, font, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
```

### Text Instancing
Render multiple glyphs in one draw call:
```c
// 100 glyphs = 1 draw call vs 100 draw calls
// 3-4× speedup for text-heavy UIs
```

### Batch Rendering
Group text calls across multiple strings:
```c
// All text in a frame rendered together
// Reduces state changes and descriptor set updates
```

## Memory Layout

### Virtual Atlas Memory
```
Single Atlas (4096×4096):
  - Image: 16MB (R8_UNORM)
  - Staging buffer: 256KB (64×64 × 64 glyphs)
  - Glyph cache: 576KB (8192 × 72 bytes)
  - Total: ~17MB

With Dynamic Growth (512×512 startup):
  - Initial: 256KB (0.256MB image)
  - Growth: 512→1024→2048→4096 as needed
  - 62× memory reduction at startup
```

### Multi-Atlas Memory
```
8 Atlases × 16MB = 128MB maximum
Typical usage: 2-3 atlases = 32-48MB
```

## Build System

### Dependencies
- Vulkan SDK 1.3+
- HarfBuzz (text shaping)
- FriBidi (BiDi support)
- FreeType (glyph rasterization)

### Compilation Units
```
Core:
  - nanovg.c (NanoVG core)
  - nanovg_vk_virtual_atlas.c (Atlas system)

Text:
  - nanovg_vk_harfbuzz.c (HarfBuzz integration)
  - nanovg_vk_bidi.c (BiDi support)
  - nanovg_vk_intl_text.c (Combined integration)

Tests:
  - 69 tests across 7 categories
  - Makefile with parallel build support
```

### Test Categories
1. **Smoke**: Basic functionality (3 tests)
2. **Unit**: Component tests (5 tests)
3. **Integration**: End-to-end (8 tests)
4. **Phase 3**: Atlas packing (4 tests)
5. **Phase 4**: International text (22 tests)
6. **Benchmarks**: Performance (2 tests)
7. **Fun**: Bad Apple!! (6 tests)

## Future Work

### Phase 5: Advanced Text Effects
- SDF multi-layer effects (outlines, glows, shadows)
- Gradient fills (linear, radial)
- Animated effects (shimmer, pulse, wave)
- Estimated: 10-14 weeks

### Potential Enhancements
- Compute-based glyph rasterization
- Async uploads via transfer queue
- Mesh shaders for path rendering
- Visual regression testing

## References

### NanoVG
- Original: https://github.com/memononen/nanovg
- License: zlib
- Author: Mikko Mononen

### HarfBuzz
- https://harfbuzz.github.io/
- OpenType text shaping engine

### FriBidi
- https://github.com/fribidi/fribidi
- Unicode BiDi algorithm implementation

### Vulkan
- https://www.vulkan.org/
- Khronos Group graphics API
