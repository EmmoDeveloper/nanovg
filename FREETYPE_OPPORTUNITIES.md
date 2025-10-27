# FreeType Advanced Features - Utilization Opportunities

## Current Usage (Minimal)

We're currently using only the **basics** of FreeType:

```c
// What we use now:
FT_Init_FreeType()
FT_New_Face() / FT_New_Memory_Face()
FT_Set_Pixel_Sizes()
FT_Load_Glyph()
FT_Render_Glyph()
FT_Get_Char_Index()
FT_Get_Advance()
FT_Get_Kerning()
FT_Outline_Get_CBox()  // Only in fontstash
```

**Render modes used**: Only `FT_RENDER_MODE_NORMAL` (8-bit grayscale)

This is ~10% of FreeType's capabilities.

---

## High-Impact Features We're Missing

### 1. **Subpixel Rendering (LCD/ClearType)** - `ftlcdfil.h`

**What it does**: 3x horizontal resolution using RGB subpixels
**Impact**: Dramatically sharper text on LCD screens
**Complexity**: Low - just change render mode

```c
// Enable subpixel rendering
FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT);
FT_Load_Glyph(face, glyph_index, FT_LOAD_TARGET_LCD);
FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);

// Result: 3x wider bitmap with RGB channels
// bitmap.pixel_mode == FT_PIXEL_MODE_LCD
// bitmap.width *= 3  (each pixel = R,G,B subpixel)
```

**Implementation**:
- Update texture format from `R8` to `RGB8`
- Modify shader to sample RGB channels separately
- Add API flag: `NVG_LCD_RENDERING`

**Benefit**: Professional-quality text rendering matching system fonts

---

### 2. **FreeType Cache Subsystem** - `ftcache.h`

**What it does**: Built-in glyph caching with LRU eviction
**Impact**: Replace our custom atlas code with battle-tested cache
**Complexity**: Medium - refactor font system

```c
// FreeType's own cache (replaces our virtual atlas)
FTC_Manager manager;
FTC_CMapCache cmapCache;
FTC_ImageCache imageCache;
FTC_SBitCache sbitCache;  // Small bitmap cache (most efficient)

// Usage:
FTC_Manager_New(ftLibrary, 0, 0, 0, face_requester, NULL, &manager);
FTC_SBitCache_New(manager, &sbitCache);

// Lookup glyph (auto-caches)
FTC_SBit sbit;
FTC_SBitCache_Lookup(sbitCache, &scaler, glyph_index, &sbit, NULL);
// sbit->buffer = bitmap data (no copy needed!)
```

**Why this is better than our code**:
- FreeType experts maintain it
- Handles memory limits automatically
- Face/size management included
- Optimized for small bitmaps (8-bit metrics)
- Thread-safe option available

**Benefit**: Simpler, more robust caching with less code

---

### 3. **Glyph Stroker (Outline/Border Effects)** - `ftstroke.h`

**What it does**: Generate outlined/stroked glyphs
**Impact**: Text effects without shaders
**Complexity**: Low - just process outline before render

```c
// Create stroker
FT_Stroker stroker;
FT_Stroker_New(ftLibrary, &stroker);
FT_Stroker_Set(stroker,
    64 * border_width,           // Thickness in 26.6 format
    FT_STROKER_LINECAP_ROUND,
    FT_STROKER_LINEJOIN_ROUND,
    0);

// Load glyph as outline
FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);

// Stroke it
FT_Glyph glyph;
FT_Get_Glyph(face->glyph, &glyph);
FT_Glyph_Stroke(&glyph, stroker, 0);  // Modifies outline

// Now render
FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
```

**Effects possible**:
- Outlined text (border around letters)
- Inside/outside borders separately
- Variable stroke width
- Different join/cap styles

**Benefit**: Text effects CPU-side, no shader complexity

---

### 4. **Color Glyph Support (Emoji)** - `ftcolor.h`

**What it does**: Access CPAL/COLR tables for color emoji
**Impact**: Native emoji support
**Complexity**: Medium - new texture format

