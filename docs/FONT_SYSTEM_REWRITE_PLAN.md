# Complete Font System Rewrite Plan

## ⚠️ CRITICAL: Y-Coordinate Convention

**Vulkan uses top-left origin with Y-axis pointing DOWN**

Most graphics libraries use different conventions:
- **OpenGL**: Bottom-left origin, Y-axis pointing UP
- **FreeType**: Bottom-left origin, Y-axis pointing UP (glyph metrics)
- **Cairo**: Top-left origin, Y-axis pointing DOWN (matches Vulkan)
- **HarfBuzz**: Returns advances, not absolute positions (coordinate-agnostic)
- **NanoVG**: Top-left origin, Y-axis pointing DOWN (matches Vulkan)

### Y-Coordinate Issues to Watch:

1. **FreeType Glyph Metrics**:
   - `bearingY` is from baseline UP (positive = above baseline)
   - Need to invert: `vulkan_y = baseline_y - bearingY`
   - Glyph bitmap origin is bottom-left, needs vertical flip OR adjust texture coordinates

2. **Cairo Surfaces**:
   - Already top-left origin - matches Vulkan ✓
   - But verify cairo_surface_t coordinate system in practice

3. **HarfBuzz Positioned Glyphs**:
   - Returns `x_offset`, `y_offset` which are deltas
   - Y offsets may need sign flip depending on FreeType backend

4. **Texture Atlas Packing**:
   - Ensure glyph upload uses Vulkan convention (top-left origin)
   - Texture coordinates must match rasterized bitmap orientation

5. **NanoVG Text Positioning**:
   - `nvgText(vg, x, y, ...)` - y is baseline position
   - Top-left quad corner = `y - ascender`
   - Bottom-right quad corner = `y + descender`

### Investigation Required:
Before implementation, investigate and document for each library:
- What is the origin point? (top-left, bottom-left, center)
- Which direction is Y-axis? (up or down)
- How to convert to/from Vulkan convention?
- Test with actual render and verify no flipping needed

**Action**: Create `docs/COORDINATE_SYSTEMS.md` with findings before starting implementation.

---

## 🏗️ CRITICAL: Architectural Principle - Clean Separation

**The font system MUST be a completely independent, self-contained module.**

### Strict Interface Boundaries:

1. **No Font System Headers in Other Modules**:
   - Font system headers (`nvg_font_*.h`) must NEVER be included in:
     - `nanovg.c`
     - `nvg_vk.c`
     - `nvg_vk_context.c`
     - `nvg_vk_texture.c`
     - Any other Vulkan backend files

2. **Single Public Interface**:
   - Only ONE header exposed to NanoVG core: `nvg_font_system.h`
   - This header contains ONLY:
     - Opaque handle typedef: `typedef struct NVGFontSystem NVGFontSystem;`
     - Creation/destruction functions
     - Text rendering function (takes text, returns vertex data)
     - No internal data structures exposed

3. **Communication via Callbacks Only**:
   - NanoVG core passes callbacks TO font system (texture creation, etc.)
   - Font system NEVER calls back into NanoVG internals
   - All communication through well-defined callback interfaces

4. **No Implementation Details Leak**:
   - Internal font system files (`nvg_font_internal.h`, `nvg_font_freetype.c`, etc.) are NEVER included outside `src/font/` directory
   - All FreeType, HarfBuzz, Cairo, FriBidi details are hidden
   - Atlas management is completely internal

5. **Standalone Compilation**:
   - Font system should be compilable as a separate library
   - No dependencies on NanoVG internal structures
   - Only depends on public NanoVG types

### File Organization:
```
src/font/
├── nvg_font_system.h          # PUBLIC - only this included by nanovg.c
├── nvg_font_system.c          # Implementation
├── nvg_font_internal.h        # PRIVATE - internal structures
├── nvg_font_freetype.c        # PRIVATE - FreeType integration
├── nvg_font_harfbuzz.c        # PRIVATE - HarfBuzz shaping
├── nvg_font_cairo.c           # PRIVATE - Cairo emoji rendering
├── nvg_font_atlas.c           # PRIVATE - Atlas management
└── nvg_font_cache.c           # PRIVATE - Glyph cache
```

