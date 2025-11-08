# Coordinate Systems Investigation

## Purpose
Document the coordinate conventions of all libraries used in the font system to ensure correct Y-axis orientation for Vulkan rendering.

---

## Vulkan (Target System)

### Coordinate Convention
- **Origin**: Top-left corner (0, 0)
- **Y-axis**: Points DOWN (increasing Y goes down)
- **Framebuffer**: `(0,0)` at top-left, `(width-1, height-1)` at bottom-right

### NDC (Normalized Device Coordinates)
- X: -1 (left) to +1 (right)
- Y: -1 (top) to +1 (bottom)  ← **Note**: Y=-1 is TOP, not bottom
- Z: 0 (near) to 1 (far)

### Implications
- Texture coordinates: `(0,0)` at top-left, `(1,1)` at bottom-right
- Vertex positions must use top-left origin with Y-down

---

## FreeType

### Glyph Coordinate Convention
- **Origin**: Bottom-left corner of glyph bitmap
- **Y-axis**: Points UP (increasing Y goes up)
- **Baseline**: Y=0 for most glyphs

### Glyph Metrics

```c
FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
FT_GlyphSlot slot = face->glyph;

// Metrics (all in pixels if FT_LOAD_TARGET_NORMAL)
int width = slot->bitmap.width;
int height = slot->bitmap.rows;
int bearingX = slot->bitmap_left;    // Horizontal bearing (left side)
int bearingY = slot->bitmap_top;     // Vertical bearing (TOP of glyph above baseline)
int advanceX = slot->advance.x >> 6; // Advance to next glyph (26.6 fixed point)
```

### Bitmap Layout
```
FT_Bitmap bitmap = slot->bitmap;
unsigned char* buffer = bitmap.buffer;

// Layout: bottom-left origin, rows from bottom to top
// buffer[0] = bottom-left pixel
// buffer[width-1] = bottom-right pixel
// buffer[(rows-1)*pitch] = top-left pixel
```

### Y-Axis Issues

1. **bearingY is UPWARD from baseline**
   - Positive `bearingY` = glyph extends above baseline
   - To get top-left Y in Vulkan: `y_vulkan = baseline_y - bearingY`

2. **Bitmap is vertically flipped**
   - Row 0 in FreeType = bottom row in Vulkan
   - **Solution A**: Flip bitmap while copying to atlas
   - **Solution B**: Flip texture coordinates (t0 ↔ t1)

3. **Example Conversion**:
   ```c
   // NanoVG text call: nvgText(vg, x, y, "Hello")
   // y = baseline position in screen space (top-left origin)

   // Get glyph from FreeType
   FT_Load_Char(face, 'H', FT_LOAD_RENDER);
   int bearingX = face->glyph->bitmap_left;
   int bearingY = face->glyph->bitmap_top;

   // Quad vertices (Vulkan coordinates)
   float x0 = x + bearingX;
   float y0 = y - bearingY;  // ← SUBTRACT because FreeType bearingY is upward
   float x1 = x0 + width;
   float y1 = y0 + height;
   ```

### Verification Test
```c
// Render uppercase 'H' at baseline y=100
// Expected: top of 'H' around y=50, bottom at y=100
// If top is at y=150, Y-axis is inverted!
```

---

## Cairo

### Surface Coordinate Convention
- **Origin**: Top-left corner (0, 0)
- **Y-axis**: Points DOWN (increasing Y goes down)
- **Matches Vulkan**: ✅

### Image Surface
```c
cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
unsigned char* data = cairo_image_surface_get_data(surface);
int stride = cairo_image_surface_get_stride(surface);

// Layout: top-left origin, rows from top to bottom
// data[0] = top-left pixel
// data[stride] = second row, leftmost pixel
```

### COLR Emoji Rendering
```c
// Paint glyphs with cairo_show_glyphs()
cairo_glyph_t glyph;
glyph.index = glyph_id;
glyph.x = x;  // Left edge
glyph.y = y;  // Baseline (NOT top-left)

cairo_show_glyphs(cr, &glyph, 1);
```

### Y-Axis Issues

1. **Baseline Positioning**
   - Cairo places glyphs relative to baseline (like FreeType)
   - But uses top-down Y-axis
   - To position at baseline: `cairo_move_to(cr, x, baseline_y)`

2. **Surface Data**
   - Already top-down, can copy directly to Vulkan texture ✅

3. **Example Conversion**:
   ```c
   // Create surface
   cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
   cairo_t* cr = cairo_create(surf);

   // Set font and size
   cairo_set_font_face(cr, font_face);
   cairo_set_font_size(cr, 48);

   // Position at baseline (origin at top-left of surface)
   // If glyph ascender = 40, place baseline at y=40
   cairo_move_to(cr, 0, 40);
   cairo_show_glyphs(cr, &glyph, 1);

   // Surface data can be copied directly to Vulkan atlas
   unsigned char* pixels = cairo_image_surface_get_data(surf);
   // pixels[0] = top-left, already in Vulkan orientation ✅
   ```