```c
// Check if font has color
FT_Bool has_color = FT_HAS_COLOR(face);

// Get palette
FT_Palette_Data palette_data;
FT_Palette_Data_Get(face, &palette_data);

// Load colored glyph
FT_Load_Glyph(face, glyph_index, FT_LOAD_COLOR);

// Result: bitmap.pixel_mode == FT_PIXEL_MODE_BGRA
// Full RGBA bitmap for emoji
```

**Integration**:
- Detect color glyphs
- Use RGBA texture instead of ALPHA
- No SDF processing for color glyphs
- Fallback to grayscale if no color data

**Benefit**: Native emoji rendering (Apple/Google/Microsoft emoji fonts)

---

### 5. **Outline Processing & Decomposition** - `ftoutln.h`

**What it does**: Direct access to glyph vector outlines
**Impact**: GPU tessellation, custom effects
**Complexity**: High - requires tessellation

```c
// Load outline (no rasterization)
FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
FT_Outline* outline = &face->glyph->outline;

// Decompose into path commands
FT_Outline_Funcs callbacks = {
    .move_to = my_move_to,
    .line_to = my_line_to,
    .conic_to = my_conic_to,  // Quadratic bezier
    .cubic_to = my_cubic_to,  // Cubic bezier
};
FT_Outline_Decompose(outline, &callbacks, user_data);

// Or transform it
FT_Matrix matrix = {.xx = 0x10000, .yy = 0x10000};
FT_Outline_Transform(outline, &matrix);

// Or embolden (fake bold)
FT_Outline_Embolden(outline, strength);
```

**Use cases**:
- GPU-based tessellation (compute shader)
- Path-based text rendering
- Custom glyph transformations
- Fake bold/italic
- Glyph morphing/animation

**Benefit**: Vector glyph data for advanced rendering

---

### 6. **Hinting Control & Loading Options**

**What we're missing**: Fine-tuned hinting control

```c
// Current: FT_LOAD_DEFAULT (auto-hinting)
// Available load flags:

FT_LOAD_NO_HINTING      // Faster, less sharp
FT_LOAD_FORCE_AUTOHINT  // FreeType's hinter (best for small sizes)
FT_LOAD_NO_AUTOHINT     // Use font's native hints only

// Rendering targets (affects hinting):
FT_LOAD_TARGET_NORMAL   // Standard
FT_LOAD_TARGET_LIGHT    // Lighter hinting
FT_LOAD_TARGET_MONO     // 1-bit monochrome (fastest)
FT_LOAD_TARGET_LCD      // Subpixel rendering
FT_LOAD_TARGET_LCD_V    // Vertical subpixels

// Rendering modes:
FT_RENDER_MODE_NORMAL   // 8-bit grayscale (current)
FT_RENDER_MODE_LIGHT    // Lighter rendering
FT_RENDER_MODE_MONO     // 1-bit (8x faster rasterization)
FT_RENDER_MODE_LCD      // Subpixel horizontal
FT_RENDER_MODE_LCD_V    // Subpixel vertical
```

**Optimization**:
- Use `FT_RENDER_MODE_MONO` for pixelated/retro styles
- Use `FT_LOAD_TARGET_LIGHT` for large text (less hinting needed)
- Cache multiple render modes per glyph

**Benefit**: Performance tuning + quality control

---

## Feature Comparison

| Feature | Current | With FreeType | Benefit |
|---------|---------|---------------|---------|
| **Render Quality** | Grayscale | Subpixel LCD | 3x sharper |
| **Caching** | Custom atlas | FTC_SBitCache | Less code, more robust |
| **Effects** | Shader-only | Stroker API | CPU outline, GPU SDF |
| **Emoji** | None | CPAL/COLR | Native color emoji |
| **Outlines** | None | FT_Outline | Vector data for GPU |
| **Hinting** | Auto | Full control | Performance tuning |

---

## Implementation Priority

