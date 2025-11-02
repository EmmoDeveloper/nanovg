#include "nvg_freetype.h"
#include "vknvg_msdf.h"
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
#include FT_GLYPH_H
#include FT_COLOR_H
#include FT_TRUETYPE_TABLES_H
#include FT_MULTIPLE_MASTERS_H

#include <hb.h>
#include <hb-ft.h>
#include <fribidi.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

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
	int msdf_mode;  // MSDF rendering mode: 0=off, 1=SDF, 2=MSDF
	FTC_FaceID face_id;  // Pointer to this struct (used by FTC)
	FT_Face hb_face;  // Fresh FT_Face for HarfBuzz (not cached)
	hb_font_t* hb_font;  // HarfBuzz font (created on demand)
	unsigned char has_colr;  // 0 = not checked, 1 = has COLR, 2 = no COLR
	FT_Face cairo_face;  // Cached FT_Face for Cairo color emoji rendering
	cairo_font_face_t* cairo_font_face;  // Cached Cairo font face
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
	unsigned char is_color;  // 1 if this is a color emoji (RGBA), 0 if grayscale
	struct NVGFTGlyph* next;  // Hash chain
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
	int kerning_enabled;
	int hinting;  // Font hinting mode (0=default, 1=none, 2=light, 3=full)
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

	// HarfBuzz buffer (reusable)
	hb_buffer_t* hb_buffer;

	// Buffer pool for RGBA conversions (avoid malloc/free churn)
	unsigned char* rgba_pool;
	int rgba_pool_size;

	// OpenType features
	hb_feature_t* features;
	int feature_count;
	int feature_capacity;
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

// Helper function to get FreeType load flags based on hinting mode
static FT_Int32 nvgft__get_load_flags(int hinting) {
	FT_Int32 flags = FT_LOAD_DEFAULT;

	switch (hinting) {
		case 0:  // NVG_HINTING_DEFAULT
			flags = FT_LOAD_DEFAULT;
			break;
		case 1:  // NVG_HINTING_NONE
			flags = FT_LOAD_NO_HINTING;
			break;
		case 2:  // NVG_HINTING_LIGHT
			flags = FT_LOAD_TARGET_LIGHT;
			break;
		case 3:  // NVG_HINTING_FULL
			flags = FT_LOAD_TARGET_NORMAL;
			break;
		default:
			flags = FT_LOAD_DEFAULT;
			break;
	}

	return flags;
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

	// Initialize HarfBuzz buffer
	sys->hb_buffer = hb_buffer_create();
	if (!sys->hb_buffer) {
		fprintf(stderr, "Failed to create HarfBuzz buffer\n");
		FTC_Manager_Done(sys->cache_manager);
		FT_Done_FreeType(sys->library);
		free(sys);
		return NULL;
	}

	// Initial state
	sys->nstates = 1;
	sys->states[0].font_id = -1;
	sys->states[0].size = 16.0f;
	sys->states[0].spacing = 0.0f;
	sys->states[0].blur = 0.0f;
	sys->states[0].align = NVGFT_ALIGN_LEFT | NVGFT_ALIGN_BASELINE;
	sys->states[0].render_mode = NVGFT_RENDER_NORMAL;
	sys->states[0].kerning_enabled = 1;  // Enabled by default
	sys->states[0].hinting = 0;  // Default hinting

	// Initialize feature storage
	sys->features = NULL;
	sys->feature_count = 0;
	sys->feature_capacity = 0;

	return sys;
}