### Verification Test
```c
// Render emoji to 64x64 surface with baseline at y=50
// Top pixels (y=0-40) should have glyph content
// If content appears at bottom (y=50-64), Y-axis is wrong!
```

---

## HarfBuzz

### Coordinate Convention
- **Relative Positioning**: HarfBuzz returns glyph advances and offsets, not absolute positions
- **Coordinate Agnostic**: Works with any backend coordinate system
- **Y-axis**: Depends on font (usually matches FreeType: baseline at Y=0, upward positive)

### Shaping Output
```c
hb_buffer_t* buf = hb_buffer_create();
hb_buffer_add_utf8(buf, text, -1, 0, -1);
hb_shape(font, buf, NULL, 0);

unsigned int glyph_count;
hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &glyph_count);
hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

for (int i = 0; i < glyph_count; i++) {
	uint32_t glyph_id = info[i].codepoint;  // Glyph ID (not Unicode)

	// Offsets are deltas from current pen position (26.6 fixed point)
	int32_t x_offset = pos[i].x_offset >> 6;  // Horizontal offset
	int32_t y_offset = pos[i].y_offset >> 6;  // Vertical offset

	// Advances move pen position (26.6 fixed point)
	int32_t x_advance = pos[i].x_advance >> 6;
	int32_t y_advance = pos[i].y_advance >> 6;
}
```

### Y-Axis Issues

1. **Offsets are Deltas**
   - `y_offset` is relative to baseline
   - Positive usually means "shift up" in font's coordinate system
   - If font is FreeType-backed (Y-up), need to negate for Vulkan: `y_vulkan = pen_y - y_offset`

2. **Advances**
   - `y_advance` is usually 0 for horizontal text
   - For vertical text, need to check convention

3. **Example Conversion**:
   ```c
   // Start position (Vulkan coordinates, top-left origin)
   float pen_x = 100;
   float pen_y = 200;  // Baseline position

   for (int i = 0; i < glyph_count; i++) {
   	// Apply offset (NOTE: y_offset negated for Vulkan)
   	float glyph_x = pen_x + (pos[i].x_offset >> 6);
   	float glyph_y = pen_y - (pos[i].y_offset >> 6);  // ← SUBTRACT for Vulkan

   	// Get glyph from FreeType at this position
   	render_glyph(info[i].codepoint, glyph_x, glyph_y);

   	// Advance pen
   	pen_x += (pos[i].x_advance >> 6);
   	pen_y += (pos[i].y_advance >> 6);  // Usually 0 for horizontal text
   }
   ```

### Verification Test
```c
// Shape Arabic text "مرحبا" (right-to-left)
// With HarfBuzz: glyphs should be reordered right-to-left
// Check: first glyph x_advance should be negative OR
//        glyphs should be iterated in visual order
```

---

## FriBidi

### Coordinate Convention
- **Pure Reordering**: FriBidi only reorders character indices, no coordinates
- **Coordinate Agnostic**: Works with any coordinate system
- **No Y-axis concerns**: Only handles character-level reordering

### Reordering Output
```c
FriBidiChar* visual_str = malloc(len * sizeof(FriBidiChar));
FriBidiCharType* types = malloc(len * sizeof(FriBidiCharType));
FriBidiLevel* levels = malloc(len * sizeof(FriBidiLevel));

fribidi_get_bidi_types(logical_str, len, types);
fribidi_get_par_direction(types, len, &base_dir);

FriBidiParType par_type = FRIBIDI_PAR_ON;
fribidi_shape(FRIBIDI_FLAGS_DEFAULT, levels, len, &par_type, logical_str);
fribidi_reorder_line(FRIBIDI_FLAGS_DEFAULT, types, len, 0, par_type, levels, visual_str, NULL);

// visual_str now contains characters in visual (display) order
// No coordinates involved
```

### No Y-axis Issues
- FriBidi works at character level before shaping
- Pass reordered characters to HarfBuzz
- Y-axis handling is in HarfBuzz/FreeType stage

---

## NanoVG

### Coordinate Convention
- **Origin**: Top-left corner (0, 0)
- **Y-axis**: Points DOWN (increasing Y goes down)
- **Matches Vulkan**: ✅

### Text Positioning
```c
nvgText(vg, x, y, "Hello", NULL);
// x = left edge (or right edge if right-aligned)
// y = BASELINE position (not top-left)
```

### Alignment
```c
nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);  // Default
nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);       // y = top edge
nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);    // y = middle
nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);    // y = bottom edge
```

### Y-Axis Issues

1. **Baseline Positioning**
   - When align is `BASELINE` (default), y parameter is baseline
   - Top of glyph = `y - ascender`
   - Bottom of glyph = `y + descender`

2. **Integration**:
   ```c
   // User calls
   nvgText(vg, 100, 200, "Hello", NULL);

   // Internally:
   float baseline_y = 200;

   // Get font metrics
   float ascender = font_ascender * scale;
   float descender = font_descender * scale;

   // For each glyph from FreeType:
   float y0 = baseline_y - bearingY;  // Top edge
   float y1 = y0 + height;             // Bottom edge
   ```

