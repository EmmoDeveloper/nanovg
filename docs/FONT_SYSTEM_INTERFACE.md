# Font System Interface Design

## Architectural Principle: Zero Leakage

The font system is a **completely independent module** with no implementation details visible to NanoVG core or Vulkan backend.

---

## Public Interface (`src/font/nvg_font_system.h`)

This is the ONLY header file that can be included by `nanovg.c`. All other font system files are private.

```c
#ifndef NVG_FONT_SYSTEM_H
#define NVG_FONT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// Opaque handle - internal structure never exposed
typedef struct NVGFontSystem NVGFontSystem;

// Font handle returned when loading a font
typedef int NVGFontHandle;

// Glyph vertex data for rendering
typedef struct NVGFontVertex {
	float x, y;         // Position
	float u, v;         // Texture coordinates
} NVGFontVertex;

// Text metrics for layout
typedef struct NVGFontMetrics {
	float ascender;     // Distance from baseline to top
	float descender;    // Distance from baseline to bottom (negative)
	float lineHeight;   // Recommended line spacing
} NVGFontMetrics;

// Callbacks provided by NanoVG to font system
typedef struct NVGFontCallbacks {
	// Create a texture atlas (returns texture ID)
	int (*createTexture)(void* userdata, int width, int height, const unsigned char* data);

	// Update texture atlas region
	void (*updateTexture)(void* userdata, int textureId, int x, int y,
	                      int width, int height, const unsigned char* data);

	// User data pointer passed to callbacks
	void* userdata;
} NVGFontCallbacks;

// === Font System Lifecycle ===

// Create font system with atlas size and callbacks
NVGFontSystem* nvgFontSystemCreate(int atlasWidth, int atlasHeight,
                                    const NVGFontCallbacks* callbacks);

// Destroy font system and free all resources
void nvgFontSystemDestroy(NVGFontSystem* fs);

// === Font Loading ===

// Load a font from file path
// Returns: font handle (>= 0) on success, -1 on error
NVGFontHandle nvgFontSystemLoadFont(NVGFontSystem* fs, const char* path);

// Load a font from memory
NVGFontHandle nvgFontSystemLoadFontMem(NVGFontSystem* fs,
                                        const unsigned char* data, int dataSize);

// === Text Rendering ===

// Render text and return glyph vertices
// vertices: output buffer (allocated by caller, can be NULL to query size)
// maxVertices: size of vertices buffer
// Returns: number of vertices generated, -1 on error
// If vertices is NULL, returns required buffer size
int nvgFontSystemRenderText(NVGFontSystem* fs,
                             NVGFontHandle font,
                             const char* text,
                             float x, float y,    // Baseline position
                             float size,
                             NVGFontVertex* vertices,
                             int maxVertices,
                             int* outTextureId);  // Which atlas texture to use

// === Text Metrics ===

// Get font metrics at given size
void nvgFontSystemGetMetrics(NVGFontSystem* fs,
                              NVGFontHandle font,
                              float size,
                              NVGFontMetrics* metrics);

// Measure text width
float nvgFontSystemMeasureText(NVGFontSystem* fs,
                                NVGFontHandle font,
                                const char* text,
                                float size);

// === Advanced Features (Phase 2+) ===

// Variable fonts
int nvgFontSystemSetVariation(NVGFontSystem* fs, NVGFontHandle font,
                               const char* axisTag, float value);

// OpenType features
void nvgFontSystemEnableFeature(NVGFontSystem* fs, const char* featureTag);
void nvgFontSystemDisableFeature(NVGFontSystem* fs, const char* featureTag);

// Text direction for BiDi
typedef enum {
	NVG_TEXT_DIR_LTR,
	NVG_TEXT_DIR_RTL,
	NVG_TEXT_DIR_AUTO
} NVGTextDirection;

void nvgFontSystemSetTextDirection(NVGFontSystem* fs, NVGTextDirection dir);

#endif // NVG_FONT_SYSTEM_H
```

---

## Integration in nanovg.c

### Include (ONLY ONE)
```c
#include "font/nvg_font_system.h"
```

### NanoVG Context Structure
```c
struct NVGcontext {
	// ... existing fields ...

	NVGFontSystem* fontSystem;  // Opaque handle - no knowledge of internals
	int currentFont;            // Current font handle
	float currentSize;          // Current font size
};
```

### Initialization
```c
NVGcontext* nvgCreateVk(...) {
	NVGcontext* ctx = malloc(sizeof(NVGcontext));

	// Setup callbacks for font system
	NVGFontCallbacks callbacks = {
		.createTexture = nvg__createTexture,
		.updateTexture = nvg__updateTexture,
		.userdata = ctx
	};

	// Create font system - all implementation hidden
	ctx->fontSystem = nvgFontSystemCreate(2048, 2048, &callbacks);

	return ctx;
}
```

### Text Rendering
```c
void nvgText(NVGcontext* ctx, float x, float y, const char* text, const char* end) {
	// Query required vertex count
	int numVerts = nvgFontSystemRenderText(ctx->fontSystem,
	                                        ctx->currentFont,
	                                        text, x, y, ctx->currentSize,
	                                        NULL, 0, NULL);

	// Allocate vertex buffer
	NVGFontVertex* verts = malloc(numVerts * sizeof(NVGFontVertex));

	// Render glyphs
	int textureId;
	nvgFontSystemRenderText(ctx->fontSystem, ctx->currentFont,
	                        text, x, y, ctx->currentSize,
	                        verts, numVerts, &textureId);

	// Convert to NanoVG internal vertices and render
	// ... (existing NanoVG rendering code)

	free(verts);
}
```