void nvgft_destroy(NVGFontSystem* sys) {
	if (!sys) return;

	// Free HarfBuzz fonts and fresh FT_Faces
	for (int i = 0; i < sys->font_count; i++) {
		if (sys->fonts[i].hb_font) {
			hb_font_destroy(sys->fonts[i].hb_font);
		}
		if (sys->fonts[i].hb_face) {
			FT_Done_Face(sys->fonts[i].hb_face);
		}
		if (sys->fonts[i].cairo_font_face) {
			cairo_font_face_destroy(sys->fonts[i].cairo_font_face);
		}
		if (sys->fonts[i].cairo_face) {
			FT_Done_Face(sys->fonts[i].cairo_face);
		}
		if (sys->fonts[i].data && sys->fonts[i].free_data) {
			free(sys->fonts[i].data);
		}
	}

	// Free HarfBuzz buffer
	if (sys->hb_buffer) {
		hb_buffer_destroy(sys->hb_buffer);
	}

	// Free RGBA pool
	if (sys->rgba_pool) {
		free(sys->rgba_pool);
	}

	// Free OpenType features
	if (sys->features) {
		free(sys->features);
	}

	// Free glyph hash table
	for (int i = 0; i < NVGFT_HASH_LUT_SIZE; i++) {
		NVGFTGlyph* g = sys->glyphs[i];
		while (g) {
			NVGFTGlyph* next = g->next;
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
	font->hb_face = NULL;
	font->hb_font = NULL;
	font->has_colr = 0;

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
	font->hb_face = NULL;
	font->hb_font = NULL;
	font->has_colr = 0;

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

void nvgft_set_kerning(NVGFontSystem* sys, int enabled) {
	nvgft__getState(sys)->kerning_enabled = enabled;
}

void nvgft_set_hinting(NVGFontSystem* sys, int hinting) {
	nvgft__getState(sys)->hinting = hinting;
}

void nvgft_set_font_msdf(NVGFontSystem* sys, int font_id, int msdf_mode) {
	if (font_id >= 0 && font_id < sys->font_count) {
		sys->fonts[font_id].msdf_mode = msdf_mode;
	}
}

void nvgft_set_texture_callback(NVGFontSystem* sys,
                                  NVGFTTextureUpdateFunc callback, void* uptr) {
	sys->texture_callback = callback;
	sys->texture_uptr = uptr;
}

// Get pooled RGBA buffer (grows as needed)
static unsigned char* nvgft__get_rgba_buffer(NVGFontSystem* sys, int required_size) {
	if (required_size > sys->rgba_pool_size) {
		// Grow pool (use 2x growth strategy)
		int new_size = required_size * 2;
		unsigned char* new_pool = (unsigned char*)realloc(sys->rgba_pool, new_size);
		if (!new_pool) return NULL;
		sys->rgba_pool = new_pool;
		sys->rgba_pool_size = new_size;
	}
	return sys->rgba_pool;
}

// Glyph hash function
static unsigned int nvgft__hash_glyph(uint32_t codepoint, int font_id, int size) {
	unsigned int h = codepoint;
	h ^= (unsigned int)font_id << 8;
	h ^= (unsigned int)size << 16;
	return h % NVGFT_HASH_LUT_SIZE;
}

// Find glyph in hash table
static NVGFTGlyph* nvgft__find_glyph(NVGFontSystem* sys, uint32_t codepoint, int font_id, int size) {
	unsigned int h = nvgft__hash_glyph(codepoint, font_id, size);
	NVGFTGlyph* g = sys->glyphs[h];
	while (g) {
		if (g->codepoint == codepoint && g->font_id == font_id && g->size == size) {
			return g;
		}
		g = g->next;
	}
	return NULL;
}

// Add glyph to hash table
static NVGFTGlyph* nvgft__add_glyph(NVGFontSystem* sys, uint32_t codepoint, int font_id, int size) {
	unsigned int h = nvgft__hash_glyph(codepoint, font_id, size);
	NVGFTGlyph* g = (NVGFTGlyph*)malloc(sizeof(NVGFTGlyph));
	if (!g) return NULL;

	memset(g, 0, sizeof(NVGFTGlyph));
	g->codepoint = codepoint;
	g->font_id = font_id;
	g->size = size;
	g->next = sys->glyphs[h];
	sys->glyphs[h] = g;
	sys->glyph_count++;

	return g;
}

// Check if font has COLR table (works for both v0 and v1)
static int nvgft__has_colr_table(FT_Face face) {
	FT_ULong length = 0;
	FT_Load_Sfnt_Table(face, FT_MAKE_TAG('C','O','L','R'), 0, NULL, &length);
	return length > 0;
}

// Render color emoji using Cairo (required for COLR v1)
// FreeType's FT_LOAD_COLOR only supports COLR v0, not v1
// Returns RGBA bitmap (4 bytes per pixel) or NULL on failure
static unsigned char* nvgft__render_color_emoji(NVGFontSystem* sys, NVGFTFont* font,
                                                  FT_UInt glyph_index, int pixel_size,
                                                  int* out_width, int* out_height,
                                                  int* out_left, int* out_top) {
	// Reuse cached FT_Face for Cairo (create once per font)
	if (!font->cairo_face) {
		FT_Error err;
		if (font->data) {
			err = FT_New_Memory_Face(sys->library, font->data, font->data_size, 0, &font->cairo_face);
		} else {
			err = FT_New_Face(sys->library, font->path, 0, &font->cairo_face);
		}
		if (err != 0) {
			return NULL;
		}
	}

	// Set pixel size on the cached face
	FT_Set_Pixel_Sizes(font->cairo_face, 0, pixel_size);

	// Load glyph to get metrics
	FT_Error err = FT_Load_Glyph(font->cairo_face, glyph_index, FT_LOAD_DEFAULT);
	if (err != 0) {
		return NULL;
	}

	// Step 1: Create or reuse cached Cairo font face
	if (!font->cairo_font_face) {
		font->cairo_font_face = cairo_ft_font_face_create_for_ft_face(font->cairo_face, FT_LOAD_COLOR);
	}
	cairo_font_face_t* font_face = font->cairo_font_face;

	// Set up font matrix (scale to pixel size)
	cairo_matrix_t font_matrix;
	cairo_matrix_init_scale(&font_matrix, pixel_size, pixel_size);

	cairo_matrix_t ctm;
	cairo_matrix_init_identity(&ctm);

	cairo_font_options_t* font_options = cairo_font_options_create();
	cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_NONE);
	cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_OFF);
	cairo_font_options_set_color_mode(font_options, CAIRO_COLOR_MODE_COLOR);

	// Create scaled font to measure extents
	cairo_scaled_font_t* scaled_font = cairo_scaled_font_create(
		font_face, &font_matrix, &ctm, font_options
	);

	// Get glyph extents
	cairo_glyph_t cairo_glyph;
	cairo_glyph.index = glyph_index;
	cairo_glyph.x = 0;
	cairo_glyph.y = 0;

	cairo_text_extents_t extents;
	cairo_scaled_font_glyph_extents(scaled_font, &cairo_glyph, 1, &extents);

	// Step 2: Create properly sized surface based on extents
	// In Cairo text extents:
	//   - width/height are the bounding box dimensions
	//   - x_bearing/y_bearing are offsets from origin to top-left of bounding box
	// We need a surface large enough for the bounding box plus padding
	int width = (int)(extents.width + 4);   // Bounding box width + padding
	int height = (int)(extents.height + 4);  // Bounding box height + padding

	// Ensure minimum size (for very small glyphs like spaces)
	if (width < 4) width = 4;
	if (height < 4) height = 4;

	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_scaled_font_destroy(scaled_font);
		cairo_font_options_destroy(font_options);
		// Note: Don't destroy font_face - it's cached
		return NULL;
	}

	cairo_t* cr = cairo_create(surface);

	// Step 3: Render glyph on properly sized surface
	cairo_set_scaled_font(cr, scaled_font);

	// Clear background
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	// Position glyph to avoid clipping
	// The glyph origin should be positioned so the bounding box starts at (2, 2)
	// Since bearings are offsets from origin to bounding box, we need: origin - bearing = 2
	cairo_glyph.x = 2 - extents.x_bearing;
	cairo_glyph.y = 2 - extents.y_bearing;

	cairo_show_glyphs(cr, &cairo_glyph, 1);

	// Check for errors
	cairo_status_t status = cairo_status(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		// Rendering failed
		cairo_scaled_font_destroy(scaled_font);
		cairo_font_options_destroy(font_options);
		// Note: Don't destroy font_face - it's cached
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return NULL;
	}

	// Cleanup Cairo objects (but not cached font_face)
	cairo_scaled_font_destroy(scaled_font);
	cairo_font_options_destroy(font_options);
	cairo_destroy(cr);

	// Extract RGBA data
	unsigned char* cairo_data = cairo_image_surface_get_data(surface);
	int cairo_stride = cairo_image_surface_get_stride(surface);

	// Allocate output buffer (caller will free this)
	unsigned char* rgba = (unsigned char*)malloc(width * height * 4);
	if (rgba) {
		// Cairo uses BGRA (premultiplied), convert to RGBA
		// Optimize with 32-bit operations where possible
		uint32_t* cairo32 = (uint32_t*)cairo_data;
		uint32_t* rgba32 = (uint32_t*)rgba;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int cairo_idx = y * (cairo_stride / 4) + x;
				int rgba_idx = y * width + x;

				// Cairo BGRA (premultiplied) → RGBA (straight alpha)
				uint32_t pixel = cairo32[cairo_idx];
				unsigned char b = (pixel >> 0) & 0xFF;
				unsigned char g = (pixel >> 8) & 0xFF;
				unsigned char r = (pixel >> 16) & 0xFF;
				unsigned char a = (pixel >> 24) & 0xFF;

				// Unpremultiply alpha if needed
				if (a > 0 && a < 255) {
					r = (r * 255) / a;
					g = (g * 255) / a;
					b = (b * 255) / a;
				}

				// Pack as RGBA: 0xAABBGGRR
				rgba32[rgba_idx] = r | (g << 8) | (b << 16) | (a << 24);
			}
		}
	}

	cairo_surface_destroy(surface);

	*out_width = width;
	*out_height = height;
	// Bearing values: offset from text origin to glyph bounding box (with padding accounted for)
	*out_left = (int)(extents.x_bearing - 2);
	*out_top = (int)(-extents.y_bearing + 2);

	return rgba;
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
	iter->next = str;
	iter->x = x;
	iter->y = y;
	iter->prev_glyph_index = -1;
	iter->codepoint = 0;

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
	const char* str_before = iter->str;
	uint32_t codepoint = nvgft__decode_utf8(&iter->str);
	iter->next = iter->str;  // Update next position
	iter->codepoint = codepoint;  // Store codepoint for text breaking

	if (codepoint == 0) {
		iter->str = str_before;  // Restore position before recursion
		return nvgft_text_iter_next(sys, iter, quad);
	}

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

	// Get current state for size
	NVGFTState* state = nvgft__getState(sys);
	int pixel_size = (int)(state->size + 0.5f);

	static int dbg_size = 0;
	if (dbg_size++ == 0) {
		printf("[nvg_freetype] state->size=%.2f, pixel_size=%d\n", state->size, pixel_size);
	}

	// Check if glyph is already in atlas
	NVGFTGlyph* glyph = nvgft__find_glyph(sys, codepoint, font->id, pixel_size);

	if (!glyph) {
		// Check if font has COLR table (color emoji)
		FT_Face face;
		if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) == 0) {
			// Cache COLR check per font
			if (font->has_colr == 0) {
				font->has_colr = nvgft__has_colr_table(face) ? 1 : 2;
			}

			if (font->has_colr == 1) {
				// Render color emoji using COLR (v0 or v1)
				int width, height, left, top;
				unsigned char* rgba = nvgft__render_color_emoji(sys, font, glyph_index,
				                                                  pixel_size, &width, &height,
				                                                  &left, &top);
				if (rgba && width > 0 && height > 0) {
					// Pack into atlas
					short atlas_x, atlas_y;
					if (nvgft__pack_glyph(sys, width, height, &atlas_x, &atlas_y)) {
						// Add to glyph cache
						glyph = nvgft__add_glyph(sys, codepoint, font->id, pixel_size);
						if (glyph) {
							glyph->x = atlas_x;
							glyph->y = atlas_y;
							glyph->width = (short)width;
							glyph->height = (short)height;
							glyph->offset_x = left;
							glyph->offset_y = top;
							glyph->is_color = 1;  // Mark as color glyph

							// Get advance from face
							if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) == 0) {
								glyph->advance_x = (short)(face->glyph->advance.x >> 6);
							}

							// Calculate UV coordinates
							glyph->u0 = (float)atlas_x / (float)sys->atlas_width;
							glyph->v0 = (float)atlas_y / (float)sys->atlas_height;
							glyph->u1 = (float)(atlas_x + width) / (float)sys->atlas_width;
							glyph->v1 = (float)(atlas_y + height) / (float)sys->atlas_height;

							// Upload RGBA to GPU (note: 4 bytes per pixel for color!)
							if (sys->texture_callback) {
								sys->texture_callback(sys->texture_uptr,
									atlas_x, atlas_y,
									width, height,
									rgba);
							}
						}
					}

					free(rgba);

					// Skip regular glyph rendering below
					if (glyph) goto glyph_ready;
				}
			}
		}

		// Regular grayscale glyph rendering
		// Setup FTC image type
		FTC_ImageTypeRec type;
		type.face_id = font->face_id;
		type.width = 0;
		type.height = pixel_size;
		type.flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;

		// Get glyph from FTC image cache
		FT_Glyph ft_glyph;
		if (FTC_ImageCache_Lookup(sys->image_cache, &type, glyph_index, &ft_glyph, NULL) != 0) {
			// Failed to load glyph
			return nvgft_text_iter_next(sys, iter, quad);
		}

		// Convert to bitmap glyph
		if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			// Skip non-bitmap glyphs
			return nvgft_text_iter_next(sys, iter, quad);
		}

		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
		FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

		static int dbg_bitmap = 0;
		if (dbg_bitmap++ == 0) {
			printf("[nvg_freetype shaped_iter] First bitmap: width=%d rows=%d, pixel_size=%d, left=%d top=%d\n",
				bitmap->width, bitmap->rows, pixel_size, bitmap_glyph->left, bitmap_glyph->top);
		}

		// Skip empty glyphs
		if (bitmap->width == 0 || bitmap->rows == 0) {
			// Add minimal glyph to cache
			glyph = nvgft__add_glyph(sys, codepoint, font->id, pixel_size);
			if (glyph) {
				glyph->width = 0;
				glyph->height = 0;
				glyph->offset_x = 0;
				glyph->offset_y = 0;
				glyph->advance_x = (short)(ft_glyph->advance.x >> 16);
			}
		} else {
			// Pack into atlas
			short atlas_x, atlas_y;
			if (!nvgft__pack_glyph(sys, bitmap->width, bitmap->rows, &atlas_x, &atlas_y)) {
				// Atlas full, skip this glyph
				return nvgft_text_iter_next(sys, iter, quad);
			}

			// Add to glyph cache
			glyph = nvgft__add_glyph(sys, codepoint, font->id, pixel_size);
			if (!glyph) {
				// Out of memory
				return nvgft_text_iter_next(sys, iter, quad);
			}

			glyph->x = atlas_x;
			glyph->y = atlas_y;
			glyph->width = (short)bitmap->width;
			glyph->height = (short)bitmap->rows;
			glyph->offset_x = bitmap_glyph->left;
			glyph->offset_y = bitmap_glyph->top;
			glyph->advance_x = (short)(ft_glyph->advance.x >> 16);
			glyph->is_color = 0;  // Grayscale glyph

			// Calculate normalized UV coordinates
			glyph->u0 = (float)atlas_x / (float)sys->atlas_width;
			glyph->v0 = (float)atlas_y / (float)sys->atlas_height;
			glyph->u1 = (float)(atlas_x + bitmap->width) / (float)sys->atlas_width;
			glyph->v1 = (float)(atlas_y + bitmap->rows) / (float)sys->atlas_height;

			// Upload to GPU texture if callback is set
			// Convert grayscale to RGBA format (atlas is RGBA to support color emoji)
			if (sys->texture_callback) {
				int pixel_count = bitmap->width * bitmap->rows;
				unsigned char* rgba = nvgft__get_rgba_buffer(sys, pixel_count * 4);
				if (rgba) {
					// Convert grayscale alpha to RGBA (white with alpha)
					// Optimize with 32-bit writes (4 bytes at once)
					uint32_t* rgba32 = (uint32_t*)rgba;
					for (int i = 0; i < pixel_count; i++) {
						// Pack RGBA as single 32-bit value: 0xAABBGGRR
						uint32_t alpha = bitmap->buffer[i];
						rgba32[i] = 0x00FFFFFF | (alpha << 24);  // R=255, G=255, B=255, A=alpha
					}
					sys->texture_callback(sys->texture_uptr,
						atlas_x, atlas_y,
						bitmap->width, bitmap->rows,
						rgba);
					// Note: buffer is pooled, not freed
				}
			}
		}
	}

