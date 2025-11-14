#ifndef NVG_FONT_INTERNAL_H
#define NVG_FONT_INTERNAL_H

#include "nvg_font_types.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_MULTIPLE_MASTERS_H
#include FT_COLOR_H
#include <hb.h>
#include <hb-ft.h>
#include <fribidi.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <vulkan/vulkan.h>
#include "nvg_font_shape_cache.h"

// Internal helpers
int nvg__findFontForCodepoint(NVGFontSystem* fs, int baseFontId, unsigned int codepoint);

// Internal font structure
struct NVGFont {
	char name[64];
	FT_Face face;
	hb_font_t* hb_font;
	unsigned char* data;
	int dataSize;
	int freeData;
	int fallbacks[NVG_FONT_MAX_FALLBACKS];
	int nfallbacks;
	int msdfMode;
	unsigned int varStateId;  // Increments when variation coordinates change
	float varCoords[NVG_FONT_MAX_VAR_AXES];  // Current variation coordinates
	unsigned int varCoordsCount;  // Number of active variation coordinates
	int hasCOLR;  // 1 if font has COLR color tables, 0 otherwise
};

// Glyph cache entry
typedef struct {
	unsigned int glyphIndex;  // FreeType glyph index (from HarfBuzz), NOT codepoint
	int fontId;
	float size;
	int hinting;              // Hinting mode
	int subpixelMode;         // Subpixel rendering mode (critical for cache correctness)
	unsigned int varStateId;  // Variation state ID (for variable fonts)
	VkColorSpaceKHR srcColorSpace;  // Source color space
	VkColorSpaceKHR dstColorSpace;  // Destination color space
	VkFormat format;          // Vulkan format this glyph is stored in
	float x, y, w, h;
	float s0, t0, s1, t1;
	float advanceX;
	float bearingX;
	float bearingY;
	unsigned int generation;
	int valid;
} NVGGlyphCacheEntry;

// Glyph cache
struct NVGGlyphCache {
	NVGGlyphCacheEntry entries[NVG_FONT_GLYPH_CACHE_SIZE];
	int count;
	unsigned int generation;
};

// Atlas node for packing
typedef struct NVGAtlasNode {
	short x, y, width;
} NVGAtlasNode;

// Maximum number of atlas formats supported
#define NVG_MAX_ATLAS_FORMATS 16

// Individual atlas storage
typedef struct NVGAtlas {
	VkColorSpaceKHR srcColorSpace;
	VkColorSpaceKHR dstColorSpace;
	VkFormat format;
	int width;
	int height;
	NVGAtlasNode* nodes;
	int nnodes;
	int cnodes;
	int active;
} NVGAtlas;

// Atlas manager - service desk for multiple atlases indexed by color space
struct NVGAtlasManager {
	NVGAtlas atlases[NVG_MAX_ATLAS_FORMATS];
	int atlasCount;
	void (*textureCallback)(void* uptr, int x, int y, int w, int h, const unsigned char* data, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format);
	void* textureUserdata;
	int (*growCallback)(void* uptr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int* newWidth, int* newHeight);
	void* growUserdata;
};

// HarfBuzz/FriBidi state
typedef struct {
	hb_buffer_t* hb_buffer;
	FriBidiCharType base_dir;
	int bidi_enabled;
} NVGTextShapingState;

// OpenType feature
typedef struct {
	char tag[5];
	int enabled;
} NVGOTFeature;

// Cairo state for COLR emoji rendering
typedef struct {
	cairo_surface_t* surface;
	cairo_t* cr;
	int surfaceWidth;
	int surfaceHeight;
} NVGCairoState;

// Main font system
struct NVGFontSystem {
	FT_Library ftLibrary;
	NVGFont fonts[NVG_FONT_MAX_FONTS];
	int nfonts;
	NVGGlyphCache* glyphCache;
	NVGAtlasManager* atlasManager;  // Single atlas manager servicing all atlas types
	NVGFontState state;
	NVGTextShapingState shapingState;
	NVGOTFeature features[32];
	int nfeatures;
	NVGCairoState cairoState;  // For COLR emoji rendering
	NVGShapedTextCache* shapedTextCache;  // Shaped text cache (Phase 14.2)
	VkColorSpaceKHR targetColorSpace;  // Target swapchain color space for rendering
};

#endif // NVG_FONT_INTERNAL_H