---

## Zero Leakage in Vulkan Backend

### nvg_vk.c - NO font system includes
```c
// NO INCLUDES of font system headers

// Only interact through NanoVG public API
NVGcontext* nvgCreateVk(...) {
	// Font system created by nanovg.c, not here
	// Vulkan backend knows NOTHING about fonts
}
```

### nvg_vk_context.c - NO font system code
```c
// NO font system initialization
// NO atlas management
// Textures created via normal nvgCreateTexture() API
```

### nvg_vk_texture.c - Generic texture handling
```c
// Font atlas is just another texture
// No special cases for font textures
// No knowledge that texture contains glyphs
```

---

## Internal Font System Organization (Hidden)

All these files are PRIVATE - never included outside `src/font/`:

### `src/font/nvg_font_internal.h`
Contains all internal structures, only included by font system .c files.

### `src/font/nvg_font_system.c`
Main implementation, manages font loading and rendering.

### `src/font/nvg_font_freetype.c`
FreeType integration - glyph rasterization, metrics.

### `src/font/nvg_font_harfbuzz.c`
HarfBuzz integration - text shaping, OpenType features.

### `src/font/nvg_font_cairo.c`
Cairo integration - COLR emoji rendering.

### `src/font/nvg_font_atlas.c`
Texture atlas packing, glyph cache, LRU eviction.

### `src/font/nvg_font_bidi.c`
FriBidi integration - bidirectional text reordering.

---

## Communication Flow

```
┌─────────────┐
│  nanovg.c   │  Knows ONLY about nvg_font_system.h
└──────┬──────┘
       │ nvgFontSystemRenderText()
       ▼
┌─────────────────┐
│ nvg_font_system │  Opaque implementation
│   (hidden)      │
└──────┬──────────┘
       │ callbacks.createTexture()
       │ callbacks.updateTexture()
       ▼
┌─────────────┐
│  nanovg.c   │  Creates/updates textures
└──────┬──────┘
       │ nvgCreateTexture()
       ▼
┌─────────────┐
│  nvg_vk.c   │  Generic texture creation
└─────────────┘
```

**Key Points:**
1. Font system calls INTO nanovg.c via callbacks
2. nanovg.c calls INTO font system via public API
3. Vulkan backend knows NOTHING about fonts
4. No circular dependencies
5. No header leakage

---

## Makefile Organization

```makefile
# Font system objects (in separate directory)
FONT_OBJS := $(BUILD_DIR)/font/nvg_font_system.o \
             $(BUILD_DIR)/font/nvg_font_freetype.o \
             $(BUILD_DIR)/font/nvg_font_harfbuzz.o \
             $(BUILD_DIR)/font/nvg_font_cairo.o \
             $(BUILD_DIR)/font/nvg_font_atlas.o \
             $(BUILD_DIR)/font/nvg_font_bidi.o

# NanoVG core (includes font/nvg_font_system.h ONLY)
$(BUILD_DIR)/nanovg.o: src/nanovg.c src/font/nvg_font_system.h
	$(CC) $(CFLAGS) -c $< -o $@

# Font system files (can include internal headers)
$(BUILD_DIR)/font/%.o: src/font/%.c src/font/nvg_font_internal.h
	@mkdir -p $(BUILD_DIR)/font
	$(CC) $(CFLAGS) -c $< -o $@
```

---

## Testing Strategy

### Unit Tests (Font System Isolated)
```c
// Test font system without NanoVG
NVGFontCallbacks callbacks = { mock_create, mock_update, NULL };
NVGFontSystem* fs = nvgFontSystemCreate(1024, 1024, &callbacks);

NVGFontHandle font = nvgFontSystemLoadFont(fs, "test.ttf");
// Test rendering, metrics, etc.

nvgFontSystemDestroy(fs);
```

### Integration Tests (With NanoVG)
```c
// Normal NanoVG API usage
NVGcontext* ctx = nvgCreateVk(...);
int font = nvgCreateFont(ctx, "test.ttf");
nvgText(ctx, 0, 0, "Hello", NULL);
```

---

## Benefits of This Architecture

1. **Complete Isolation**: Font system can be replaced without touching other code
2. **Independent Testing**: Font system tested in isolation
3. **Clear Ownership**: Each module owns its data, no shared state
4. **Maintainability**: Changes to font system don't break Vulkan backend
5. **Portability**: Font system can be used with OpenGL, Metal, etc.
6. **Debuggability**: Font issues isolated to `src/font/` directory
7. **No Header Pollution**: FreeType/HarfBuzz/Cairo headers never leak

---

## Enforcement Checklist

Before merging any code, verify:

- [ ] Only `src/font/nvg_font_system.h` included in `nanovg.c`
- [ ] NO font system headers in `nvg_vk.c`, `nvg_vk_context.c`, `nvg_vk_texture.c`
- [ ] NO direct FreeType/HarfBuzz/Cairo includes outside `src/font/`
- [ ] All font system internals in `nvg_font_internal.h`
- [ ] Font system compiles as standalone library
- [ ] No cyclic dependencies
- [ ] All communication via callbacks or public API
- [ ] Vulkan backend treats font atlas as generic texture
