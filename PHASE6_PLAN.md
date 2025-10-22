# Phase 6: Color Emoji Support - Implementation Plan

## Overview

Add color emoji rendering support to NanoVG Vulkan backend using industry-standard font formats.

## Goals

1. **Bitmap Emoji Support**: SBIX (Apple) and CBDT/CBLC (Google) formats
2. **Vector Emoji Support**: COLRv0 (layered color glyphs)
3. **RGBA Atlas System**: New color texture atlas alongside SDF atlas
4. **Automatic Format Detection**: Choose best available format from font
5. **Seamless Integration**: Works with existing text rendering API

## Target Fonts

- **Apple Color Emoji** (SBIX) - iOS/macOS system font
- **Noto Color Emoji** (CBDT + COLRv1) - Google's open emoji font
- **Twemoji** (CBDT/SVG) - Twitter's emoji set
- **Emoji One/JoyPixels** (SVG) - Commercial emoji fonts

## Architecture Overview

### Dual Atlas System

```
┌─────────────────────┐         ┌──────────────────────┐
│   SDF Atlas         │         │   RGBA Color Atlas   │
│   (Grayscale R8)    │         │   (Color RGBA8)      │
│                     │         │                      │
│ - Regular text      │         │ - Color emoji        │
│ - Monochrome emoji  │         │ - SBIX bitmaps       │
│ - Text effects      │         │ - CBDT bitmaps       │
└─────────────────────┘         │ - COLR layers        │
                                └──────────────────────┘
```

### Rendering Pipeline

```
Font Type Detection
        │
        ├──> Has SBIX/CBDT? ──> Bitmap Pipeline ──> RGBA Atlas
        ├──> Has COLR?      ──> Vector Pipeline ──> RGBA Atlas
        └──> Fallback       ──> SDF Pipeline   ──> SDF Atlas
```

### Font Format Priority

1. **COLR/CPAL** (if available) - Best scalability
2. **SBIX** (Apple fonts) - High quality bitmaps
3. **CBDT/CBLC** (Google fonts) - Compact bitmaps
4. **Fallback to SDF** - Monochrome outline

## Implementation Plan

### Phase 6.1: Font Table Parsing (Week 1-2)

**Files to Create:**
- `src/nanovg_vk_emoji_tables.h` - Table parsing API
- `src/nanovg_vk_emoji_tables.c` - Implementation

**Table Parsers:**

#### SBIX Parser
```c
typedef struct VKNVGsbixStrike {
	uint16_t ppem;           // Pixels per em
	uint16_t resolution;     // DPI (usually 72)
	uint32_t* glyphOffsets;  // Offset array
	uint8_t* glyphData;      // PNG data
} VKNVGsbixStrike;

// Parse SBIX table
VKNVGsbixStrike* vknvg__parseSbixTable(uint8_t* fontData, uint32_t tableOffset);
// Select strike closest to target size
VKNVGsbixStrike* vknvg__selectSbixStrike(VKNVGsbixStrike* strikes, int count, float size);
// Extract PNG for glyph
uint8_t* vknvg__getSbixPNG(VKNVGsbixStrike* strike, uint32_t glyphID, uint32_t* length);
```

#### CBDT/CBLC Parser
```c
typedef struct VKNVGcbdtStrike {
	uint8_t ppemX, ppemY;
	uint8_t bitDepth;        // 32 for RGBA
	uint8_t flags;
	// Index subtables for glyph lookup
	void* indexSubtables;
} VKNVGcbdtStrike;

// Parse CBDT/CBLC tables
VKNVGcbdtStrike* vknvg__parseCbdtTable(uint8_t* fontData, uint32_t cblcOffset, uint32_t cbdtOffset);
// Get bitmap data (PNG or raw BGRA)
uint8_t* vknvg__getCbdtBitmap(VKNVGcbdtStrike* strike, uint32_t glyphID, uint32_t* length, uint8_t* format);
```

