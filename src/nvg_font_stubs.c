// Temporary stub implementations for font system
// These will be replaced when the new font system is implemented

#include <stdio.h>
#include <stdlib.h>

// Stub font system functions - minimal implementations to allow compilation

void* nvgft_create(int atlasWidth, int atlasHeight) {
	(void)atlasWidth; (void)atlasHeight;
	// Return non-NULL stub pointer to indicate "success"
	static int dummy = 0;
	return &dummy;
}

void nvgft_destroy(void* fs) {
	(void)fs;
}

void nvgft_set_texture_callback(void* fs, void* callback, void* userdata) {
	(void)fs; (void)callback; (void)userdata;
}

int nvgft_add_font(void* fs, const char* name, const char* path) {
	(void)fs; (void)name; (void)path;
	return -1;
}

int nvgft_add_font_mem(void* fs, const char* name, unsigned char* data, int ndata, int freeData) {
	(void)fs; (void)name; (void)data; (void)ndata; (void)freeData;
	return -1;
}

int nvgft_find_font(void* fs, const char* name) {
	(void)fs; (void)name;
	return -1;
}

void nvgft_add_fallback(void* fs, int baseFont, int fallbackFont) {
	(void)fs; (void)baseFont; (void)fallbackFont;
}

void nvgft_reset_fallback(void* fs, int baseFont) {
	(void)fs; (void)baseFont;
}

void nvgft_set_font_msdf(void* fs, int font, int msdfMode) {
	(void)fs; (void)font; (void)msdfMode;
}

void nvgft_reset_atlas(void* fs, int width, int height) {
	(void)fs; (void)width; (void)height;
}

void nvgft_set_font(void* fs, int fontId) {
	(void)fs; (void)fontId;
}

void nvgft_set_size(void* fs, float size) {
	(void)fs; (void)size;
}

void nvgft_set_spacing(void* fs, float spacing) {
	(void)fs; (void)spacing;
}

void nvgft_set_blur(void* fs, float blur) {
	(void)fs; (void)blur;
}

void nvgft_set_align(void* fs, int align) {
	(void)fs; (void)align;
}

void nvgft_shaped_text_iter_init(void* fs, void* iter, float x, float y, const char* string, const char* end, int bidi, void* state) {
	(void)fs; (void)iter; (void)x; (void)y; (void)string; (void)end; (void)bidi; (void)state;
}

int nvgft_shaped_text_iter_next(void* fs, void* iter, void* q) {
	(void)fs; (void)iter; (void)q;
	return 0;
}

void nvgft_text_iter_free(void* fs, void* iter) {
	(void)fs; (void)iter;
}

int nvgft_text_iter_next(void* fs, void* iter, void* quad) {
	(void)fs; (void)iter; (void)quad;
	return 0;
}

float nvgft_text_bounds(void* fs, float x, float y, const char* string, const char* end, float* bounds) {
	(void)fs; (void)x; (void)y; (void)string; (void)end; (void)bounds;
	return 0.0f;
}

float nvgft_text_bounds_shaped(void* fs, float x, float y, const char* string, const char* end, float* bounds, int bidi, void* state) {
	(void)fs; (void)x; (void)y; (void)string; (void)end; (void)bounds; (void)bidi; (void)state;
	return 0.0f;
}

void nvgft_vert_metrics(void* fs, float* ascender, float* descender, float* lineh) {
	(void)fs;
	if (ascender) *ascender = 0.0f;
	if (descender) *descender = 0.0f;
	if (lineh) *lineh = 0.0f;
}

float nvgft_line_bounds(void* fs, float y, float* miny, float* maxy) {
	(void)fs; (void)y;
	if (miny) *miny = 0.0f;
	if (maxy) *maxy = 0.0f;
	return 0.0f;
}

int nvgft_set_var_design_coords(void* fs, int fontId, const float* coords, unsigned int num_coords) {
	(void)fs; (void)fontId; (void)coords; (void)num_coords;
	return 0;
}

int nvgft_get_var_design_coords(void* fs, int fontId, float* coords, unsigned int num_coords) {
	(void)fs; (void)fontId; (void)coords; (void)num_coords;
	return 0;
}

int nvgft_get_named_instance_count(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 0;
}

int nvgft_set_named_instance(void* fs, int fontId, unsigned int instance_index) {
	(void)fs; (void)fontId; (void)instance_index;
	return 0;
}

void nvgft_set_feature(void* fs, const char* tag, int enabled) {
	(void)fs; (void)tag; (void)enabled;
}

void nvgft_reset_features(void* fs) {
	(void)fs;
}

void nvgft_set_hinting(void* fs, int hinting) {
	(void)fs; (void)hinting;
}

void nvgft_set_kerning(void* fs, int enabled) {
	(void)fs; (void)enabled;
}

const char* nvgft_get_family_name(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return "";
}

const char* nvgft_get_style_name(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return "";
}

int nvgft_is_variable(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 0;
}

int nvgft_is_scalable(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 1;
}

int nvgft_is_fixed_width(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 0;
}

int nvgft_get_var_axis_count(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 0;
}

int nvgft_get_var_axis(void* fs, int fontId, unsigned int axis_index, void* axis) {
	(void)fs; (void)fontId; (void)axis_index; (void)axis;
	return 0;
}

int nvgft_get_glyph_count(void* fs, int fontId) {
	(void)fs; (void)fontId;
	return 0;
}

int nvgft_get_glyph_metrics(void* fs, int fontId, unsigned int glyph_index, void* metrics) {
	(void)fs; (void)fontId; (void)glyph_index; (void)metrics;
	return 0;
}

float nvgft_get_kerning(void* fs, int fontId, unsigned int left_glyph, unsigned int right_glyph) {
	(void)fs; (void)fontId; (void)left_glyph; (void)right_glyph;
	return 0.0f;
}

int nvgft_render_glyph(void* fs, int fontId, unsigned int glyph_index, float size, unsigned char** bitmap, int* width, int* height, int* pitch, int* left, int* top) {
	(void)fs; (void)fontId; (void)glyph_index; (void)size;
	if (bitmap) *bitmap = NULL;
	if (width) *width = 0;
	if (height) *height = 0;
	if (pitch) *pitch = 0;
	if (left) *left = 0;
	if (top) *top = 0;
	return 0;
}