---

## Summary of Y-Axis Conversions

### Vulkan Target (Top-left origin, Y-down)

| Library | Origin | Y-axis | Conversion Needed |
|---------|--------|--------|-------------------|
| **Vulkan** | Top-left | DOWN ↓ | - (target system) |
| **FreeType Metrics** | Baseline | UP ↑ | `y_vulkan = baseline - bearingY` |
| **FreeType Bitmap** | Bottom-left | UP ↑ | Flip rows OR flip texture coords |
| **Cairo** | Top-left | DOWN ↓ | None (matches) ✅ |
| **HarfBuzz** | N/A (deltas) | UP ↑ (font convention) | `y_vulkan = pen_y - y_offset` |
| **FriBidi** | N/A (reordering) | N/A | None (no coordinates) |
| **NanoVG** | Top-left | DOWN ↓ | None (matches) ✅ |

---

## Implementation Guidelines

### 1. FreeType Glyph Rasterization
```c
FT_Load_Char(face, codepoint, FT_LOAD_RENDER);
FT_GlyphSlot slot = face->glyph;

// Extract metrics
int width = slot->bitmap.width;
int height = slot->bitmap.rows;
int bearingX = slot->bitmap_left;
int bearingY = slot->bitmap_top;  // Distance ABOVE baseline

// Copy bitmap to atlas WITH VERTICAL FLIP
unsigned char* src = slot->bitmap.buffer;
unsigned char* dst = atlas_data + (atlas_y * atlas_width + atlas_x);

for (int y = 0; y < height; y++) {
	// Flip: FreeType row 0 → Vulkan row (height-1)
	int src_row = height - 1 - y;
	memcpy(dst + y * atlas_width, src + src_row * slot->bitmap.pitch, width);
}

// Store metrics (bearingY is already correct for Vulkan)
glyph->bearingX = bearingX;
glyph->bearingY = bearingY;  // Used as: y_top = baseline - bearingY
```

### 2. Cairo COLR Emoji
```c
cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
cairo_t* cr = cairo_create(surf);

// Position at baseline (ascender pixels from top)
int ascender = font_ascender * scale;
cairo_move_to(cr, 0, ascender);

// Render glyph
cairo_glyph_t glyph = { .index = glyph_id, .x = 0, .y = ascender };
cairo_show_glyphs(cr, &glyph, 1);

// Copy surface data directly (already top-down)
unsigned char* pixels = cairo_image_surface_get_data(surf);
memcpy(atlas_data + offset, pixels, size * size * 4);  // No flip needed ✅
```

### 3. HarfBuzz Shaped Text
```c
// Shape text
hb_shape(hb_font, buffer, NULL, 0);
hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &count);

float pen_x = start_x;
float pen_y = baseline_y;  // Baseline in Vulkan coordinates

for (int i = 0; i < count; i++) {
	// Apply offset (NEGATE y_offset for Vulkan)
	float glyph_x = pen_x + (pos[i].x_offset / 64.0f);
	float glyph_y = pen_y - (pos[i].y_offset / 64.0f);  // ← SUBTRACT

	// Render glyph at (glyph_x, glyph_y)
	// glyph_y is baseline position
	// Top of quad = glyph_y - bearingY

	// Advance pen
	pen_x += (pos[i].x_advance / 64.0f);
	pen_y += (pos[i].y_advance / 64.0f);
}
```

---

## Testing Strategy

### Visual Verification Tests

1. **ASCII Baseline Test**
   ```
   Render: "Hpg" at y=100
   Expected:
   - 'H' top at ~y=50 (ascender)
   - 'p' bottom at ~y=120 (descender)
   - Baseline visible at y=100
   ```

2. **CJK Glyph Test**
   ```
   Render: "中" (Chinese character) at y=200
   Expected:
   - Centered around y=200
   - Not upside-down
   ```

3. **Emoji Test**
   ```
   Render: "😀" (COLR emoji) at y=150
   Expected:
   - Proper orientation (not flipped)
   - Baseline at y=150
   ```

4. **Complex Script Test**
   ```
   Render: "مرحبا" (Arabic, RTL)
   Expected:
   - Characters right-to-left
   - Proper vertical alignment
   - Diacritics above letters, not below
   ```

### Debug Visualization
```c
// Draw baseline reference line
draw_line(0, baseline_y, width, baseline_y, RED);

// Draw bounding boxes
for each glyph:
	draw_rect(x0, y0, x1, y1, BLUE);  // Should align with baseline
```

---

## References

- [Vulkan Specification - Coordinate Systems](https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#fundamentals-coordinates)
- [FreeType Documentation - Glyph Metrics](https://freetype.org/freetype2/docs/glyphs/glyphs-3.html)
- [Cairo Documentation - Coordinate System](https://www.cairographics.org/manual/cairo-Transformations.html)
- [HarfBuzz Manual - Buffers](https://harfbuzz.github.io/buffers-language-script-and-direction.html)
- [FriBidi Reference](https://fribidi.org/)