glyph_ready:
	// Apply kerning if available and enabled
	if (iter->prev_glyph_index >= 0 && state->kerning_enabled) {
		FT_Face face;
		if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) == 0) {
			if (FT_HAS_KERNING(face)) {
				FT_Vector kerning;
				FT_Get_Kerning(face, iter->prev_glyph_index, glyph_index,
					FT_KERNING_DEFAULT, &kerning);
				iter->x += kerning.x / 64.0f;
			}
		}
	}

	// Build output quad
	if (glyph && glyph->width > 0 && glyph->height > 0) {
		// Apply vertical alignment
		NVGFTState* state = nvgft__getState(sys);
		int align = state->align;
		float baseline_y = iter->y;
		if (align & NVGFT_ALIGN_TOP) {
			baseline_y += state->size;
		} else if (align & NVGFT_ALIGN_MIDDLE) {
			baseline_y += state->size * 0.5f;
		}

		quad->x0 = iter->x + glyph->offset_x;
		quad->y0 = baseline_y - glyph->offset_y;
		quad->x1 = quad->x0 + glyph->width;
		quad->y1 = quad->y0 + glyph->height;
		quad->s0 = glyph->u0;
		quad->t0 = glyph->v0;
		quad->s1 = glyph->u1;
		quad->t1 = glyph->v1;
	} else {
		// Empty glyph
		quad->x0 = iter->x;
		quad->y0 = iter->y;
		quad->x1 = iter->x;
		quad->y1 = iter->y;
		quad->s0 = 0;
		quad->t0 = 0;
		quad->s1 = 0;
		quad->t1 = 0;
	}

	// Advance cursor
	if (glyph) {
		iter->x += glyph->advance_x + iter->spacing;
	}
	iter->prev_glyph_index = glyph_index;

	return 1;
}

