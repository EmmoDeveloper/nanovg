#include "nvg_freetype.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_ADVANCES_H
#include FT_BITMAP_H
#include FT_OUTLINE_H

#define NVGFT_MAX_FONTS 32
#define NVGFT_MAX_STATES 8
#define NVGFT_HASH_LUT_SIZE 256

// UTF-8 decoder
static uint32_t nvgft__decode_utf8(const char** str) {
	uint32_t cp = 0;
	const unsigned char* s = (const unsigned char*)*str;

	if ((*s & 0x80) == 0) {
		cp = *s;
		*str += 1;
	} else if ((*s & 0xe0) == 0xc0) {
		cp = ((*s & 0x1f) << 6) | (s[1] & 0x3f);
		*str += 2;
	} else if ((*s & 0xf0) == 0xe0) {
		cp = ((*s & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		*str += 3;
	} else if ((*s & 0xf8) == 0xf0) {
		cp = ((*s & 0x07) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		*str += 4;
	} else {
		*str += 1;
	}

	return cp;
}

// Font data structure
typedef struct NVGFTFont {
	char name[64];
	char path[260];
	unsigned char* data;  // For memory fonts
	int data_size;
	int free_data;
	int id;
	int fallback;  // Fallback font ID (-1 = none)
	FTC_FaceID face_id;  // Pointer to this struct (used by FTC)
} NVGFTFont;

// Glyph in atlas
typedef struct NVGFTGlyph {
	uint32_t codepoint;
	int font_id;
	int size;  // Pixel size
	short x, y;  // Atlas position
	short width, height;
	short offset_x, offset_y;  // Bearing
	short advance_x;
	float u0, v0, u1, v1;  // Normalized UV
} NVGFTGlyph;

// Atlas row (for packing)
typedef struct NVGFTRow {
	short x, y, h;
} NVGFTRow;

// State (for push/pop)
typedef struct NVGFTState {
	int font_id;
	float size;
	float spacing;
	float blur;
	int align;
	NVGFTRenderMode render_mode;
} NVGFTState;

// Main system
struct NVGFontSystem {
	// FreeType & FTC
	FT_Library library;
	FTC_Manager cache_manager;
	FTC_CMapCache cmap_cache;
	FTC_ImageCache image_cache;

	// Fonts
	NVGFTFont fonts[NVGFT_MAX_FONTS];
	int font_count;

	// Current state
	NVGFTState states[NVGFT_MAX_STATES];
	int nstates;

	// Atlas
	int atlas_width;
	int atlas_height;
	NVGFTRow rows[256];
	int nrows;

	// Glyph hash table
	NVGFTGlyph* glyphs[NVGFT_HASH_LUT_SIZE];
	int glyph_count;

	// Texture callback
	NVGFTTextureUpdateFunc texture_callback;
	void* texture_uptr;
};

// FTC face requester callback
static FT_Error nvgft__face_requester(FTC_FaceID face_id, FT_Library library,
                                       FT_Pointer request_data, FT_Face* aface) {
	(void)request_data;  // Unused
	NVGFTFont* font = (NVGFTFont*)face_id;

	if (font->data) {
		return FT_New_Memory_Face(library, font->data, font->data_size, 0, aface);
	} else {
		return FT_New_Face(library, font->path, 0, aface);
	}
}

NVGFontSystem* nvgft_create(int atlasWidth, int atlasHeight) {
	NVGFontSystem* sys = (NVGFontSystem*)calloc(1, sizeof(NVGFontSystem));
	if (!sys) return NULL;

	// Init FreeType
	if (FT_Init_FreeType(&sys->library)) {
		free(sys);
		return NULL;
	}

	// Create cache manager (16MB max)
	if (FTC_Manager_New(sys->library, 0, 0, 16 * 1024 * 1024,
	                     nvgft__face_requester, NULL, &sys->cache_manager)) {
		FT_Done_FreeType(sys->library);
		free(sys);
		return NULL;
	}

	// Create caches
	if (FTC_CMapCache_New(sys->cache_manager, &sys->cmap_cache)) {
		FTC_Manager_Done(sys->cache_manager);
		FT_Done_FreeType(sys->library);
		free(sys);
		return NULL;
	}

	if (FTC_ImageCache_New(sys->cache_manager, &sys->image_cache)) {
		FTC_Manager_Done(sys->cache_manager);
		FT_Done_FreeType(sys->library);
		free(sys);
		return NULL;
	}

	sys->atlas_width = atlasWidth;
	sys->atlas_height = atlasHeight;

	// Initial state
	sys->nstates = 1;
	sys->states[0].font_id = -1;
	sys->states[0].size = 16.0f;
	sys->states[0].spacing = 0.0f;
	sys->states[0].blur = 0.0f;
	sys->states[0].align = NVGFT_ALIGN_LEFT | NVGFT_ALIGN_BASELINE;
	sys->states[0].render_mode = NVGFT_RENDER_NORMAL;

	return sys;
}

void nvgft_destroy(NVGFontSystem* sys) {
	if (!sys) return;

	// Free font data
	for (int i = 0; i < sys->font_count; i++) {
		if (sys->fonts[i].data && sys->fonts[i].free_data) {
			free(sys->fonts[i].data);
		}
	}

	// Free glyph hash table
	for (int i = 0; i < NVGFT_HASH_LUT_SIZE; i++) {
		NVGFTGlyph* g = sys->glyphs[i];
		while (g) {
			NVGFTGlyph* next = NULL;
			// TODO: implement linked list properly with next pointer
			free(g);
			g = next;
		}
	}

	FTC_Manager_Done(sys->cache_manager);
	FT_Done_FreeType(sys->library);
	free(sys);
}

int nvgft_add_font(NVGFontSystem* sys, const char* name, const char* path) {
	if (sys->font_count >= NVGFT_MAX_FONTS) return -1;

	NVGFTFont* font = &sys->fonts[sys->font_count];
	strncpy(font->name, name, sizeof(font->name) - 1);
	strncpy(font->path, path, sizeof(font->path) - 1);
	font->data = NULL;
	font->data_size = 0;
	font->free_data = 0;
	font->id = sys->font_count;
	font->fallback = -1;
	font->face_id = (FTC_FaceID)font;

	return sys->font_count++;
}

int nvgft_add_font_mem(NVGFontSystem* sys, const char* name,
                        const void* data, int size, int free_data) {
	if (sys->font_count >= NVGFT_MAX_FONTS) return -1;

	NVGFTFont* font = &sys->fonts[sys->font_count];
	strncpy(font->name, name, sizeof(font->name) - 1);
	font->path[0] = '\0';
	font->data = (unsigned char*)data;
	font->data_size = size;
	font->free_data = free_data;
	font->id = sys->font_count;
	font->fallback = -1;
	font->face_id = (FTC_FaceID)font;

	return sys->font_count++;
}

int nvgft_find_font(NVGFontSystem* sys, const char* name) {
	for (int i = 0; i < sys->font_count; i++) {
		if (strcmp(sys->fonts[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

void nvgft_add_fallback(NVGFontSystem* sys, int base_font, int fallback_font) {
	if (base_font < 0 || base_font >= sys->font_count) return;
	if (fallback_font < 0 || fallback_font >= sys->font_count) return;
	sys->fonts[base_font].fallback = fallback_font;
}

void nvgft_reset_fallback(NVGFontSystem* sys, int base_font) {
	if (base_font < 0 || base_font >= sys->font_count) return;
	sys->fonts[base_font].fallback = -1;
}

// State management
static NVGFTState* nvgft__getState(NVGFontSystem* sys) {
	return &sys->states[sys->nstates - 1];
}

void nvgft_set_size(NVGFontSystem* sys, float size) {
	nvgft__getState(sys)->size = size;
}

void nvgft_set_spacing(NVGFontSystem* sys, float spacing) {
	nvgft__getState(sys)->spacing = spacing;
}

void nvgft_set_blur(NVGFontSystem* sys, float blur) {
	nvgft__getState(sys)->blur = blur;
}

void nvgft_set_align(NVGFontSystem* sys, int align) {
	nvgft__getState(sys)->align = align;
}

void nvgft_set_font(NVGFontSystem* sys, int font_id) {
	nvgft__getState(sys)->font_id = font_id;
}

void nvgft_set_render_mode(NVGFontSystem* sys, NVGFTRenderMode mode) {
	nvgft__getState(sys)->render_mode = mode;
}

void nvgft_set_texture_callback(NVGFontSystem* sys,
                                  NVGFTTextureUpdateFunc callback, void* uptr) {
	sys->texture_callback = callback;
	sys->texture_uptr = uptr;
}

// Atlas packing (simple row-based)
static int nvgft__pack_glyph(NVGFontSystem* sys, int gw, int gh, short* x, short* y) {
	int best_h = sys->atlas_height;
	int best_i = -1;

	// Find best row
	for (int i = 0; i < sys->nrows; i++) {
		if (sys->rows[i].x + gw <= sys->atlas_width && sys->rows[i].h >= gh && sys->rows[i].h < best_h) {
			best_h = sys->rows[i].h;
			best_i = i;
		}
	}

	// No space, try new row
	if (best_i == -1) {
		if (sys->nrows >= 256) return 0;  // Out of rows

		int py = 0;
		if (sys->nrows > 0) {
			py = sys->rows[sys->nrows - 1].y + sys->rows[sys->nrows - 1].h;
		}

		if (py + gh > sys->atlas_height) return 0;  // Out of space

		sys->rows[sys->nrows].x = gw;
		sys->rows[sys->nrows].y = py;
		sys->rows[sys->nrows].h = gh;
		*x = 0;
		*y = py;
		sys->nrows++;
		return 1;
	}

	// Pack into found row
	*x = sys->rows[best_i].x;
	*y = sys->rows[best_i].y;
	sys->rows[best_i].x += gw;

	return 1;
}

// Text iteration
void nvgft_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                           float x, float y, const char* str, const char* end) {
	if (end == NULL) end = str + strlen(str);

	iter->str = str;
	iter->end = end;
	iter->x = x;
	iter->y = y;
	iter->prev_glyph_index = -1;

	NVGFTState* state = nvgft__getState(sys);
	iter->font_id = state->font_id;
	iter->spacing = state->spacing;
	iter->internal = NULL;
}

int nvgft_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter, NVGFTQuad* quad) {
	if (iter->str >= iter->end) return 0;
	if (iter->font_id < 0 || iter->font_id >= sys->font_count) return 0;

	NVGFTFont* font = &sys->fonts[iter->font_id];

	// Decode UTF-8
	uint32_t codepoint = nvgft__decode_utf8(&iter->str);
	if (codepoint == 0) return nvgft_text_iter_next(sys, iter, quad);

	// Get glyph index via charmap cache
	FT_UInt glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache, font->face_id, -1, codepoint);

	// Try fallback if glyph not found
	if (glyph_index == 0 && font->fallback >= 0) {
		font = &sys->fonts[font->fallback];
		glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache, font->face_id, -1, codepoint);
	}

	if (glyph_index == 0) {
		// Skip missing glyph
		return nvgft_text_iter_next(sys, iter, quad);
	}

	// TODO: Get glyph from FTC cache, pack into atlas, return quad
	// This is where we need to integrate atlas packing with FTC
	// For now, return placeholder

	quad->x0 = iter->x;
	quad->y0 = iter->y;
	quad->x1 = iter->x + 16;
	quad->y1 = iter->y + 16;
	quad->s0 = 0;
	quad->t0 = 0;
	quad->s1 = 1;
	quad->t1 = 1;

	iter->x += 16 + iter->spacing;
	iter->prev_glyph_index = glyph_index;

	return 1;
}

// Metrics (TODO: implement)
float nvgft_text_bounds(NVGFontSystem* sys, float x, float y,
                         const char* string, const char* end, float* bounds) {
	(void)sys; (void)string; (void)end;  // TODO
	if (bounds) {
		bounds[0] = x;
		bounds[1] = y;
		bounds[2] = x + 100;
		bounds[3] = y + 20;
	}
	return x + 100;
}

void nvgft_vert_metrics(NVGFontSystem* sys, float* ascender, float* descender, float* lineh) {
	(void)sys;  // TODO
	if (ascender) *ascender = 12;
	if (descender) *descender = -4;
	if (lineh) *lineh = 16;
}

void nvgft_line_bounds(NVGFontSystem* sys, float y, float* miny, float* maxy) {
	(void)sys;  // TODO
	if (miny) *miny = y - 12;
	if (maxy) *maxy = y + 4;
}

// Atlas management
void nvgft_get_atlas_size(NVGFontSystem* sys, int* width, int* height) {
	if (width) *width = sys->atlas_width;
	if (height) *height = sys->atlas_height;
}

int nvgft_reset_atlas(NVGFontSystem* sys, int width, int height) {
	sys->atlas_width = width;
	sys->atlas_height = height;
	sys->nrows = 0;

	// Clear glyph hash table
	for (int i = 0; i < NVGFT_HASH_LUT_SIZE; i++) {
		NVGFTGlyph* g = sys->glyphs[i];
		while (g) {
			NVGFTGlyph* next = NULL;
			// TODO: implement linked list properly
			free(g);
			g = next;
		}
		sys->glyphs[i] = NULL;
	}
	sys->glyph_count = 0;

	return 1;
}

int nvgft_expand_atlas(NVGFontSystem* sys, int width, int height) {
	// TODO: handle atlas expansion (need to preserve existing glyphs)
	return nvgft_reset_atlas(sys, width, height);
}
