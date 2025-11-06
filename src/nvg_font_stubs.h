// Stub font system header - declarations for temporary stubs
#ifndef NVG_FONT_STUBS_H
#define NVG_FONT_STUBS_H

// Font system management
void* nvgft_create(int atlasWidth, int atlasHeight);
void nvgft_destroy(void* fs);
void nvgft_set_texture_callback(void* fs, void* callback, void* userdata);

// Font loading
int nvgft_add_font(void* fs, const char* name, const char* path);
int nvgft_add_font_mem(void* fs, const char* name, unsigned char* data, int ndata, int freeData);
int nvgft_find_font(void* fs, const char* name);
void nvgft_add_fallback(void* fs, int baseFont, int fallbackFont);
void nvgft_reset_fallback(void* fs, int baseFont);

// Font state
void nvgft_set_font(void* fs, int fontId);
void nvgft_set_size(void* fs, float size);
void nvgft_set_spacing(void* fs, float spacing);
void nvgft_set_blur(void* fs, float blur);
void nvgft_set_align(void* fs, int align);
void nvgft_set_font_msdf(void* fs, int font, int msdfMode);
void nvgft_reset_atlas(void* fs, int width, int height);

// Text layout
void nvgft_shaped_text_iter_init(void* fs, void* iter, float x, float y, const char* string, const char* end, int bidi, void* state);
int nvgft_shaped_text_iter_next(void* fs, void* iter, void* q);
void nvgft_text_iter_free(void* iter);
int nvgft_text_iter_next(void* fs, void* iter, void* quad);

// Text measurement
float nvgft_text_bounds(void* fs, float x, float y, const char* string, const char* end, float* bounds);
float nvgft_text_bounds_shaped(void* fs, float x, float y, const char* string, const char* end, float* bounds, int bidi, void* state);
void nvgft_vert_metrics(void* fs, float* ascender, float* descender, float* lineh);
float nvgft_line_bounds(void* fs, float y, float* miny, float* maxy);

// Variable fonts
int nvgft_set_var_design_coords(void* fs, int fontId, const float* coords, unsigned int num_coords);
int nvgft_get_var_design_coords(void* fs, int fontId, float* coords, unsigned int num_coords);
int nvgft_get_named_instance_count(void* fs, int fontId);
int nvgft_set_named_instance(void* fs, int fontId, unsigned int instance_index);

// OpenType features
void nvgft_set_feature(void* fs, const char* tag, int enabled);
void nvgft_reset_features(void* fs);

// Font configuration
void nvgft_set_hinting(void* fs, int hinting);
void nvgft_set_kerning(void* fs, int enabled);

// Font information
const char* nvgft_get_family_name(void* fs, int fontId);
const char* nvgft_get_style_name(void* fs, int fontId);
int nvgft_is_variable(void* fs, int fontId);
int nvgft_is_scalable(void* fs, int fontId);
int nvgft_is_fixed_width(void* fs, int fontId);
int nvgft_get_var_axis_count(void* fs, int fontId);
int nvgft_get_var_axis(void* fs, int fontId, unsigned int axis_index, void* axis);

// Glyph-level API
int nvgft_get_glyph_count(void* fs, int fontId);
int nvgft_get_glyph_metrics(void* fs, int fontId, unsigned int glyph_index, void* metrics);
float nvgft_get_kerning(void* fs, int fontId, unsigned int left_glyph, unsigned int right_glyph);
int nvgft_render_glyph(void* fs, int fontId, unsigned int glyph_index, float size, unsigned char** bitmap, int* width, int* height, int* pitch, int* left, int* top);

#endif // NVG_FONT_STUBS_H
