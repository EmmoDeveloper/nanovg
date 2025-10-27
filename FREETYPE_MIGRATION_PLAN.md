# FreeType Direct Integration - Migration Plan

## Goal
Replace **fontstash.h** (1,100 lines) with **direct FreeType** integration using FTC cache subsystem.

---

## Current Architecture

```
NanoVG (nanovg.c)
    ↓
FONScontext (fontstash.h)
    ↓
FreeType (wrapped by fontstash)
```

**Fontstash responsibilities**:
1. Font loading and management
2. Glyph atlas management (packing, eviction)
3. Text iteration and layout
4. Texture updates
5. Glyph caching

---

## New Architecture

```
NanoVG (nanovg.c)
    ↓
NVGFontSystem (new: nvg_freetype.c)
    ↓
FTC Cache Subsystem (/opt/freetype/include/freetype/ftcache.h)
    ↓
FreeType
```

**NVGFontSystem responsibilities**:
1. Font loading → `FTC_Manager`
2. Glyph caching → `FTC_SBitCache` or `FTC_ImageCache`
3. Text iteration → Custom (simple, ~50 lines)
4. Texture updates → Direct GPU upload
5. Atlas management → FTC handles automatically

---

## Fontstash API That Needs Replacement

### Context Management
```c
FONScontext* fonsCreateInternal(FONSparams* params);
void fonsDeleteInternal(FONScontext* s);
```
**Replace with**:
```c
NVGFontSystem* nvgft_create(NVGcontext* ctx, int atlasWidth, int atlasHeight);
void nvgft_destroy(NVGFontSystem* sys);
```

### Font Loading
```c
int fonsAddFont(FONScontext* s, const char* name, const char* path, int fontIndex);
int fonsAddFontMem(FONScontext* s, const char* name, unsigned char* data, int ndata, int freeData, int fontIndex);
int fonsGetFontByName(FONScontext* s, const char* name);
int fonsAddFallbackFont(FONScontext* stash, int base, int fallback);
```
**Replace with**:
```c
int nvgft_add_font(NVGFontSystem* sys, const char* name, const char* path);
int nvgft_add_font_mem(NVGFontSystem* sys, const char* name, const void* data, int size);
int nvgft_find_font(NVGFontSystem* sys, const char* name);
void nvgft_add_fallback(NVGFontSystem* sys, int base, int fallback);
```

### State Management
```c
void fonsPushState(FONScontext* s);
void fonsPopState(FONScontext* s);
void fonsSetSize(FONScontext* s, float size);
void fonsSetSpacing(FONScontext* s, float spacing);
void fonsSetBlur(FONScontext* s, float blur);
void fonsSetAlign(FONScontext* s, int align);
void fonsSetFont(FONScontext* s, int font);
```
**Replace with**: Move to NanoVG's existing state system (already has fontSize, letterSpacing, textAlign)

### Text Iteration (Core of nvgText)
```c
int fonsTextIterInit(FONScontext* stash, FONStextIter* iter, float x, float y, const char* str, const char* end, int bitmapOption);
int fonsTextIterNext(FONScontext* stash, FONStextIter* iter, struct FONSquad* quad);
```
**Replace with**:
```c
typedef struct NVGFTTextIter {
    const char* str;
    const char* end;
    float x, y;
    int prev_glyph_index;
    FTC_ScalerRec scaler;
} NVGFTTextIter;

void nvgft_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                           float x, float y, const char* str, const char* end);
int nvgft_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter,
                          float* x0, float* y0, float* x1, float* y1,
                          float* s0, float* t0, float* s1, float* t1);
```

### Metrics
```c
float fonsTextBounds(FONScontext* s, float x, float y, const char* string, const char* end, float* bounds);
void fonsLineBounds(FONScontext* s, float y, float* miny, float* maxy);
void fonsVertMetrics(FONScontext* s, float* ascender, float* descender, float* lineh);
```
**Replace with**:
```c
float nvgft_text_bounds(NVGFontSystem* sys, float x, float y, const char* string, const char* end, float* bounds);
void nvgft_vert_metrics(NVGFontSystem* sys, float* ascender, float* descender, float* lineh);
```

### Atlas Management
```c
void fonsGetAtlasSize(FONScontext* s, int* width, int* height);
int fonsExpandAtlas(FONScontext* s, int width, int height);
int fonsResetAtlas(FONScontext* stash, int width, int height);
int fonsValidateTexture(FONScontext* s, int* dirty);
```
**Replace with**: FTC cache handles automatically, no manual atlas management needed