#### COLR/CPAL Parser
```c
typedef struct VKNVGcolrLayer {
	uint16_t glyphID;        // Outline glyph to render
	uint16_t paletteIndex;   // Color palette index
} VKNVGcolrLayer;

typedef struct VKNVGcolrGlyph {
	uint16_t numLayers;
	VKNVGcolrLayer* layers;
} VKNVGcolrGlyph;

typedef struct VKNVGcpal {
	uint32_t* colors;        // BGRA colors (sRGB)
	uint16_t numColors;
	uint16_t numPalettes;
} VKNVGcpal;

// Parse COLR table
VKNVGcolrGlyph* vknvg__parseColrGlyph(uint8_t* fontData, uint32_t tableOffset, uint32_t glyphID);
// Parse CPAL palette
VKNVGcpal* vknvg__parseCpalTable(uint8_t* fontData, uint32_t tableOffset);
```

### Phase 6.2: RGBA Atlas System (Week 2-3)

**Files to Create:**
- `src/nanovg_vk_color_atlas.h` - Color atlas API
- `src/nanovg_vk_color_atlas.c` - Implementation

**Features:**
- Separate RGBA8 texture atlas (VK_FORMAT_R8G8B8A8_UNORM)
- Reuse Guillotine packing from Phase 3
- Multi-atlas support (up to 8 RGBA atlases)
- Dynamic growth (512→1024→2048→4096)
- Same cache management as SDF atlas

**Structure:**
```c
typedef struct VKNVGcolorAtlas {
	// Vulkan resources
	VkImage* atlasImages;           // Array of RGBA8 images
	VkImageView* atlasViews;
	VkDeviceMemory* atlasMemory;

	// Atlas manager (reuse from Phase 3)
	VKNVGatlasManager* atlasManager;

	// Cache entries
	VKNVGglyphCacheEntry* glyphCache;
	uint32_t glyphCacheSize;

	// Staging buffer for RGBA uploads
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	void* stagingMapped;

	// Format detection cache
	uint8_t* fontFormats;  // Per-font format flags
} VKNVGcolorAtlas;

// API functions
VKNVGcolorAtlas* vknvg__createColorAtlas(VkDevice device, VkPhysicalDevice physicalDevice);
void vknvg__destroyColorAtlas(VKNVGcolorAtlas* atlas);

// Allocate space in RGBA atlas
int vknvg__allocColorGlyph(VKNVGcolorAtlas* atlas, uint32_t width, uint32_t height,
                           uint16_t* outAtlasX, uint16_t* outAtlasY, uint32_t* outAtlasIndex);

// Upload RGBA bitmap to atlas
void vknvg__uploadColorGlyph(VKNVGcolorAtlas* atlas, uint32_t atlasIndex,
                             uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             uint8_t* rgba);
```

### Phase 6.3: Bitmap Emoji Pipeline (Week 3-4)

**Files to Create:**
- `src/nanovg_vk_emoji_bitmap.h` - Bitmap emoji API
- `src/nanovg_vk_emoji_bitmap.c` - Implementation

**Pipeline:**
1. Detect glyph is emoji (check if glyph exists in SBIX/CBDT)
2. Select appropriate strike/size
3. Extract PNG or raw BGRA data
4. Decode PNG using libpng (if needed)
5. Allocate space in RGBA atlas
6. Upload RGBA bitmap to atlas
7. Cache coordinates for rendering

**PNG Decoding:**
```c
// Decode PNG to RGBA8
uint8_t* vknvg__decodePNG(uint8_t* pngData, uint32_t pngLength,
                          uint32_t* outWidth, uint32_t* outHeight);

// Convert BGRA to RGBA (for CBDT)
void vknvg__bgra2rgba(uint8_t* data, uint32_t width, uint32_t height);
```

