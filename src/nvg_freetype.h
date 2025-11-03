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

// Shaped glyph (output from HarfBuzz)
typedef struct NVGFTShapedGlyph {
	uint32_t glyph_index;
	float x_offset;
	float y_offset;
	float x_advance;
	float y_advance;
	uint32_t cluster;  // Byte offset in original text
} NVGFTShapedGlyph;

// Text iterator
typedef struct NVGFTTextIter {
	const char* str;
	const char* end;
	const char* next;  // Next character position
	float x, y;
	float spacing;
	int prev_glyph_index;
	int font_id;
	uint32_t codepoint;  // Current codepoint (for text breaking)
	void* internal;  // FTC scaler data

	// HarfBuzz shaped glyphs (when using shaped iteration)
	NVGFTShapedGlyph* shaped_glyphs;
	int num_shaped_glyphs;
	int current_glyph;
	float shaped_x, shaped_y;  // Current position for shaped glyphs
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
void nvgft_set_kerning(NVGFontSystem* sys, int enabled);
void nvgft_set_hinting(NVGFontSystem* sys, int hinting);
void nvgft_set_font_msdf(NVGFontSystem* sys, int font_id, int msdf_mode);

// Text iteration (simple, no shaping)
void nvgft_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                           float x, float y, const char* str, const char* end);
int nvgft_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter, NVGFTQuad* quad);

// Shaped text iteration (with HarfBuzz + FriBidi)
// direction: 0=auto, 1=LTR, 2=RTL
// language: "en", "ar", "hi", etc. (NULL for auto-detect)
void nvgft_shaped_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                                  float x, float y, const char* str, const char* end,
                                  int direction, const char* language);
int nvgft_shaped_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter, NVGFTQuad* quad);
void nvgft_text_iter_free(NVGFTTextIter* iter);

// Text metrics
float nvgft_text_bounds(NVGFontSystem* sys, float x, float y,
                         const char* string, const char* end, float* bounds);
float nvgft_text_bounds_shaped(NVGFontSystem* sys, float x, float y,
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

// Glyph rasterization (for virtual atlas integration)
// Returns pixel data in format specified by bytesPerPixel (caller must free with free())
// bytesPerPixel output: 1=GRAY, 3=RGB (MSDF), 4=RGBA
// Returns NULL on failure
unsigned char* nvgft_rasterize_glyph(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                                       int pixel_size, int* width, int* height,
                                       int* bearing_x, int* bearing_y, int* advance_x,
                                       int* bytesPerPixel);

// Glyph outline extraction (for GPU MSDF generation)
// Forward declaration for outline structure
struct VKNVGglyphOutline;
// Returns outline data (caller must free with free()), NULL on failure
struct VKNVGglyphOutline* nvgft_extract_glyph_outline(NVGFontSystem* sys, int font_id,
                                                         uint32_t codepoint, int pixel_size);

// Glyph-level API extensions
typedef struct NVGFTGlyphMetrics {
	float bearingX, bearingY;    // Glyph placement offset from baseline
	float advanceX, advanceY;    // Cursor movement after glyph
	float width, height;         // Bitmap dimensions
	int glyphIndex;              // FreeType glyph index
} NVGFTGlyphMetrics;

// Get metrics for a codepoint (returns 0 on success, -1 on failure)
int nvgft_get_glyph_metrics(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                              NVGFTGlyphMetrics* metrics);

// Get kerning between two codepoints (in pixels)
float nvgft_get_kerning(NVGFontSystem* sys, int font_id,
                         uint32_t left_codepoint, uint32_t right_codepoint);

// Render a single glyph and return quad (returns 0 on success, -1 on failure)
int nvgft_render_glyph(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                        float x, float y, NVGFTQuad* quad);

// Render glyph by FreeType glyph index (returns 0 on success, -1 on failure)
int nvgft_render_glyph_index(NVGFontSystem* sys, int font_id, int glyph_index,
                               float x, float y, NVGFTQuad* quad);

// Font information queries
const char* nvgft_get_family_name(NVGFontSystem* sys, int font_id);
const char* nvgft_get_style_name(NVGFontSystem* sys, int font_id);
int nvgft_get_glyph_count(NVGFontSystem* sys, int font_id);
int nvgft_is_scalable(NVGFontSystem* sys, int font_id);
int nvgft_is_fixed_width(NVGFontSystem* sys, int font_id);

// Variable font support
typedef struct NVGFTVarAxis {
	char name[64];
	float minimum;
	float def;
	float maximum;
	unsigned int tag;
} NVGFTVarAxis;

int nvgft_is_variable(NVGFontSystem* sys, int font_id);
int nvgft_get_var_axis_count(NVGFontSystem* sys, int font_id);
int nvgft_get_var_axis(NVGFontSystem* sys, int font_id, int axis_index, NVGFTVarAxis* axis);
int nvgft_set_var_design_coords(NVGFontSystem* sys, int font_id, const float* coords, int num_coords);
int nvgft_get_var_design_coords(NVGFontSystem* sys, int font_id, float* coords, int num_coords);
int nvgft_get_named_instance_count(NVGFontSystem* sys, int font_id);
int nvgft_set_named_instance(NVGFontSystem* sys, int font_id, int instance_index);

// OpenType features
void nvgft_set_feature(NVGFontSystem* sys, unsigned int tag, int enabled);
void nvgft_reset_features(NVGFontSystem* sys);

// Pixel format definitions
typedef enum NVGFTPixelFormat {
	NVGFT_PIXEL_GRAY = 1,   // 1 channel: grayscale (SDF, bitmap fonts)
	NVGFT_PIXEL_RGB = 3,    // 3 channels: RGB (MSDF)
	NVGFT_PIXEL_RGBA = 4    // 4 channels: RGBA (color emoji, general images)
} NVGFTPixelFormat;

// Get bytes per pixel for a format
static inline int nvgft_bytes_per_pixel(NVGFTPixelFormat format) {
	return (int)format;
}

// Get channel count for a format
static inline int nvgft_channel_count(NVGFTPixelFormat format) {
	return (int)format;
}

// Virtual atlas integration callbacks
typedef struct NVGFTVirtualAtlasGlyph {
	float s0, t0, s1, t1;  // Texture coordinates in atlas
	int atlasIndex;        // Which texture to use
	int width, height;     // Glyph dimensions in pixels
	int bearingX, bearingY;
	int advance;
} NVGFTVirtualAtlasGlyph;

// Query virtual atlas for a glyph (returns 0 if found, -1 if not found)
typedef int (*NVGFTVirtualAtlasQueryFunc)(void* atlasContext, int font_id, uint32_t codepoint,
                                           int pixel_size, NVGFTVirtualAtlasGlyph* glyph);

// Set virtual atlas callbacks
void nvgft_set_virtual_atlas(NVGFontSystem* sys, NVGFTVirtualAtlasQueryFunc queryFunc, void* atlasContext);

#ifdef __cplusplus
}
#endif

#endif // NVG_FREETYPE_H
