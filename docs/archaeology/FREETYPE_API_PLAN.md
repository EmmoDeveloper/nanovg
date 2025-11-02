# Plan: Expose FreeType Features in NanoVG

## Current State

### What NanoVG Currently Provides
- `nvgText(x, y, string)` - Simple text rendering at position
- `nvgTextAlign()` - Basic alignment (left/center/right, top/middle/bottom/baseline)
- `nvgTextLetterSpacing()` - Letter spacing adjustment
- `nvgTextLineHeight()` - Line height control
- `nvgTextBox()` - Word-wrapped text
- `nvgTextGlyphPositions()` - Query glyph positions after layout

### What FreeType Already Has (Internal)
- Kerning support (FT_Get_Kerning) - implemented in nvg_freetype.c:828-836, 931-935
- Glyph metrics (bearing, advance) - available but not exposed
- HarfBuzz text shaping - implemented in nvg_freetype.c
- Multiple render modes (mono, grayscale, LCD) - partially exposed via NVGFTRenderMode

### What's Missing
1. Per-glyph control (load/render individual glyphs)
2. Access to detailed glyph metrics
3. Kerning control (enable/disable, query values)
4. Subpixel positioning (currently rounds to pixels)
5. Direct glyph index rendering (bypass shaping)
6. Fine-grained baseline control

## Proposed API Design

### 1. Glyph-Level API

Add to nanovg.h:

```c
// Glyph metrics structure
typedef struct NVGglyphMetrics {
	float bearingX, bearingY;    // Glyph placement offset from baseline
	float advanceX, advanceY;    // Cursor movement after glyph
	float width, height;         // Bitmap dimensions
	int glyphIndex;              // FreeType glyph index
} NVGglyphMetrics;

// Get metrics for a single codepoint
int nvgGetGlyphMetrics(NVGcontext* ctx, unsigned int codepoint, NVGglyphMetrics* metrics);

// Render a single glyph at exact position
void nvgRenderGlyph(NVGcontext* ctx, unsigned int codepoint, float x, float y);

// Render glyph by FreeType glyph index (bypass Unicode mapping)
void nvgRenderGlyphIndex(NVGcontext* ctx, int glyphIndex, float x, float y);
```

### 2. Kerning Control

Add to nanovg.h:

```c
// Query kerning between two codepoints (in pixels)
float nvgGetKerning(NVGcontext* ctx, unsigned int left, unsigned int right);

// Enable/disable automatic kerning (default: enabled)
void nvgKerning(NVGcontext* ctx, int enabled);
```

### 3. Subpixel Positioning

Add to nanovg.h:

```c
// Enable subpixel positioning (1/64th pixel precision)
// When enabled, glyph positions are not rounded to integers
void nvgSubpixelPositioning(NVGcontext* ctx, int enabled);
```

### 4. Enhanced Text Iteration

Add to nanovg.h:

```c
// Extended glyph position with metrics
typedef struct NVGglyphPositionEx {
	const char* str;             // Position in text
	float x;                     // Glyph x-position
	float y;                     // Glyph y-position (includes baseline offset)
	float minx, maxx;            // Glyph bounds
	float miny, maxy;
	float advanceX, advanceY;    // How much cursor moved
	float bearingX, bearingY;    // Glyph bearing
	int glyphIndex;              // FreeType glyph index
	unsigned int codepoint;      // Unicode codepoint
} NVGglyphPositionEx;

// Get detailed glyph positions with full metrics
int nvgTextGlyphPositionsEx(NVGcontext* ctx, float x, float y,
                             const char* string, const char* end,
                             NVGglyphPositionEx* positions, int maxPositions);
```

### 5. Baseline Control

Add to nanovg.h:

```c
// Get font baseline metrics
void nvgFontBaselines(NVGcontext* ctx, float* ascender, float* descender);

// Adjust baseline offset (for mixing fonts of different sizes)
void nvgBaselineOffset(NVGcontext* ctx, float offset);
```

## Implementation Plan

### Phase 1: Internal nvg_freetype.h Extensions
- Add `nvgft_get_glyph_metrics()` to query glyph info
- Add `nvgft_get_kerning()` to expose FT_Get_Kerning
- Add `nvgft_set_kerning_enabled()` flag
- Add `nvgft_render_glyph_index()` for direct glyph rendering

### Phase 2: NanoVG API Integration
- Add new functions to nanovg.h
- Wire through to nvg_freetype.h functions
- Add state tracking for new flags (kerning enabled, subpixel mode)
- Update nanovg.c to call new functions

### Phase 3: Vulkan Backend Support
- Ensure nvg_vk.c handles subpixel texture coordinates
- Test that glyph-level rendering works with atlas
- Verify no performance regression

### Phase 4: Testing
- Create test_glyph_control.c to test per-glyph rendering
- Create test_kerning.c to verify kerning query/control
- Create test_subpixel.c for subpixel positioning
- Update test_braille with precise positioning

## Design Decisions

### Keep vs Replace Current API
**Decision:** Keep existing `nvgText()` API, add new APIs alongside it
- Existing code continues to work
- New features are opt-in
- Simple cases stay simple

### Kerning Default
**Decision:** Keep kerning enabled by default
- Current behavior already uses kerning
- Users can disable if needed for monospace layout

### Subpixel Positioning
**Decision:** Default to pixel rounding (current behavior)
- Subpixel is opt-in for users who need precision
- Avoids breaking existing layouts

### API Naming
**Decision:** Use `nvg` prefix for all public APIs
- Consistent with existing NanoVG style
- Clear separation from internal `nvgft_` functions

## Success Criteria

1. Can render individual glyphs at precise positions
2. Can query kerning values between any glyph pair
3. Can disable kerning for monospace/grid layouts
4. Can position glyphs with subpixel precision
5. Can access full glyph metrics (bearing, advance)
6. Existing NanoVG code continues to work unchanged
7. No performance regression in standard use cases

## Future Extensions

- Glyph substitution (ligatures, variants)
- OpenType feature control (small caps, old-style figures)
- Vertical text layout
- Multi-font fallback control
- Custom shaping callbacks