// Metrics
float nvgft_text_bounds(NVGFontSystem* sys, float x, float y,
                         const char* string, const char* end, float* bounds) {
	NVGFTState* state = nvgft__getState(sys);
	if (state->font_id < 0 || state->font_id >= sys->font_count) {
		if (bounds) {
			bounds[0] = x;
			bounds[1] = y;
			bounds[2] = x;
			bounds[3] = y;
		}
		return x;
	}

	NVGFTFont* font = &sys->fonts[state->font_id];
	int pixel_size = (int)(state->size + 0.5f);

	if (end == NULL) end = string + strlen(string);

	float minx = x, maxx = x;
	float miny = y, maxy = y;
	float advance_x = x;
	FT_UInt prev_glyph_index = 0;
	int is_first_glyph = 1;

	// Setup image type for FTC cache - must use RENDER to get bitmap
	FTC_ImageTypeRec type;
	type.face_id = font->face_id;
	type.width = pixel_size;
	type.height = pixel_size;
	type.flags = FT_LOAD_RENDER;

	// Get face for kerning
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
		if (bounds) {
			bounds[0] = x;
			bounds[1] = y;
			bounds[2] = x;
			bounds[3] = y;
		}
		return x;
	}

	static int dbg_format = 0;
	const char* str = string;
	while (str < end) {
		uint32_t codepoint = nvgft__decode_utf8(&str);
		if (codepoint == 0) continue;

		// Get glyph index
		FT_UInt glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache, font->face_id, -1, codepoint);
		if (glyph_index == 0) continue;

		// Load glyph via cache
		FT_Glyph ft_glyph;
		if (FTC_ImageCache_Lookup(sys->image_cache, &type, glyph_index, &ft_glyph, NULL) != 0) {
			if (dbg_format++ == 0) printf("[nvgft_text_bounds] FTC_ImageCache_Lookup failed\n");
			continue;
		}

		if (dbg_format++ == 0) printf("[nvgft_text_bounds] First glyph format: %c%c%c%c\n",
			(ft_glyph->format >> 24) & 0xFF,
			(ft_glyph->format >> 16) & 0xFF,
			(ft_glyph->format >> 8) & 0xFF,
			ft_glyph->format & 0xFF);

		// Must be bitmap glyph for bounds calculation
		if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			advance_x += (ft_glyph->advance.x / 65536.0f) + state->spacing;
			prev_glyph_index = glyph_index;
			continue;
		}

		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
		FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

		// Apply kerning if enabled
		if (prev_glyph_index != 0 && state->kerning_enabled && FT_HAS_KERNING(face)) {
			FT_Vector kerning;
			FT_Get_Kerning(face, prev_glyph_index, glyph_index, FT_KERNING_DEFAULT, &kerning);
			advance_x += kerning.x / 64.0f;
		}

		// Calculate X bounds only (Y bounds are set by nvgft_line_bounds in wrapper)
		// Match the actual rendering position: advance_x + bearing
		float x0 = advance_x + bitmap_glyph->left;
		float x1 = x0 + bitmap->width;

		if (x0 < minx) minx = x0;
		if (x1 > maxx) maxx = x1;

		advance_x += (ft_glyph->advance.x / 65536.0f) + state->spacing;
		prev_glyph_index = glyph_index;
	}

	if (bounds) {
		bounds[0] = minx;
		bounds[1] = miny;
		bounds[2] = maxx;
		bounds[3] = maxy;
	}

	return advance_x;
}

float nvgft_text_bounds_shaped(NVGFontSystem* sys, float x, float y,
                                 const char* string, const char* end, float* bounds) {
	NVGFTState* state = nvgft__getState(sys);
	if (state->font_id < 0 || state->font_id >= sys->font_count) {
		if (bounds) {
			bounds[0] = x;
			bounds[1] = y;
			bounds[2] = x;
			bounds[3] = y;
		}
		return x;
	}

	if (end == NULL) end = string + strlen(string);
	if (string == end) {
		if (bounds) {
			bounds[0] = x;
			bounds[1] = y;
			bounds[2] = x;
			bounds[3] = y;
		}
		return x;
	}

	// Initialize iterator with HarfBuzz shaping (auto direction, no language)
	NVGFTTextIter iter;
	nvgft_shaped_text_iter_init(sys, &iter, x, y, string, end, 0, NULL);

	float minx = x, maxx = x;
	float miny = y, maxy = y;
	float advance_x = x;

	// Iterate through shaped glyphs
	NVGFTQuad quad;
	while (nvgft_text_iter_next(sys, &iter, &quad)) {
		if (quad.x0 < minx) minx = quad.x0;
		if (quad.x1 > maxx) maxx = quad.x1;
		if (quad.y0 < miny) miny = quad.y0;
		if (quad.y1 > maxy) maxy = quad.y1;
	}

	// Get final advance from iterator (same as nvgText does)
	advance_x = iter.x;

	nvgft_text_iter_free(&iter);

	if (bounds) {
		bounds[0] = minx;
		bounds[1] = miny;
		bounds[2] = maxx;
		bounds[3] = maxy;
	}

	return advance_x;
}

void nvgft_vert_metrics(NVGFontSystem* sys, float* ascender, float* descender, float* lineh) {
	NVGFTState* state = nvgft__getState(sys);
	if (state->font_id < 0 || state->font_id >= sys->font_count) {
		if (ascender) *ascender = 0;
		if (descender) *descender = 0;
		if (lineh) *lineh = 0;
		return;
	}

	NVGFTFont* font = &sys->fonts[state->font_id];
	int pixel_size = (int)(state->size + 0.5f);

	// Load a glyph to force FTC to set up the size
	// Use space character (codepoint 32) as it's universally available
	FT_UInt glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache, font->face_id, -1, 32);
	if (glyph_index == 0) glyph_index = 1;  // Fallback to .notdef

	FTC_ImageTypeRec type;
	type.face_id = font->face_id;
	type.width = 0;
	type.height = pixel_size;
	type.flags = FT_LOAD_DEFAULT;

	FT_Glyph ft_glyph;
	if (FTC_ImageCache_Lookup(sys->image_cache, &type, glyph_index, &ft_glyph, NULL) != 0) {
		if (ascender) *ascender = 0;
		if (descender) *descender = 0;
		if (lineh) *lineh = 0;
		return;
	}

	// Now get the face with the size set
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
		if (ascender) *ascender = 0;
		if (descender) *descender = 0;
		if (lineh) *lineh = 0;
		return;
	}

	// Calculate metrics based on units_per_EM and pixel size
	float scale = (float)pixel_size / (float)face->units_per_EM;
	if (ascender) *ascender = face->ascender * scale;
	if (descender) *descender = face->descender * scale;
	if (lineh) *lineh = face->height * scale;
}