### Integration Pattern:
```c
// In nanovg.c - ONLY this include allowed
#include "font/nvg_font_system.h"

// Create font system with callbacks
NVGFontSystem* fs = nvgFontSystemCreate(createTextureCb, updateTextureCb, userData);

// Use font system - no knowledge of internals
nvgFontSystemRenderText(fs, "Hello", x, y, size, &vertices, &numVerts);

// Clean up
nvgFontSystemDestroy(fs);
```

**Rationale**: This ensures the font system can be:
- Replaced entirely without touching other code
- Tested independently
- Debugged in isolation
- Ported to other rendering backends
- Maintained without fear of breaking unrelated systems

---

## Overview
Complete rewrite of the NanoVG font system to be synchronous, comprehensive, and professional-grade, supporting all modern text rendering requirements.

---

## Phase 1: Code Cleanup & Removal

### 1.1 Remove Old Font Systems
**Goal**: Clean slate - remove all existing font/atlas implementations

**Files to Remove**:
- `src/nanovg_vk_virtual_atlas.h` (and .c)
- `src/nanovg_vk_virtual_atlas.h.old` (and .c.old)
- `src/nanovg_vk_atlas_packing.h` (and .c)
- `src/nanovg_vk_multi_atlas.h` (and .c)
- `src/nanovg_vk_atlas_defrag.h` (and .c)
- `src/nanovg_vk_async_upload.h` (and .c) - if exists
- `src/nanovg_vk_compute.h` (and .c) - compute raster parts
- `src/vulkan/atlas_defrag_comp.h` - compute shader
- `src/nvg_font.h` (and .c) - incomplete system
- `src/nvg_freetype.h` (and .c) - current implementation

**Code to Remove from Other Files**:
- `src/vulkan/nvg_vk_context.c`:
  - Virtual atlas creation/initialization (lines ~105-142)
  - Virtual atlas destruction (lines ~158-160)
  - Remove `#include "nanovg_vk_virtual_atlas.h"`

- `src/nvg_vk.c`:
  - Virtual atlas query callbacks (lines ~115-200)
  - Rasterization callbacks (lines ~61-113)
  - Font system created callback modifications (lines ~656-720)
  - Remove `#include "../nanovg_vk_virtual_atlas.h"`

- `src/vulkan/nvg_vk_texture.c`:
  - Special case virtual atlas texture handling (lines ~196-231)
  - Remove `#include "../nanovg_vk_virtual_atlas.h"`

- `src/nanovg.c`:
  - Remove old font system initialization
  - Remove `renderFontSystemCreated` callback
  - Remove `fontImages[]` array - will be replaced

**Makefile**:
- Remove compilation rules for deleted files
- Remove from OBJECTS list

**Verification**:
- `make clean`
- Ensure no references to removed symbols remain
- Code should not compile yet (expected) but should have no undefined references to old system

---

## Phase 2: Core Font System Design

### 2.1 Architecture Document
**File**: `docs/FONT_SYSTEM_ARCHITECTURE.md`

**Define**:
- Module boundaries and responsibilities
- Data flow (font load → glyph request → rasterization → atlas → GPU)
- Public API surface
- Internal implementation details
- Memory ownership rules
- Error handling strategy

### 2.2 Data Structures