**Integration:**
```c
// Check if glyph should use color atlas
int vknvg__isColorEmoji(FONScontext* fs, int fontID, uint32_t codepoint);

// Load bitmap emoji to color atlas
int vknvg__loadBitmapEmoji(VKNVGcolorAtlas* atlas, FONScontext* fs,
                           int fontID, uint32_t codepoint, float size,
                           VKNVGglyphCacheEntry* entry);
```

### Phase 6.4: Vector Emoji (COLRv0) (Week 4-5)

**Files to Create:**
- `src/nanovg_vk_emoji_colr.h` - COLR rendering API
- `src/nanovg_vk_emoji_colr.c` - Implementation

**Rendering Approach:**
1. Parse COLR layers for glyph
2. Rasterize each layer using existing outline rasterizer
3. Composite layers with palette colors
4. Upload final RGBA bitmap to color atlas

**Layer Composition:**
```c
// Rasterize COLR glyph to RGBA bitmap
uint8_t* vknvg__rasterizeColrGlyph(FONScontext* fs, int fontID,
                                    VKNVGcolrGlyph* colr, VKNVGcpal* cpal,
                                    float size, uint32_t* outWidth, uint32_t* outHeight);

// Composite single layer
void vknvg__compositeColrLayer(uint8_t* canvas, uint32_t width, uint32_t height,
                               uint8_t* layerBitmap, uint32_t layerW, uint32_t layerH,
                               uint32_t color);  // BGRA color from CPAL
```

**COLRv0 Algorithm:**
```
For each layer (bottom to top):
  1. Rasterize layer glyph outline to grayscale
  2. Get palette color (BGRA from CPAL)
  3. Convert grayscale to RGBA with palette color:
     rgba = (color.rgb, grayscale_alpha * color.a)
  4. Alpha blend onto canvas
```

### Phase 6.5: Shader Modifications (Week 5-6)

**Files to Modify:**
- `shaders/fill.frag` - Add RGBA atlas sampling path
- `shaders/text.frag` - Dual path: SDF vs RGBA

**Shader Changes:**

#### Dual Sampler Approach
```glsl
layout(binding = 0) uniform sampler2D texSDF;    // Existing SDF atlas
layout(binding = 1) uniform sampler2D texRGBA;   // New RGBA color atlas

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;
layout(location = 0) out vec4 outColor;

// Push constants or uniforms
layout(push_constant) uniform Constants {
	int useColorAtlas;   // 0 = SDF, 1 = RGBA
	int atlasIndex;      // Which atlas texture (for multi-atlas)
	// ... other params
} constants;

void main() {
	if (constants.useColorAtlas == 1) {
		// Color emoji path: direct RGBA sample
		vec4 color = texture(texRGBA, ftcoord);
		outColor = color;  // Pre-multiplied alpha
	} else {
		// Regular SDF path (existing code)
		float dist = texture(texSDF, ftcoord).r;
		// ... existing SDF rendering ...
	}
}
```

#### Text Effects on Color Emoji
Color emoji can still have effects:
- **Outline**: Detect edges, draw outline around opaque pixels
- **Glow**: Blur alpha channel, apply glow color
- **Shadow**: Offset + blur alpha channel
- **Gradients**: Not applicable (already colored)
- **Animations**: Apply color modulation

### Phase 6.6: Integration (Week 6-7)

**Tasks:**
1. Modify glyph lookup to check color atlas first
2. Add format detection at font load time
3. Update descriptor sets for dual texture binding
4. Modify rendering path selection (SDF vs RGBA)
5. Update cache statistics to track color vs SDF glyphs
6. Handle fallback when color format unavailable

**Unified API:**
```c
// No API changes! Automatic emoji detection
nvgText(vg, x, y, "Hello 👋 World 🌍", NULL);
// "Hello", "World" → SDF atlas
// 👋, 🌍 → RGBA atlas (if available)
```

