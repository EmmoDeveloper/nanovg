# Option A Implementation Analysis

## Summary

Option A (glyphAdded callback) was implemented but has a fundamental architectural flaw that causes graphics driver crashes. The implementation successfully intercepts glyph creation but fails during rendering due to UV coordinate mismatches.

## What Was Implemented

1. **Callback Integration** (nanovg_vk.h:130-158)
   - Added `vknvg__onGlyphAdded` callback to fontstash
   - Callback is invoked after each glyph is fully initialized

2. **Coordinate Patching** (nanovg_vk_render.h:614-664)
   - Modified glyph->x0/y0/x1/y1 to point to virtual atlas positions
   - Saved pending uploads for later processing

3. **Upload Redirection** (nanovg_vk_render.h:770-828)
   - Intercept renderUpdateTexture calls
   - Extract glyph pixels from fontstash's dirty rectangles
   - Queue uploads to virtual atlas

4. **Texture Binding** (nanovg_vk_render.h:1720-1727)
   - Redirect fontstash texture to virtual atlas during rendering
   - Existing code already handles this redirection

## The Fundamental Problem

**Root Cause**: Modifying glyph coordinates breaks fontstash's UV calculation.

### How Fontstash Calculates UVs

When fontstash prepares glyphs for rendering (via fonsTextIterNext), it calculates texture coordinates:

```c
// Fontstash internal calculation (approximate)
uv_x = glyph->x0 / atlas_width;
uv_y = glyph->y0 / atlas_height;
```

### The Mismatch

1. **Original Flow** (works correctly):
   - Glyph at (100, 200) in 512x512 fontstash atlas
   - UV = (100/512, 200/512) = (0.195, 0.391) ✓ Valid [0,1] range

2. **Option A Flow** (broken):
   - Glyph rasterized at (100, 200) in fontstash's 512x512 atlas
   - Callback modifies coordinates to (1000, 2000) for virtual atlas
   - Fontstash calculates UV = (1000/512, 2000/512) = (1.95, 3.91) ✗ OUT OF RANGE
   - Invalid UVs cause undefined behavior in GPU driver → crash

### Why It Crashes

- UVs outside [0,1] range cause texture sampling issues
- NVIDIA driver attempts to access invalid memory addresses
- Results in "free(): invalid pointer" during vkBeginCommandBuffer
- Crash occurs in test_text_rendering_virtual_atlas during nvgEndFrame

## Bugs Fixed During Investigation

1. **Memory Access Bug** (nanovg_vk_render.h:790)
   - **Issue**: Treated renderUpdateTexture `data` parameter as full atlas
   - **Reality**: `data` only contains the dirty rectangle
   - **Fix**: Use relative coordinates within dirty rectangle

2. **Pending Upload Management** (nanovg_vk_render.h:818)
   - **Issue**: Cleared all pending uploads even if not in dirty rectangle
   - **Fix**: Only clear successfully processed uploads, keep rest for next update

3. **Bounds Checking** (nanovg_vk_render.h:643-646)
   - Added check to prevent glyph coordinates from exceeding 4096x4096 atlas

## Why Option A Cannot Work As-Is

The approach of modifying fontstash's internal glyph coordinates is fundamentally incompatible with how fontstash calculates texture coordinates. Any modification to glyph coordinates must account for:

1. **Atlas Size Mismatch**:
   - Fontstash atlas: typically 512x512 or 1024x1024
   - Virtual atlas: 4096x4096
   - Coordinates meant for one cannot be used with the other's UV calculation

2. **Internal Consistency**:
   - Fontstash assumes glyph coordinates are relative to its own atlas
   - Breaking this assumption causes cascading failures
   - UV calculation happens deep in fontstash, cannot be easily intercepted

## Alternative Approaches

### Solution 1: Replace Fontstash Atlas Entirely

Make fontstash believe its atlas IS the virtual atlas:

```c
// When creating fontstash context
FONSparams params;
params.width = 4096;   // Virtual atlas size
params.height = 4096;
```

**Pros**: UV calculations work correctly, no coordinate patching needed
**Cons**: Large memory overhead (4096x4096), fontstash not designed for this

### Solution 2: Intercept and Modify UVs During Rendering

Don't modify glyph coordinates. Instead, in nvgText rendering (nanovg.c:2523-2528):

```c
// Intercept FONSquad and recalculate UVs for virtual atlas
if (using_virtual_atlas) {
	q.s0 = virtual_glyph_x0 / 4096.0f;
	q.t0 = virtual_glyph_y0 / 4096.0f;
	// ...
}
```

**Pros**: Keeps fontstash intact, correct UV calculation
**Cons**: Requires modifying nanovg.c, complex lookup from glyph to virtual atlas position

### Solution 3: Use Option B, C, or D

Refer to REMAINING_WORK.md for alternative approaches that don't involve modifying fontstash internals.

## Test Results

- `test_nvg_virtual_atlas.c`: ✓ PASS (no actual rendering)
- `test_real_text_rendering.c`: ✗ CRASH (invalid UVs cause driver crash)
  - Baseline test (no virtual atlas): ✓ PASS
  - Virtual atlas test: ✗ CRASH during nvgEndFrame → vkBeginCommandBuffer

## Recommendation

Option A should be abandoned or redesigned. The most promising path forward is **Solution 1** (replace fontstash atlas entirely) or exploring Option B/C/D from REMAINING_WORK.md.

The implementation attempted here provides valuable insights into the integration points but demonstrates that modifying fontstash's internal state is not viable.