**Core Types** (in `src/nvg_font_system.h`):
```c
// Font system instance
typedef struct NVGFontSystem NVGFontSystem;

// Font handle (returned to user)
typedef int NVGFontHandle;

// Glyph request
typedef struct {
	NVGFontHandle font;
	uint32_t codepoint;
	float size;              // in pixels
	uint32_t renderFlags;    // hinting, subpixel, etc.
} NVGGlyphRequest;

// Glyph result (texture coords + metrics)
typedef struct {
	float x0, y0, x1, y1;    // Quad vertices (bearing applied)
	float s0, t0, s1, t1;    // Texture coordinates
	float advanceX, advanceY; // Advance
	int atlasIndex;           // Which atlas texture
} NVGGlyphInfo;

// Shaped text result (for complex scripts)
typedef struct {
	uint32_t glyphCount;
	uint32_t* glyphIDs;      // Shaped glyph IDs
	float* positions;         // X,Y positions
	uint32_t* clusters;       // Character → glyph mapping
} NVGShapedText;
```

### 2.3 Module Breakdown

**Modules** (separate .h/.c files):
1. **nvg_font_system.c** - Main coordinator
2. **nvg_font_loader.c** - FreeType font loading
3. **nvg_font_rasterizer.c** - Glyph rasterization (FreeType + Cairo for COLR)
4. **nvg_font_shaper.c** - HarfBuzz text shaping
5. **nvg_font_bidi.c** - FriBidi bidirectional text
6. **nvg_font_atlas.c** - Texture atlas management
7. **nvg_font_cache.c** - Glyph cache (hash table)
8. **nvg_font_color.c** - Color space conversions
9. **nvg_font_metrics.c** - Font/glyph metrics calculations

---

## Phase 3: Font Loading (FreeType)

### 3.1 Basic Font Loading
**File**: `src/nvg_font_loader.c`

**Features**:
- Load .ttf, .otf files
- Load font collections (.ttc)
- Face selection
- Font metrics extraction (ascender, descender, line height)
- Error handling

**API**:
```c
NVGFontHandle nvgFont_load(NVGFontSystem* fs, const char* path);
NVGFontHandle nvgFont_loadFromMemory(NVGFontSystem* fs, const uint8_t* data, size_t size);
void nvgFont_unload(NVGFontSystem* fs, NVGFontHandle font);
```

### 3.2 Variable Font Support
- Axis enumeration
- Setting axis values (weight, width, slant)
- Instance creation

### 3.3 Font Fallback Chain
- Register fallback fonts
- Automatic fallback for missing glyphs
- System font discovery (optional)

---

## Phase 4: Glyph Rasterization

### 4.1 FreeType Rasterization
**File**: `src/nvg_font_rasterizer.c`

**Features**:
- Grayscale antialiased rendering
- Subpixel rendering (LCD RGB/BGR/V-RGB/V-BGR)
- Hinting modes (none, light, normal, full)
- Load glyph outline
- Render to bitmap
- Extract metrics (advance, bearing, bounding box)

**API**:
```c
typedef struct {
	uint8_t* pixelData;     // Grayscale bitmap
	int width, height;
	int pitch;               // Row stride
	int bearingX, bearingY;
	int advance;
	int bytesPerPixel;       // 1=gray, 3=RGB, 4=RGBA
} NVGRasterizedGlyph;

bool nvgRasterizer_renderGlyph(NVGFontSystem* fs,
                                 NVGGlyphRequest* req,
                                 NVGRasterizedGlyph* out);
```

### 4.2 COLR Emoji Rendering (FreeType 2.14+ + Cairo)

**Modern Standard**: COLR with true gradients, transforms, and compositing (OpenType v1.9+)

**Requirement**: FreeType 2.13.0+ (available: FreeType 2.14.1 in `/opt/freetype/`)

**Full Feature Set**:
- **Gradients**: Linear, Radial, Sweep with color stops and extend modes
- **Transformations**: Translate, Scale, Rotate, Skew, Affine (2x3 matrix)
- **Compositing**: All 26 Porter-Duff blend modes (SRC, DST, MULTIPLY, SCREEN, etc.)
- **Paint Graph**: Directed acyclic graph of nested paint operations
- **Clipping**: ClipBox per glyph for optimization
- **Variable Colors**: Integration with font variation axes
- **Color Spaces**: sRGB, Linear sRGB, CIELAB