void nvgft_line_bounds(NVGFontSystem* sys, float y, float* miny, float* maxy) {
	NVGFTState* state = nvgft__getState(sys);
	float ascender, descender, lineh;
	nvgft_vert_metrics(sys, &ascender, &descender, &lineh);

	// Apply vertical alignment to convert y to baseline
	int align = state->align;
	float baseline_y = y;
	if (align & NVGFT_ALIGN_TOP) {
		baseline_y += state->size;
	} else if (align & NVGFT_ALIGN_MIDDLE) {
		baseline_y += state->size * 0.5f;
	}

	if (miny) *miny = baseline_y - ascender;
	if (maxy) *maxy = baseline_y - descender;
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
			NVGFTGlyph* next = g->next;
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
// Shaped text iteration with HarfBuzz + FriBidi
// This file is appended to nvg_freetype.c

// Helper: Get HarfBuzz font for a FreeType face
static hb_font_t* nvgft__get_hb_font(NVGFontSystem* sys, int font_id) {
	if (font_id < 0 || font_id >= sys->font_count) return NULL;

	NVGFTFont* font = &sys->fonts[font_id];

	// Create HarfBuzz font if not already created
	if (!font->hb_font) {
		// Create fresh FT_Face for HarfBuzz (cached faces don't work)
		if (!font->hb_face) {
			FT_Error error;
			if (font->data) {
				error = FT_New_Memory_Face(sys->library, font->data, font->data_size, 0, &font->hb_face);
			} else {
				error = FT_New_Face(sys->library, font->path, 0, &font->hb_face);
			}
			if (error != 0) {
				return NULL;
			}
		}

		// Create HarfBuzz font from fresh FreeType face
		font->hb_font = hb_ft_font_create(font->hb_face, NULL);
	}

	return font->hb_font;
}

// Shaped text iteration initialization
void nvgft_shaped_text_iter_init(NVGFontSystem* sys, NVGFTTextIter* iter,
                                  float x, float y, const char* str, const char* end,
                                  int direction, const char* language) {
	memset(iter, 0, sizeof(*iter));

	if (!str || !sys) return;
	if (end == NULL) end = str + strlen(str);

	int length = end - str;
	if (length <= 0) return;

	iter->str = str;
	iter->end = end;
	iter->x = x;
	iter->y = y;

	NVGFTState* state = nvgft__getState(sys);
	iter->font_id = state->font_id;
	iter->spacing = state->spacing;

	// Step 1: Apply BiDi reordering if needed
	FriBidiChar* unicode_str = NULL;
	FriBidiStrIndex unicode_len = 0;
	char* visual_str = NULL;

	// Convert UTF-8 to Unicode (FriBidi uses UTF-32)
	unicode_len = length;  // Approximate
	unicode_str = (FriBidiChar*)malloc(sizeof(FriBidiChar) * (unicode_len + 1));
	if (!unicode_str) return;

	// Decode UTF-8 to UTF-32
	const char* p = str;
	FriBidiStrIndex idx = 0;
	while (p < end && idx < unicode_len) {
		uint32_t cp = nvgft__decode_utf8(&p);
		if (cp == 0) break;
		unicode_str[idx++] = (FriBidiChar)cp;
	}
	unicode_len = idx;

	// Detect paragraph direction
	FriBidiParType base_dir = FRIBIDI_PAR_ON;  // Auto-detect
	if (direction == 1) base_dir = FRIBIDI_PAR_LTR;
	else if (direction == 2) base_dir = FRIBIDI_PAR_RTL;

	// Apply BiDi algorithm
	FriBidiChar* visual_unicode = (FriBidiChar*)malloc(sizeof(FriBidiChar) * (unicode_len + 1));
	if (!visual_unicode) {
		free(unicode_str);
		return;
	}

	// Simple BiDi - no embedding levels for now
	FriBidiLevel max_level = fribidi_log2vis(
		unicode_str, unicode_len, &base_dir,
		visual_unicode, NULL, NULL, NULL
	);

	if (max_level == 0) {
		// BiDi failed, use original order
		memcpy(visual_unicode, unicode_str, unicode_len * sizeof(FriBidiChar));
	}

	free(unicode_str);

	// Convert back to UTF-8
	int visual_utf8_len = unicode_len * 4 + 1;  // Max UTF-8 bytes
	visual_str = (char*)malloc(visual_utf8_len);
	if (!visual_str) {
		free(visual_unicode);
		return;
	}

	// Encode UTF-32 to UTF-8
	char* out = visual_str;
	for (FriBidiStrIndex i = 0; i < unicode_len; i++) {
		uint32_t cp = visual_unicode[i];
		if (cp < 0x80) {
			*out++ = (char)cp;
		} else if (cp < 0x800) {
			*out++ = (char)(0xc0 | (cp >> 6));
			*out++ = (char)(0x80 | (cp & 0x3f));
		} else if (cp < 0x10000) {
			*out++ = (char)(0xe0 | (cp >> 12));
			*out++ = (char)(0x80 | ((cp >> 6) & 0x3f));
			*out++ = (char)(0x80 | (cp & 0x3f));
		} else {
			*out++ = (char)(0xf0 | (cp >> 18));
			*out++ = (char)(0x80 | ((cp >> 12) & 0x3f));
			*out++ = (char)(0x80 | ((cp >> 6) & 0x3f));
			*out++ = (char)(0x80 | (cp & 0x3f));
		}
	}
	*out = '\0';
	int visual_str_len = out - visual_str;

	free(visual_unicode);

	// Step 2: Shape with HarfBuzz
	hb_font_t* hb_font = nvgft__get_hb_font(sys, state->font_id);
	if (!hb_font) {
		free(visual_str);
		return;
	}

	// Set font size
	int pixel_size = (int)(state->size + 0.5f);
	hb_font_set_scale(hb_font, pixel_size * 64, pixel_size * 64);

	// Clear and configure buffer
	hb_buffer_clear_contents(sys->hb_buffer);

	// Set direction
	if (base_dir == FRIBIDI_PAR_RTL) {
		hb_buffer_set_direction(sys->hb_buffer, HB_DIRECTION_RTL);
	} else {
		hb_buffer_set_direction(sys->hb_buffer, HB_DIRECTION_LTR);
	}

	// Set script (auto-detect for now)
	hb_buffer_set_script(sys->hb_buffer, HB_SCRIPT_INVALID);
	hb_buffer_guess_segment_properties(sys->hb_buffer);

	// Set language
	if (language) {
		hb_buffer_set_language(sys->hb_buffer, hb_language_from_string(language, -1));
	}

	// Add text
	hb_buffer_add_utf8(sys->hb_buffer, visual_str, visual_str_len, 0, visual_str_len);

	// Shape with features
	hb_shape(hb_font, sys->hb_buffer, sys->features, sys->feature_count);

	// Get shaped glyphs
	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(sys->hb_buffer, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(sys->hb_buffer, &glyph_count);

	// Allocate shaped glyphs array
	iter->shaped_glyphs = (NVGFTShapedGlyph*)malloc(sizeof(NVGFTShapedGlyph) * glyph_count);
	if (!iter->shaped_glyphs) {
		free(visual_str);
		return;
	}

	// Convert HarfBuzz output to our format
	for (unsigned int i = 0; i < glyph_count; i++) {
		iter->shaped_glyphs[i].glyph_index = glyph_info[i].codepoint;
		iter->shaped_glyphs[i].x_offset = glyph_pos[i].x_offset / 64.0f;
		iter->shaped_glyphs[i].y_offset = glyph_pos[i].y_offset / 64.0f;
		iter->shaped_glyphs[i].x_advance = glyph_pos[i].x_advance / 64.0f;
		iter->shaped_glyphs[i].y_advance = glyph_pos[i].y_advance / 64.0f;
		iter->shaped_glyphs[i].cluster = glyph_info[i].cluster;
	}

	iter->num_shaped_glyphs = glyph_count;
	iter->current_glyph = 0;
	iter->shaped_x = x;
	iter->shaped_y = y;

	free(visual_str);
}

// Shaped text iteration - get next glyph
int nvgft_shaped_text_iter_next(NVGFontSystem* sys, NVGFTTextIter* iter, NVGFTQuad* quad) {
	if (!iter || !iter->shaped_glyphs || iter->current_glyph >= iter->num_shaped_glyphs) {
		return 0;
	}

	NVGFTState* state = nvgft__getState(sys);
	NVGFTShapedGlyph* sg = &iter->shaped_glyphs[iter->current_glyph];
	iter->current_glyph++;

	// Render glyph using glyph index
	// Note: We use glyph_index as "codepoint" in cache since HarfBuzz gives us glyph IDs
	NVGFTFont* font = &sys->fonts[state->font_id];
	int pixel_size = (int)(state->size + 0.5f);

	// Check cache using glyph_index as lookup key
	NVGFTGlyph* glyph = nvgft__find_glyph(sys, sg->glyph_index, font->id, pixel_size);

	if (!glyph) {
		// Need to render glyph by index
		FTC_ImageTypeRec type;
		type.face_id = font->face_id;
		type.width = 0;
		type.height = pixel_size;
		type.flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;

		FT_Glyph ft_glyph;
		if (FTC_ImageCache_Lookup(sys->image_cache, &type, sg->glyph_index, &ft_glyph, NULL) != 0) {
			// Skip missing glyph
			iter->shaped_x += sg->x_advance;
			iter->shaped_y += sg->y_advance;
			return nvgft_shaped_text_iter_next(sys, iter, quad);
		}

		if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			iter->shaped_x += sg->x_advance;
			iter->shaped_y += sg->y_advance;
			return nvgft_shaped_text_iter_next(sys, iter, quad);
		}

		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
		FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

		if (bitmap->width == 0 || bitmap->rows == 0) {
			glyph = nvgft__add_glyph(sys, sg->glyph_index, font->id, pixel_size);
			if (glyph) {
				glyph->width = 0;
				glyph->height = 0;
			}
		} else {
			short atlas_x, atlas_y;
			if (!nvgft__pack_glyph(sys, bitmap->width, bitmap->rows, &atlas_x, &atlas_y)) {
				iter->shaped_x += sg->x_advance;
				iter->shaped_y += sg->y_advance;
				return nvgft_shaped_text_iter_next(sys, iter, quad);
			}

			glyph = nvgft__add_glyph(sys, sg->glyph_index, font->id, pixel_size);
			if (!glyph) {
				iter->shaped_x += sg->x_advance;
				iter->shaped_y += sg->y_advance;
				return nvgft_shaped_text_iter_next(sys, iter, quad);
			}

			glyph->x = atlas_x;
			glyph->y = atlas_y;
			glyph->width = (short)bitmap->width;
			glyph->height = (short)bitmap->rows;
			glyph->offset_x = bitmap_glyph->left;
			glyph->offset_y = bitmap_glyph->top;
			glyph->advance_x = (short)(ft_glyph->advance.x >> 16);

			glyph->u0 = (float)atlas_x / (float)sys->atlas_width;
			glyph->v0 = (float)atlas_y / (float)sys->atlas_height;
			glyph->u1 = (float)(atlas_x + bitmap->width) / (float)sys->atlas_width;
			glyph->v1 = (float)(atlas_y + bitmap->rows) / (float)sys->atlas_height;

			if (sys->texture_callback) {
				sys->texture_callback(sys->texture_uptr,
					atlas_x, atlas_y,
					bitmap->width, bitmap->rows,
					bitmap->buffer);
			}
		}
	}

	if (!glyph) {
		iter->shaped_x += sg->x_advance;
		iter->shaped_y += sg->y_advance;
		return nvgft_shaped_text_iter_next(sys, iter, quad);
	}

	// Build quad with offsets from HarfBuzz
	float x = iter->shaped_x + sg->x_offset;
	float y = iter->shaped_y + sg->y_offset;

	// Apply vertical alignment
	int align = state->align;
	float baseline_y = y;
	if (align & NVGFT_ALIGN_TOP) {
		baseline_y += state->size;
	} else if (align & NVGFT_ALIGN_MIDDLE) {
		baseline_y += state->size * 0.5f;
	}

	if (glyph && glyph->width > 0 && glyph->height > 0) {
		quad->x0 = x + glyph->offset_x;
		quad->y0 = baseline_y - glyph->offset_y;
		quad->x1 = quad->x0 + glyph->width;
		quad->y1 = quad->y0 + glyph->height;
		quad->s0 = glyph->u0;
		quad->t0 = glyph->v0;
		quad->s1 = glyph->u1;
		quad->t1 = glyph->v1;
	} else {
		quad->x0 = quad->y0 = quad->x1 = quad->y1 = 0;
		quad->s0 = quad->t0 = quad->s1 = quad->t1 = 0;
	}

	// Advance position
	iter->shaped_x += sg->x_advance;
	iter->shaped_y += sg->y_advance;

	return 1;
}

// Free iterator resources
void nvgft_text_iter_free(NVGFTTextIter* iter) {
	if (!iter) return;

	if (iter->shaped_glyphs) {
		free(iter->shaped_glyphs);
		iter->shaped_glyphs = NULL;
	}
}

// Rasterize a glyph for virtual atlas integration
// Returns RGBA pixel data (caller must free())
unsigned char* nvgft_rasterize_glyph(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                                       int pixel_size, int* width, int* height,
                                       int* bearing_x, int* bearing_y, int* advance_x)
{
	if (!sys || font_id < 0 || font_id >= sys->font_count) return NULL;

	NVGFTFont* font = &sys->fonts[font_id];

	// Get glyph index from codepoint
	FT_UInt glyph_index = FTC_CMapCache_Lookup(sys->cmap_cache, (FTC_FaceID)font,
	                                            -1, codepoint);
	if (glyph_index == 0) return NULL;  // Glyph not found

	// Setup FTC image type for rasterization
	FTC_ImageTypeRec type;
	type.face_id = (FTC_FaceID)font;
	type.width = 0;
	type.height = pixel_size;
	type.flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;

	// Get glyph from FTC image cache
	FT_Glyph ft_glyph;
	if (FTC_ImageCache_Lookup(sys->image_cache, &type, glyph_index, &ft_glyph, NULL) != 0) {
		return NULL;  // Failed to load glyph
	}

	// Convert to bitmap glyph
	if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
		return NULL;  // Not a bitmap glyph
	}

	FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)ft_glyph;
	FT_Bitmap* bitmap = &bitmap_glyph->bitmap;

	// Return glyph metrics
	if (width) *width = bitmap->width;
	if (height) *height = bitmap->rows;
	if (bearing_x) *bearing_x = bitmap_glyph->left;
	if (bearing_y) *bearing_y = bitmap_glyph->top;
	if (advance_x) *advance_x = (int)(ft_glyph->advance.x >> 16);

	// Handle empty glyphs
	if (bitmap->width == 0 || bitmap->rows == 0) {
		return NULL;
	}

	// Allocate RGBA buffer
	int pixel_count = bitmap->width * bitmap->rows;
	unsigned char* rgba = (unsigned char*)malloc(pixel_count * 4);
	if (!rgba) return NULL;

	// Convert grayscale to RGBA (white with alpha)
	for (int i = 0; i < pixel_count; i++) {
		rgba[i*4 + 0] = 255;  // R
		rgba[i*4 + 1] = 255;  // G
		rgba[i*4 + 2] = 255;  // B
		rgba[i*4 + 3] = bitmap->buffer[i];  // A (grayscale value)
	}

	return rgba;
}