---

## Implementation Plan

### Phase 1: Create nvg_freetype.c/h (FTC Integration)

**File**: `src/nvg_freetype.c`

```c
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_GLYPH_H

typedef struct NVGFTFont {
    char name[64];
    FTC_FaceID face_id;  // Pointer to font data or path
    int id;
    int fallback;  // Fallback font ID
} NVGFTFont;

typedef struct NVGFontSystem {
    FT_Library library;
    FTC_Manager cache_manager;
    FTC_CMapCache cmap_cache;
    FTC_SBitCache sbit_cache;  // Small bitmap cache
    FTC_ImageCache image_cache;  // Glyph image cache

    NVGFTFont fonts[32];
    int font_count;

    // Current state (for text iteration)
    int current_font;
    float current_size;
    float letter_spacing;
    int text_align;

    // Texture management
    VkImage atlas_texture;
    int atlas_width;
    int atlas_height;
    int atlas_dirty;

    NVGcontext* nvg_ctx;  // Back-reference for GPU upload
} NVGFontSystem;

// Face requester callback (required by FTC)
static FT_Error face_requester(FTC_FaceID face_id,
                                FT_Library library,
                                FT_Pointer request_data,
                                FT_Face* aface) {
    NVGFTFont* font = (NVGFTFont*)face_id;
    // Load from font->path or font->data
    return FT_New_Face(library, font->path, 0, aface);
}

NVGFontSystem* nvgft_create(NVGcontext* ctx, int atlasWidth, int atlasHeight) {
    NVGFontSystem* sys = (NVGFontSystem*)calloc(1, sizeof(NVGFontSystem));

    // Init FreeType
    FT_Init_FreeType(&sys->library);

    // Create cache manager
    FTC_Manager_New(sys->library,
        0,  // max_faces (0 = no limit)
        0,  // max_sizes (0 = no limit)
        16 * 1024 * 1024,  // max_bytes (16MB)
        face_requester,
        NULL,  // request_data
        &sys->cache_manager);

    // Create caches
    FTC_CMapCache_New(sys->cache_manager, &sys->cmap_cache);
    FTC_SBitCache_New(sys->cache_manager, &sys->sbit_cache);
    FTC_ImageCache_New(sys->cache_manager, &sys->image_cache);

    sys->atlas_width = atlasWidth;
    sys->atlas_height = atlasHeight;
    sys->nvg_ctx = ctx;

    return sys;
}

int nvgft_add_font(NVGFontSystem* sys, const char* name, const char* path) {
    if (sys->font_count >= 32) return -1;

    NVGFTFont* font = &sys->fonts[sys->font_count];
    strncpy(font->name, name, sizeof(font->name)-1);
    font->path = strdup(path);  // Or copy path
    font->face_id = (FTC_FaceID)font;  // Use pointer as ID
    font->id = sys->font_count;
    font->fallback = -1;

    return sys->font_count++;
}

// Text iteration using FTC
int nvgft_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter,
                          float* x0, float* y0, float* x1, float* y1,
                          float* s0, float* t0, float* s1, float* t1) {
    if (iter->str >= iter->end) return 0;

    // Decode UTF-8
    uint32_t codepoint = decode_utf8(&iter->str);

    // Get glyph index via charmap cache
    FT_UInt glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache,
        sys->fonts[sys->current_font].face_id,
        -1,  // charmap index (-1 = default)
        codepoint);

    if (glyph_index == 0) {
        // Try fallback font
        if (sys->fonts[sys->current_font].fallback >= 0) {
            glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache,
                sys->fonts[sys->fonts[sys->current_font].fallback].face_id,
                -1, codepoint);
        }
        if (glyph_index == 0) return nvgft_text_iter_next(...);  // Skip
    }

    // Get glyph bitmap from cache
    FTC_SBit sbit;
    FTC_ImageTypeRec type = {
        .face_id = sys->fonts[sys->current_font].face_id,
        .width = 0,
        .height = (int)sys->current_size,
        .flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL
    };

    FTC_SBitCache_Lookup(sys->sbit_cache, &type, glyph_index, &sbit, NULL);

    // Get kerning
    if (iter->prev_glyph_index >= 0) {
        FT_Vector kerning;
        FTC_Manager_LookupFace(sys->cache_manager, type.face_id, &face);
        FT_Get_Kerning(face, iter->prev_glyph_index, glyph_index,
                       FT_KERNING_DEFAULT, &kerning);
        iter->x += kerning.x / 64.0f;
    }

    // Build quad
    *x0 = iter->x + sbit->left;
    *y0 = iter->y - sbit->top;
    *x1 = *x0 + sbit->width;
    *y1 = *y0 + sbit->height;

    // UV coordinates (need to upload to atlas first)
    // This is where FTC doesn't help - we still need manual atlas packing
    // OR: Use separate texture per glyph (less efficient)
    // OR: Use FTC_ImageCache and pack later

    // Advance cursor
    iter->x += sbit->xadvance;
    iter->prev_glyph_index = glyph_index;

    return 1;
}
```