**Implementation Strategy**:
1. Check glyph has COLR data via `FT_Get_Color_Glyph_Paint()`
2. Traverse paint graph recursively using `FT_Get_Paint()`
3. For each paint node:
   - Set up Cairo context (gradients, transforms, composite modes)
   - Recursively render child paints
   - Apply paint operation (fill, composite, etc.)
4. Extract final RGBA bitmap from Cairo surface
5. Upload to texture atlas with `VK_FORMAT_R8G8B8A8_SRGB`

**Legacy Fallback Layer**:
Modern COLR fonts embed a "v0 compatibility layer" with solid-color layers for old renderers.
- If paint graph rendering fails, fall back to `FT_Get_Color_Glyph_Layer()` (simple layer iteration)
- If no COLR data present, fall back to grayscale outline

**Note**: The "version" field in COLR table distinguishes modern (v1) from legacy (v0) formats, but conceptually we treat COLR as the modern standard with gradients, and v0 as a backward-compatibility shim.

### 4.3 SVG Emoji
- SVG table detection
- Rasterization via librsvg or similar
- Fallback to grayscale if unavailable

### 4.4 Bitmap Emoji
- CBDT/CBLC tables
- sbix table (Apple)
- Direct bitmap extraction

---

## Phase 5: Text Shaping (HarfBuzz)

### 5.1 Basic Shaping
**File**: `src/nvg_font_shaper.c`

**Features**:
- Shape UTF-8 text to glyph IDs
- Apply OpenType features
- Return positioned glyphs
- Cluster mapping (character → glyph)

**API**:
```c
NVGShapedText* nvgShaper_shape(NVGFontSystem* fs,
                                NVGFontHandle font,
                                const char* text,
                                size_t textLen,
                                float size);
void nvgShaper_freeShapedText(NVGShapedText* shaped);
```

### 5.2 OpenType Features
- Feature enumeration
- Feature enable/disable (liga, kern, calt, etc.)
- Feature tags

### 5.3 Complex Scripts
- Correct shaping for Arabic, Devanagari, Thai, etc.
- Contextual glyph substitution
- Mark positioning

---

## Phase 6: Bidirectional Text (FriBidi)

### 6.1 BiDi Algorithm
**File**: `src/nvg_font_bidi.c`

**Features**:
- UAX #9 implementation via FriBidi
- Visual reordering of RTL/LTR mixed text
- Paragraph direction detection
- Explicit direction overrides

**API**:
```c
typedef struct {
	uint32_t* visualOrder;   // Reordered character indices
	uint8_t* levels;          // Embedding levels
	size_t length;
} NVGBidiResult;

NVGBidiResult* nvgBidi_reorder(const char* text, size_t len, int baseDir);
void nvgBidi_free(NVGBidiResult* result);
```

### 6.2 Integration with Shaping
- Reorder before shaping
- Handle cursor positioning in logical order
- Selection rectangles

---

## Phase 7: Atlas Management

### 7.1 Single Atlas Implementation
**File**: `src/nvg_font_atlas.c`

**Features**:
- Fixed size texture (e.g., 2048x2048 or 4096x4096)
- Guillotine or Skyline packing algorithm
- Single format: RGBA (handles all glyph types)
- Synchronous upload

**Data Structure**:
```c
typedef struct {
	uint8_t* pixelData;      // RGBA bitmap
	int width, height;
	int usedWidth, usedHeight; // High water mark
	// Packing state
	void* packingContext;
} NVGFontAtlas;
```

**API**:
```c
NVGFontAtlas* nvgAtlas_create(int width, int height);
void nvgAtlas_destroy(NVGFontAtlas* atlas);

// Allocate space, returns position or -1 if full
bool nvgAtlas_allocate(NVGFontAtlas* atlas,
                        int width, int height,
                        int* outX, int* outY);

// Copy glyph pixels to atlas
void nvgAtlas_upload(NVGFontAtlas* atlas,
                      int x, int y,
                      int width, int height,
                      const uint8_t* pixels,
                      int bytesPerPixel);
```