### Phase 1: Quick Wins (Low Complexity, High Impact)
1. **Subpixel Rendering** - 1-2 days
   - Add LCD render mode
   - Update texture format R8 → RGB8
   - Modify text shader

2. **Load Flag Optimization** - 1 day
   - Add hinting controls to API
   - Test different render modes
   - Document performance characteristics

### Phase 2: Architecture Improvements (Medium Complexity)
3. **FreeType Cache Subsystem** - 3-5 days
   - Replace custom atlas with FTC
   - Simplify font management code
   - Benchmark performance

4. **Glyph Stroker** - 2-3 days
   - Add outline/border API
   - Integrate with existing rendering
   - Add stroke width parameter

### Phase 3: Advanced Features (High Complexity)
5. **Color Glyph Support** - 5-7 days
   - RGBA texture atlas
   - Color glyph detection
   - Dual-mode rendering (gray + color)

6. **Outline Decomposition** - 7-10 days
   - Expose vector outline data
   - GPU tessellation pipeline
   - Custom path effects

---

## Code Samples

### Subpixel Rendering Integration

```c
// In nvg_font.c
void nvgfont_set_render_mode(NVGFont* font, int mode) {
    font->render_mode = mode;  // NORMAL, LIGHT, MONO, LCD, LCD_V
}

NVGGlyph* nvgfont_get_glyph(NVGFont* font, uint32_t codepoint) {
    // ... existing code ...

    FT_Int32 load_flags = FT_LOAD_DEFAULT;
    FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;

    switch (font->render_mode) {
        case NVG_RENDER_LCD:
            load_flags = FT_LOAD_TARGET_LCD;
            render_mode = FT_RENDER_MODE_LCD;
            break;
        case NVG_RENDER_LIGHT:
            load_flags = FT_LOAD_TARGET_LIGHT;
            break;
        case NVG_RENDER_MONO:
            load_flags = FT_LOAD_TARGET_MONO;
            render_mode = FT_RENDER_MODE_MONO;
            break;
    }

    FT_Load_Glyph(font->face, glyphIndex, load_flags);
    FT_Render_Glyph(font->face->glyph, render_mode);

    // Handle different pixel modes
    FT_Bitmap* bitmap = &font->face->glyph->bitmap;
    switch (bitmap->pixel_mode) {
        case FT_PIXEL_MODE_GRAY:
            // Current code path
            break;
        case FT_PIXEL_MODE_LCD:
            // Width is 3x, RGB subpixels
            glyph->width = bitmap->width / 3;
            // Upload as RGB texture
            break;
        case FT_PIXEL_MODE_MONO:
            // 1-bit, need to expand to 8-bit
            break;
    }
}
```

### FreeType Cache Integration

```c
// Replace entire virtual atlas system
typedef struct NVGFontSystem {
    FTC_Manager cache_manager;
    FTC_CMapCache cmap_cache;
    FTC_SBitCache sbit_cache;
} NVGFontSystem;

// Init (replaces atlas creation)
FTC_Manager_New(ftLibrary,
    0,                    // max faces (0 = unlimited)
    0,                    // max sizes (0 = unlimited)
    MAX_CACHE_BYTES,      // max memory
    face_requester,       // callback to load faces
    NULL,                 // user data
    &sys->cache_manager);

// Get glyph (replaces our lookup + rasterize)
FTC_SBit sbit;
FTC_ImageTypeRec type = {
    .face_id = font_id,
    .width = 0,
    .height = pixel_size,
    .flags = FT_LOAD_DEFAULT
};
FTC_SBitCache_Lookup(sys->sbit_cache, &type, glyph_index, &sbit, NULL);

// Use directly (no copy!)
upload_to_gpu(sbit->buffer, sbit->width, sbit->height);
```

---

## Recommendation

**Start with Subpixel Rendering** - it's the easiest win with the biggest visual impact. Modern displays are all LCD, and users expect ClearType-quality text.

Then consider FreeType Cache if atlas management is causing issues. The stroker API is great for effects without shader complexity.