// Glyph-level API implementations

int nvgft_get_glyph_metrics(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                              NVGFTGlyphMetrics* metrics)
{
	if (!sys || !metrics) return -1;
	if (font_id < 0 || font_id >= sys->font_count) return -1;

	NVGFTFont* font = &sys->fonts[font_id];
	NVGFTState* state = nvgft__getState(sys);
	int pixel_size = (int)(state->size + 0.5f);

	// Get FreeType face
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
		return -1;
	}

	// Get glyph index from codepoint
	FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
	if (glyph_index == 0) return -1;

	// Set pixel size
	if (FT_Set_Pixel_Sizes(face, 0, pixel_size) != 0) {
		return -1;
	}

	// Load glyph with hinting
	FT_Int32 load_flags = nvgft__get_load_flags(state->hinting);
	if (FT_Load_Glyph(face, glyph_index, load_flags) != 0) {
		return -1;
	}

	// Fill metrics
	metrics->glyphIndex = glyph_index;
	metrics->bearingX = face->glyph->metrics.horiBearingX / 64.0f;
	metrics->bearingY = face->glyph->metrics.horiBearingY / 64.0f;
	metrics->advanceX = face->glyph->metrics.horiAdvance / 64.0f;
	metrics->advanceY = face->glyph->metrics.vertAdvance / 64.0f;
	metrics->width = face->glyph->metrics.width / 64.0f;
	metrics->height = face->glyph->metrics.height / 64.0f;

	return 0;
}