### 7.2 Multi-Atlas Support
- Grow to multiple atlases when first fills
- Atlas index returned with glyph info
- GPU texture array or multiple textures

### 7.3 Eviction (Optional - Phase 8)
- LRU tracking
- Evict least recently used glyphs
- Compact/defragment periodically

---

## Phase 8: Glyph Cache

### 8.1 Cache Data Structure
**File**: `src/nvg_font_cache.c`

**Features**:
- Hash table: (font, codepoint, size) → cached glyph
- Fast lookup
- LRU list for eviction

**Cache Entry**:
```c
typedef struct {
	NVGGlyphRequest key;
	NVGGlyphInfo info;       // Atlas position + metrics
	uint32_t lastAccessFrame; // For LRU
	struct CacheEntry* lruNext;
	struct CacheEntry* lruPrev;
} NVGCacheEntry;
```

**API**:
```c
NVGCacheEntry* nvgCache_lookup(NVGFontSystem* fs, NVGGlyphRequest* req);
NVGCacheEntry* nvgCache_insert(NVGFontSystem* fs,
                                 NVGGlyphRequest* req,
                                 NVGGlyphInfo* info);
void nvgCache_touch(NVGCacheEntry* entry); // Update LRU
```

### 8.2 Eviction Strategy
- When atlas full, evict LRU glyphs
- Mark atlas regions as free
- Re-pack if fragmentation high

---

## Phase 9: Color Space Support

### 9.1 Color Space Module
**File**: `src/nvg_font_color.c`

**Features**:
- Conversion matrices for all 15 color spaces
- sRGB ↔ Linear conversions
- Wide gamut (P3, BT.2020, Adobe RGB)
- HDR (ACES, PQ, HLG)
- Proper gamma handling

**API**:
```c
void nvgColor_convert(const float* inputRGB,
                       NVGColorSpace srcSpace,
                       NVGColorSpace dstSpace,
                       float* outputRGB);
```

### 9.2 Integration
- COLR emoji rendered in sRGB, converted as needed
- Shader receives color space transform matrix
- Uniform buffer for color space (already implemented in nvg_vk_color_space.c)

---

## Phase 10: GPU Integration (Vulkan)

### 10.1 Texture Upload
**File**: `src/vulkan/nvg_vk_font_gpu.c`

**Features**:
- Create Vulkan texture from atlas
- Upload RGBA data
- Sampler configuration
- Descriptor set binding

**API**:
```c
bool nvgVkFont_uploadAtlas(VkDevice device,
                            VkCommandBuffer cmd,
                            NVGFontAtlas* atlas,
                            VkImage* outImage,
                            VkImageView* outView);
```

### 10.2 Incremental Updates
- Track atlas dirty regions
- Upload only changed areas
- Use staging buffer for transfers

### 10.3 Shader Integration
- Fragment shader samples from font atlas
- Alpha blending for grayscale
- Direct RGBA for color emoji
- Color space transform applied

---

## Phase 11: NanoVG API Integration

### 11.1 Update nanovg.h
**Changes**:
- Remove `nvgCreateFont()` - use new API
- Add `nvgLoadFontFile(ctx, name, path)`
- Add `nvgLoadFontMem(ctx, name, data, size)`
- Keep `nvgText()`, `nvgTextBox()` unchanged (internal implementation changes)

### 11.2 Update nanovg.c
**Changes**:
- Replace font system calls with new API
- Remove fontstash code
- Update `nvgText()` to use shaping + bidi + rasterization
- Update `nvgTextBounds()` to use metrics

### 11.3 Update nvg_vk.c
**Changes**:
- Initialize new font system in `nvgvk__renderCreate()`
- Create atlas texture
- Upload glyphs on demand
- Remove virtual atlas code completely

---

## Phase 12: Testing & Validation