**Format Detection:**
```c
typedef enum VKNVGemojiFormat {
	VKNVG_EMOJI_NONE = 0,     // No color emoji support
	VKNVG_EMOJI_SBIX = 1,     // Apple bitmap
	VKNVG_EMOJI_CBDT = 2,     // Google bitmap
	VKNVG_EMOJI_COLR = 3,     // Vector color
	VKNVG_EMOJI_SVG = 4,      // SVG (future)
} VKNVGemojiFormat;

// Detect available formats at font load
VKNVGemojiFormat vknvg__detectEmojiFormat(FONScontext* fs, int fontID);

// Cache per font
typedef struct VKNVGfontInfo {
	VKNVGemojiFormat emojiFormat;
	void* emojiData;  // Parsed SBIX/CBDT/COLR data
} VKNVGfontInfo;
```

### Phase 6.7: Testing (Week 7-8)

**Test Files:**
- `tests/test_emoji_tables.c` - Table parsing tests
- `tests/test_emoji_sbix.c` - SBIX bitmap tests
- `tests/test_emoji_cbdt.c` - CBDT bitmap tests
- `tests/test_emoji_colr.c` - COLR vector tests
- `tests/test_color_atlas.c` - RGBA atlas tests
- `tests/test_emoji_integration.c` - End-to-end tests

**Test Cases:**
1. Parse SBIX table from Apple Color Emoji
2. Parse CBDT/CBLC from Noto Color Emoji
3. Parse COLR/CPAL tables
4. PNG decoding (various sizes)
5. RGBA atlas allocation and upload
6. COLR layer composition
7. Format detection priority
8. Mixed text + emoji rendering
9. Emoji with text effects (outline, glow, shadow)
10. Multi-atlas overflow for emoji
11. LRU eviction with emoji cache
12. Performance: 1000 emoji glyphs

**Visual Tests:**
- Render emoji grid (all available emoji)
- Render family emoji (👨‍👩‍👧‍👦 - ZWJ sequence)
- Render flags (🇺🇸 - regional indicators)
- Render text with inline emoji
- Render emoji at various sizes (12px to 128px)

## Dependencies

### Required Libraries

1. **libpng** - PNG decoding for SBIX/CBDT
   ```bash
   apt-get install libpng-dev
   ```

2. **FreeType** (already dependency) - Has built-in SFNT table access

### Optional Libraries

1. **stb_image.h** - Alternative to libpng (single-header)
2. **NanoSVG** - If adding SVG-in-OpenType support later

## Performance Considerations

### Memory Usage

**SDF Atlas:**
- Single channel (R8): 4MB per 4096×4096 atlas
- 8 atlases max: 32MB

**RGBA Color Atlas:**
- Four channels (RGBA8): 16MB per 4096×4096 atlas
- 8 atlases max: 128MB
- **Total max**: 160MB for both systems

**Bitmap Size Examples:**
- Apple Color Emoji @ 160px: ~8-20KB per emoji (PNG compressed)
- Noto Color Emoji @ 128px: ~2-10KB per emoji (CBDT)
- Cache 1000 emoji: ~5-15MB decompressed RGBA

### Rendering Performance

**Bitmap Emoji (SBIX/CBDT):**
- Cache hit: Same as SDF (~100-200ns lookup)
- Cache miss: PNG decode (~0.5-2ms) + upload (~100µs)
- GPU render: Direct texture sample (very fast)

**Vector Emoji (COLR):**
- Cache hit: Same as bitmap
- Cache miss: Multi-layer rasterize (~2-10ms depending on layers) + upload
- More layers = slower, but scales to any size
- Typical emoji: 5-15 layers

**Comparison:**
- Bitmap: Fast, but memory-heavy and size-limited
- COLR: Slower rasterization, but scalable and compact

### Optimization Strategies

1. **Prewarming**: Load common emoji at startup
2. **Size Bucketing**: Cache multiple sizes (16, 24, 32, 48, 64px)
3. **Lazy Loading**: Only load emoji when first used
4. **Async Rasterization**: Background thread for COLR composition
5. **Mipmaps**: Generate mipmaps for emoji at different scales