float nvgft_get_kerning(NVGFontSystem* sys, int font_id,
                         uint32_t left_codepoint, uint32_t right_codepoint)
{
	if (!sys) return 0.0f;
	if (font_id < 0 || font_id >= sys->font_count) return 0.0f;

	NVGFTFont* font = &sys->fonts[font_id];

	// Get FreeType face
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
		return 0.0f;
	}

	// Check if font has kerning
	if (!FT_HAS_KERNING(face)) {
		return 0.0f;
	}

	// Get glyph indices
	FT_UInt left_glyph = FT_Get_Char_Index(face, left_codepoint);
	FT_UInt right_glyph = FT_Get_Char_Index(face, right_codepoint);

	if (left_glyph == 0 || right_glyph == 0) {
		return 0.0f;
	}

	// Get kerning vector
	FT_Vector kerning;
	if (FT_Get_Kerning(face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning) != 0) {
		return 0.0f;
	}

	return kerning.x / 64.0f;
}

int nvgft_render_glyph(NVGFontSystem* sys, int font_id, uint32_t codepoint,
                        float x, float y, NVGFTQuad* quad)
{
	if (!sys || !quad) return -1;
	if (font_id < 0 || font_id >= sys->font_count) return -1;

	NVGFTFont* font = &sys->fonts[font_id];

	// Get FreeType face
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
		return -1;
	}

	// Get glyph index
	FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
	if (glyph_index == 0) return -1;

	// Use render_glyph_index for the actual work
	return nvgft_render_glyph_index(sys, font_id, glyph_index, x, y, quad);
}

int nvgft_render_glyph_index(NVGFontSystem* sys, int font_id, int glyph_index,
                               float x, float y, NVGFTQuad* quad)
{
	if (!sys || !quad) return -1;
	if (font_id < 0 || font_id >= sys->font_count) return -1;

	NVGFTFont* font = &sys->fonts[font_id];
	NVGFTState* state = nvgft__getState(sys);
	int pixel_size = (int)(state->size + 0.5f);

	// Check if glyph is already in cache
	NVGFTGlyph* glyph = nvgft__find_glyph(sys, glyph_index, font->id, pixel_size);

	if (!glyph) {
		// Need to render glyph
		FT_Face face;
		if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) {
			return -1;
		}

		// Set pixel size
		if (FT_Set_Pixel_Sizes(face, 0, pixel_size) != 0) {
			return -1;
		}

		// Load glyph with hinting (or without for MSDF)
		FT_Int32 load_flags = (font->msdf_mode > 0) ? FT_LOAD_NO_BITMAP : nvgft__get_load_flags(state->hinting);
		if (FT_Load_Glyph(face, glyph_index, load_flags) != 0) {
			return -1;
		}

		FT_Bitmap* bitmap = NULL;
		unsigned char* msdf_buffer = NULL;

		// Check if MSDF mode is enabled for this font
		if (font->msdf_mode > 0) {
			// Generate MSDF/SDF from outline
			int msdf_size = pixel_size * 2;  // MSDF needs higher resolution
			int msdf_padding = pixel_size / 4;

			VKNVGmsdfParams params;
			params.width = msdf_size + msdf_padding * 2;
			params.height = msdf_size + msdf_padding * 2;
			params.stride = params.width * (font->msdf_mode == 2 ? 3 : 1);  // MSDF=3 channels, SDF=1
			params.range = pixel_size / 8.0f;
			params.scale = 1.0f;
			params.offsetX = msdf_padding;
			params.offsetY = msdf_padding;

			// Allocate buffer
			msdf_buffer = (unsigned char*)malloc(params.width * params.height * (font->msdf_mode == 2 ? 3 : 1));
			if (msdf_buffer) {
				if (font->msdf_mode == 2) {
					vknvg__generateMSDF(face->glyph, msdf_buffer, &params);
				} else {
					vknvg__generateSDF(face->glyph, msdf_buffer, &params);
				}

				// Create temporary bitmap structure
				static FT_Bitmap msdf_bitmap;
				msdf_bitmap.width = params.width;
				msdf_bitmap.rows = params.height;
				msdf_bitmap.pitch = params.stride;
				msdf_bitmap.buffer = msdf_buffer;
				msdf_bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
				bitmap = &msdf_bitmap;
			}
		} else {
			// Standard bitmap rendering
			FT_Render_Mode render_mode;
			switch (state->render_mode) {
				case NVGFT_RENDER_MONO: render_mode = FT_RENDER_MODE_MONO; break;
				case NVGFT_RENDER_LCD: render_mode = FT_RENDER_MODE_LCD; break;
				case NVGFT_RENDER_LCD_V: render_mode = FT_RENDER_MODE_LCD_V; break;
				default: render_mode = FT_RENDER_MODE_NORMAL; break;
			}

			if (FT_Render_Glyph(face->glyph, render_mode) != 0) {
				if (msdf_buffer) free(msdf_buffer);
				return -1;
			}

			bitmap = &face->glyph->bitmap;
		}

		// Handle empty glyphs
		if (bitmap->width == 0 || bitmap->rows == 0) {
			glyph = nvgft__add_glyph(sys, glyph_index, font->id, pixel_size);
			if (glyph) {
				glyph->width = 0;
				glyph->height = 0;
			}
		} else {
			// Pack glyph in atlas
			short atlas_x, atlas_y;
			if (!nvgft__pack_glyph(sys, bitmap->width, bitmap->rows, &atlas_x, &atlas_y)) {
				if (msdf_buffer) free(msdf_buffer);
				return -1;
			}

			glyph = nvgft__add_glyph(sys, glyph_index, font->id, pixel_size);
			if (!glyph) {
				if (msdf_buffer) free(msdf_buffer);
				return -1;
			}

			glyph->x = atlas_x;
			glyph->y = atlas_y;
			glyph->width = (short)bitmap->width;
			glyph->height = (short)bitmap->rows;
			glyph->offset_x = face->glyph->bitmap_left;
			glyph->offset_y = face->glyph->bitmap_top;
			glyph->advance_x = (short)(face->glyph->advance.x >> 6);

			glyph->u0 = (float)atlas_x / (float)sys->atlas_width;
			glyph->v0 = (float)atlas_y / (float)sys->atlas_height;
			glyph->u1 = (float)(atlas_x + bitmap->width) / (float)sys->atlas_width;
			glyph->v1 = (float)(atlas_y + bitmap->rows) / (float)sys->atlas_height;

			// Upload to texture
			if (sys->texture_callback) {
				sys->texture_callback(sys->texture_uptr,
					atlas_x, atlas_y,
					bitmap->width, bitmap->rows,
					bitmap->buffer);
			}
		}

		// Cleanup MSDF buffer if allocated
		if (msdf_buffer) {
			free(msdf_buffer);
		}
	}

	// Build quad from glyph
	if (!glyph || glyph->width == 0 || glyph->height == 0) {
		quad->x0 = quad->y0 = quad->x1 = quad->y1 = 0;
		quad->s0 = quad->t0 = quad->s1 = quad->t1 = 0;
		return 0;
	}

	// Apply glyph alignment
	int align = state->align;
	float rx = x;
	float ry = y;

	// Horizontal alignment is typically handled by caller
	// Vertical alignment
	if (align & NVGFT_ALIGN_TOP) {
		ry += state->size;
	} else if (align & NVGFT_ALIGN_MIDDLE) {
		ry += state->size * 0.5f;
	} else if (align & NVGFT_ALIGN_BOTTOM) {
		ry += 0;
	} else {
		// Baseline (default)
		ry += 0;
	}

	// Position glyph
	quad->x0 = rx + glyph->offset_x;
	quad->y0 = ry - glyph->offset_y;
	quad->x1 = quad->x0 + glyph->width;
	quad->y1 = quad->y0 + glyph->height;

	quad->s0 = glyph->u0;
	quad->t0 = glyph->v0;
	quad->s1 = glyph->u1;
	quad->t1 = glyph->v1;

	return 0;
}