### 12.1 Unit Tests
**Files**: `tests/font_system/`
- `test_font_loader.c` - Load various font formats
- `test_rasterizer.c` - Rasterize ASCII, CJK, emoji
- `test_shaper.c` - Shape complex scripts
- `test_bidi.c` - RTL/LTR mixing
- `test_atlas.c` - Packing, eviction
- `test_cache.c` - Lookup performance
- `test_color.c` - Color space conversions

### 12.2 Integration Tests
- ASCII text rendering
- CJK text rendering (Chinese, Japanese, Korean)
- Emoji rendering (COLR, bitmap)
- Arabic text (RTL + shaping)
- Mixed BiDi text
- Variable fonts
- Color space validation

### 12.3 Performance Tests
- Glyph cache hit rate
- Atlas utilization
- Rendering throughput (glyphs/sec)
- Memory usage

### 12.4 Visual Tests
- Screenshot comparison
- Render known text samples
- Compare with reference images

---

## Phase 13: Documentation

### 13.1 API Documentation
**File**: `docs/FONT_API.md`
- Public API reference
- Usage examples
- Best practices

### 13.2 Architecture Documentation
**File**: `docs/FONT_ARCHITECTURE.md`
- System overview
- Data flow diagrams
- Module interactions
- Extension points

### 13.3 Developer Guide
**File**: `docs/FONT_DEVELOPER_GUIDE.md`
- Building the system
- Adding new features
- Debugging techniques
- Performance profiling

---

## Phase 14: Optimization (Future)

### 14.1 MSDF Support (Optional)
- Multi-channel signed distance fields
- Scalable text at any size
- Compute shader generation

### 14.2 Shaped Text Caching
- Cache entire shaped text runs
- Hash of (text, font, size) → shaped result
- Invalidate on font change

### 14.3 Parallel Processing (Optional)
- **IF** needed for performance, add threading
- Thread pool for rasterization
- Lock-free cache
- But keep API synchronous

### 14.4 GPU Rasterization (Optional)
- Compute shader glyph rendering
- For very dynamic text

---

## Implementation Timeline

### Sprint 1 (Phase 1): Cleanup
- Remove old code
- Verify clean build

### Sprint 2 (Phases 2-3): Core + Loading
- Design architecture
- Implement font loading
- Basic FreeType integration

### Sprint 3 (Phase 4): Rasterization
- Grayscale rendering
- COLR emoji (Cairo)
- Metrics extraction

### Sprint 4 (Phases 5-6): Shaping + BiDi
- HarfBuzz integration
- FriBidi integration
- Complex script support

### Sprint 5 (Phase 7-8): Atlas + Cache
- Atlas packing
- Glyph caching
- Memory management

### Sprint 6 (Phase 9-10): Color + GPU
- Color space support
- Vulkan texture upload
- Shader integration

### Sprint 7 (Phase 11): NanoVG Integration
- Update public API
- Replace internals
- End-to-end flow

### Sprint 8 (Phase 12): Testing
- Unit tests
- Integration tests
- Visual validation

### Sprint 9 (Phase 13): Documentation
- API docs
- Architecture docs
- Examples

---

## Success Criteria

1. ✅ **Correctness**: All text renders correctly (no scrambled glyphs)
2. ✅ **Completeness**: Supports all required features (COLR, CJK, BiDi, etc.)
3. ✅ **Performance**: Acceptable frame rate with heavy text
4. ✅ **Synchronous**: No threading in font system
5. ✅ **Maintainable**: Clean code, documented, testable
6. ✅ **Extensible**: Easy to add new features

---

## Notes

- **Synchronous by Design**: All operations complete before returning. No background threads.
- **Incremental Development**: Each phase builds on previous, can be tested independently.
- **Pragmatic Approach**: Start simple, add complexity only when needed.
- **Leverage Existing Libraries**: FreeType, HarfBuzz, FriBidi, Cairo - don't reinvent.