### Phase 2: Modify nanovg.c

**Changes to struct NVGcontext**:
```c
struct NVGcontext {
    NVGparams params;
    // ... existing fields ...

    // REMOVE: struct FONScontext* fs;
    // ADD:
    struct NVGFontSystem* ft;  // FreeType system

    int fontImages[NVG_MAX_FONTIMAGES];
    int fontImageIdx;
    // ... rest unchanged ...
};
```

**Modify nvgCreateInternal()**:
```c
NVGcontext* nvgCreateInternal(NVGparams* params) {
    // ... existing code ...

    // REMOVE fontstash creation
    // ADD FreeType creation
    ctx->ft = nvgft_create(ctx, atlasSize, atlasSize);
    if (ctx->ft == NULL) goto error;

    // ... rest ...
}
```

**Modify nvgText()**:
```c
float nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
    NVGstate* state = nvg__getState(ctx);
    NVGFTTextIter iter;
    // ... existing setup ...

    // REPLACE fonsTextIterInit/Next with:
    nvgft_text_iter_init(ctx->ft, &iter, x*scale, y*scale, string, end);

    while (nvgft_text_iter_next(ctx->ft, &iter, &x0, &y0, &x1, &y1, &s0, &t0, &s1, &t1)) {
        // Build triangles (same as before)
        nvg__vset(&verts[nverts], c[0], c[1], s0, t0); nverts++;
        // ...
    }

    // ... rest unchanged ...
}
```

### Phase 3: Handle Atlas Packing

**Challenge**: FTC caches glyphs but doesn't pack them into atlas textures.

**Options**:

**Option A**: Keep our packing algorithm, use FTC for caching only
```c
// In nvgft_text_iter_next:
FTC_SBitCache_Lookup(..., &sbit, NULL);

// Check if glyph is in atlas
if (!atlas_has_glyph(glyph_id)) {
    // Pack into atlas
    pack_glyph(atlas, sbit->buffer, sbit->width, sbit->height, &u0, &v0);
    // Upload region to GPU
    upload_atlas_region(...);
}

// Return UV coords from atlas
```

**Option B**: Use FTC_ImageCache + render on-demand
```c
// Use image cache instead of sbit cache
FTC_ImageCache_Lookup(..., &glyph);
// glyph->bitmap has the rendered bitmap
// Pack and upload as needed
```

**Option C**: One texture per cached glyph (simplest but least efficient)
```c
// Each FTC glyph gets its own tiny texture
// Use texture arrays or bindless textures
```

**Recommendation**: Option A - keep our atlas packing, use FTC just for glyph caching

---

## Benefits

1. **Less code**: Remove 1,100 lines of fontstash, add ~400 lines of FTC wrapper
2. **Better caching**: FTC is production-tested (used by most Linux font rendering)
3. **More features**: Easy access to all FreeType features
4. **Performance**: FTC is highly optimized
5. **Maintenance**: FreeType team maintains the cache

## Risks

1. **Atlas packing**: FTC doesn't do atlas packing, we still need to handle that
2. **Texture updates**: Need to integrate with Vulkan upload path
3. **Testing**: Need to verify all text rendering still works

## Timeline

- Phase 1 (nvg_freetype.c): 3-4 days
- Phase 2 (nanovg.c integration): 2-3 days
- Phase 3 (Atlas packing): 2-3 days
- Testing: 2-3 days

**Total**: ~10-12 days for complete migration

---

## Next Steps

1. Create `src/nvg_freetype.c` with FTC integration
2. Test standalone (without NanoVG integration)
3. Modify nanovg.c to use new system
4. Remove fontstash.h
5. Verify all tests pass
6. Add subpixel rendering while we're at it

Should I start implementing nvg_freetype.c?