// Font information queries
const char* nvgft_get_family_name(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return NULL;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return NULL;
	return face->family_name;
}

const char* nvgft_get_style_name(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return NULL;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return NULL;
	return face->style_name;
}

int nvgft_get_glyph_count(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return -1;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return -1;
	return (int)face->num_glyphs;
}

int nvgft_is_scalable(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return 0;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return 0;
	return FT_IS_SCALABLE(face) ? 1 : 0;
}

int nvgft_is_fixed_width(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return 0;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return 0;
	return FT_IS_FIXED_WIDTH(face) ? 1 : 0;
}

// Variable font support
int nvgft_is_variable(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return 0;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return 0;

	FT_MM_Var* mmvar = NULL;
	if (FT_Get_MM_Var(face, &mmvar) != 0) return 0;
	int is_var = (mmvar->num_axis > 0) ? 1 : 0;
	FT_Done_MM_Var(sys->library, mmvar);
	return is_var;
}

int nvgft_get_var_axis_count(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return 0;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return 0;

	FT_MM_Var* mmvar = NULL;
	if (FT_Get_MM_Var(face, &mmvar) != 0) return 0;
	int count = (int)mmvar->num_axis;
	FT_Done_MM_Var(sys->library, mmvar);
	return count;
}

int nvgft_get_var_axis(NVGFontSystem* sys, int font_id, int axis_index, NVGFTVarAxis* axis) {
	if (!sys || !axis || font_id < 0 || font_id >= sys->font_count) return -1;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return -1;

	FT_MM_Var* mmvar = NULL;
	if (FT_Get_MM_Var(face, &mmvar) != 0) return -1;

	if (axis_index < 0 || axis_index >= (int)mmvar->num_axis) {
		FT_Done_MM_Var(sys->library, mmvar);
		return -1;
	}

	FT_Var_Axis* ft_axis = &mmvar->axis[axis_index];

	// Copy name (truncate if too long)
	if (ft_axis->name) {
		strncpy(axis->name, ft_axis->name, 63);
		axis->name[63] = '\0';
	} else {
		axis->name[0] = '\0';
	}

	// Convert FT_Fixed (16.16) to float
	axis->minimum = (float)ft_axis->minimum / 65536.0f;
	axis->def = (float)ft_axis->def / 65536.0f;
	axis->maximum = (float)ft_axis->maximum / 65536.0f;
	axis->tag = ft_axis->tag;

	FT_Done_MM_Var(sys->library, mmvar);
	return 0;
}

int nvgft_set_var_design_coords(NVGFontSystem* sys, int font_id, const float* coords, int num_coords) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return -1;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return -1;

	// Reset to defaults if num_coords is 0
	if (num_coords == 0 || !coords) {
		if (FT_Set_Var_Design_Coordinates(face, 0, NULL) != 0) return -1;
		return 0;
	}

	// Convert float coords to FT_Fixed (16.16)
	FT_Fixed* ft_coords = (FT_Fixed*)malloc(num_coords * sizeof(FT_Fixed));
	if (!ft_coords) return -1;

	for (int i = 0; i < num_coords; i++) {
		ft_coords[i] = (FT_Fixed)(coords[i] * 65536.0f);
	}

	int result = FT_Set_Var_Design_Coordinates(face, num_coords, ft_coords);
	free(ft_coords);

	return (result == 0) ? 0 : -1;
}

int nvgft_get_var_design_coords(NVGFontSystem* sys, int font_id, float* coords, int num_coords) {
	if (!sys || !coords || font_id < 0 || font_id >= sys->font_count) return -1;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return -1;

	FT_Fixed* ft_coords = (FT_Fixed*)malloc(num_coords * sizeof(FT_Fixed));
	if (!ft_coords) return -1;

	if (FT_Get_Var_Design_Coordinates(face, num_coords, ft_coords) != 0) {
		free(ft_coords);
		return -1;
	}

	// Convert FT_Fixed (16.16) to float
	for (int i = 0; i < num_coords; i++) {
		coords[i] = (float)ft_coords[i] / 65536.0f;
	}

	free(ft_coords);
	return 0;
}

int nvgft_get_named_instance_count(NVGFontSystem* sys, int font_id) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return 0;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return 0;

	FT_MM_Var* mmvar = NULL;
	if (FT_Get_MM_Var(face, &mmvar) != 0) return 0;
	int count = (int)mmvar->num_namedstyles;
	FT_Done_MM_Var(sys->library, mmvar);
	return count;
}

int nvgft_set_named_instance(NVGFontSystem* sys, int font_id, int instance_index) {
	if (!sys || font_id < 0 || font_id >= sys->font_count) return -1;
	NVGFTFont* font = &sys->fonts[font_id];
	FT_Face face;
	if (FTC_Manager_LookupFace(sys->cache_manager, font->face_id, &face) != 0) return -1;

	if (FT_Set_Named_Instance(face, instance_index) != 0) return -1;
	return 0;
}

void nvgft_set_feature(NVGFontSystem* sys, unsigned int tag, int enabled) {
	if (!sys) return;

	// Search for existing feature
	for (int i = 0; i < sys->feature_count; i++) {
		if (sys->features[i].tag == tag) {
			sys->features[i].value = enabled ? 1 : 0;
			return;
		}
	}

	// Add new feature
	if (sys->feature_count >= sys->feature_capacity) {
		int new_capacity = sys->feature_capacity == 0 ? 8 : sys->feature_capacity * 2;
		hb_feature_t* new_features = (hb_feature_t*)realloc(sys->features, new_capacity * sizeof(hb_feature_t));
		if (!new_features) return;
		sys->features = new_features;
		sys->feature_capacity = new_capacity;
	}

	sys->features[sys->feature_count].tag = tag;
	sys->features[sys->feature_count].value = enabled ? 1 : 0;
	sys->features[sys->feature_count].start = HB_FEATURE_GLOBAL_START;
	sys->features[sys->feature_count].end = HB_FEATURE_GLOBAL_END;
	sys->feature_count++;
}

void nvgft_reset_features(NVGFontSystem* sys) {
	if (!sys) return;
	sys->feature_count = 0;
}