## Estimated Effort

- **Week 1-2**: Font table parsing (SBIX, CBDT, COLR) - 40 hours
- **Week 2-3**: RGBA atlas system - 30 hours
- **Week 3-4**: Bitmap emoji pipeline (PNG decode) - 35 hours
- **Week 4-5**: COLR vector emoji - 40 hours
- **Week 5-6**: Shader modifications - 25 hours
- **Week 6-7**: Integration and format detection - 30 hours
- **Week 7-8**: Testing and validation - 30 hours

**Total**: ~230 hours (8 weeks at 30 hours/week)

## Success Criteria

- [ ] SBIX table parsing and PNG extraction
- [ ] CBDT/CBLC table parsing
- [ ] COLR/CPAL table parsing and palette lookup
- [ ] RGBA atlas creation and management
- [ ] PNG decoding (libpng or stb_image)
- [ ] COLR layer composition
- [ ] Dual shader path (SDF vs RGBA)
- [ ] Automatic format detection
- [ ] Mixed text + emoji rendering
- [ ] Family emoji (👨‍👩‍👧) renders correctly
- [ ] All tests passing (50+ tests)
- [ ] Performance: <5ms for 1000 emoji glyphs

## Deliverables

1. Font table parsers (SBIX, CBDT/CBLC, COLR/CPAL)
2. RGBA color atlas system
3. Bitmap emoji pipeline (PNG decode + upload)
4. Vector emoji pipeline (COLR composition)
5. Modified shaders for dual atlas
6. Integration with existing text rendering
7. Test suite (50+ tests, 8 test files)
8. Documentation updates
9. Example code showing emoji rendering

## Future Enhancements (Phase 6+)

### Phase 6.5: COLRv1 Support
- Gradient fills (linear, radial, sweep)
- Transformations and affine transforms
- Compositing modes (multiply, screen, overlay)
- Paint tables and reusable elements

### Phase 6.6: SVG-in-OpenType
- SVG parser integration (NanoSVG)
- Rasterization to RGBA bitmap
- Animation support (if needed)

### Phase 6.7: Animated Emoji
- APNG support (animated PNG in SBIX)
- Frame sequencing
- Animation playback

### Phase 6.8: Emoji Modifiers
- Skin tone modifiers (🏻🏼🏽🏾🏿)
- Hair style modifiers
- Gender modifiers (♂️♀️)

## Example Usage

```c
#include "nanovg.h"
#include "nanovg_vk.h"

// Create context (same as before)
NVGcontext* vg = nvgCreateVk(params);

// Load emoji font
int emojiFont = nvgCreateFont(vg, "emoji", "AppleColorEmoji.ttf");
// Or: int emojiFont = nvgCreateFont(vg, "emoji", "NotoColorEmoji.ttf");

// Automatic emoji detection - no API changes!
nvgBeginFrame(vg, width, height, pixelRatio);
nvgFontFace(vg, "emoji");
nvgFontSize(vg, 48.0f);

// Mix text and emoji seamlessly
nvgText(vg, 100, 100, "Hello 👋", NULL);
nvgText(vg, 100, 150, "World 🌍", NULL);
nvgText(vg, 100, 200, "Family 👨‍👩‍👧", NULL);  // ZWJ sequence

// Emoji with effects (Phase 5 integration)
VKNVGtextEffect* effect = vknvg__createTextEffect();
vknvg__addOutline(effect, 2.0f, nvgRGBA(0, 0, 0, 255));
vknvg__setGlow(effect, 10.0f, nvgRGBA(255, 200, 0, 128), 0.8f);

nvgText(vg, 100, 250, "Glowing 🔥", NULL);

vknvg__destroyTextEffect(effect);
nvgEndFrame(vg);
```

---

**Status**: Ready to begin implementation
**Target Completion**: 8 weeks from start
**Dependencies**: Phase 1-5 complete ✓, libpng library
