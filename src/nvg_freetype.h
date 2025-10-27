#ifndef NVG_FREETYPE_H
#define NVG_FREETYPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct NVGFontSystem NVGFontSystem;
typedef struct NVGcontext NVGcontext;

// Text alignment
#define NVGFT_ALIGN_LEFT     (1<<0)  // Horizontal align
#define NVGFT_ALIGN_CENTER   (1<<1)
#define NVGFT_ALIGN_RIGHT    (1<<2)
#define NVGFT_ALIGN_TOP      (1<<3)  // Vertical align
#define NVGFT_ALIGN_MIDDLE   (1<<4)
#define NVGFT_ALIGN_BOTTOM   (1<<5)
#define NVGFT_ALIGN_BASELINE (1<<6)

// Render modes
typedef enum NVGFTRenderMode {
	NVGFT_RENDER_NORMAL = 0,   // 8-bit grayscale
	NVGFT_RENDER_LIGHT,         // Light hinting
	NVGFT_RENDER_MONO,          // 1-bit monochrome
	NVGFT_RENDER_LCD,           // Subpixel horizontal
	NVGFT_RENDER_LCD_V          // Subpixel vertical
} NVGFTRenderMode;

// Glyph quad (for text iteration)
typedef struct NVGFTQuad {
	float x0, y0, x1, y1;  // Screen coordinates
	float s0, t0, s1, t1;  // Texture coordinates
} NVGFTQuad;

// Text iterator
typedef struct NVGFTTextIter {
	const char* str;
	const char* end;
	float x, y;
	float spacing;
	int prev_glyph_index;
	int font_id;
	void* internal;  // FTC scaler data
} NVGFTTextIter;

// System management
NVGFontSystem* nvgft_create(int atlasWidth, int atlasHeight);
void nvgft_destroy(NVGFontSystem* sys);

// Font management
int nvgft_add_font(NVGFontSystem* sys, const char* name, const char* path);
int nvgft_add_font_mem(NVGFontSystem* sys, const char* name, const void* data, int size, int free_data);
int nvgft_find_font(NVGFontSystem* sys, const char* name);
void nvgft_add_fallback(NVGFontSystem* sys, int base_font, int fallback_font);
void nvgft_reset_fallback(NVGFontSystem* sys, int base_font);

// State management
void nvgft_set_size(NVGFontSystem* sys, float size);
void nvgft_set_spacing(NVGFontSystem* sys, float spacing);
void nvgft_set_blur(NVGFontSystem* sys, float blur);
void nvgft_set_align(NVGFontSystem* sys, int align);
void nvgft_set_font(NVGFontSystem* sys, int font_id);
void nvgft_set_render_mode(NVGFontSystem* sys, NVGFTRenderMode mode);

// Text iteration
void nvgft_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                           float x, float y, const char* str, const char* end);
int nvgft_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter, NVGFTQuad* quad);

// Text metrics
float nvgft_text_bounds(NVGFontSystem* sys, float x, float y,
                         const char* string, const char* end, float* bounds);
void nvgft_vert_metrics(NVGFontSystem* sys, float* ascender, float* descender, float* lineh);
void nvgft_line_bounds(NVGFontSystem* sys, float y, float* miny, float* maxy);

// Atlas management
void nvgft_get_atlas_size(NVGFontSystem* sys, int* width, int* height);
int nvgft_reset_atlas(NVGFontSystem* sys, int width, int height);
int nvgft_expand_atlas(NVGFontSystem* sys, int width, int height);

// Texture callback (for GPU upload)
typedef void (*NVGFTTextureUpdateFunc)(void* uptr, int x, int y, int w, int h, const unsigned char* data);
void nvgft_set_texture_callback(NVGFontSystem* sys, NVGFTTextureUpdateFunc callback, void* uptr);

#ifdef __cplusplus
}
#endif

#endif // NVG_FREETYPE_H
