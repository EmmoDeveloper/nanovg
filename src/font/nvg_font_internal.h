#ifndef NVG_FONT_INTERNAL_H
#define NVG_FONT_INTERNAL_H

#include "nvg_font_types.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_MULTIPLE_MASTERS_H
#include <hb.h>
#include <hb-ft.h>
#include <fribidi.h>

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
};

// Glyph cache entry
typedef struct {
	unsigned int glyphIndex;  // FreeType glyph index (from HarfBuzz), NOT codepoint
	int fontId;
	float size;
	int hinting;              // Hinting mode
	unsigned int varStateId;  // Variation state ID (for variable fonts)
	int atlasIndex;
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

// Atlas manager
struct NVGAtlasManager {
	int width;
	int height;
	NVGAtlasNode* nodes;
	int nnodes;
	int cnodes;
	int atlasCount;
	void (*textureCallback)(void* uptr, int x, int y, int w, int h, const unsigned char* data);
	void* textureUserdata;
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

// Main font system
struct NVGFontSystem {
	FT_Library ftLibrary;
	NVGFont fonts[NVG_FONT_MAX_FONTS];
	int nfonts;
	NVGGlyphCache* glyphCache;
	NVGAtlasManager* atlasManager;
	NVGFontState state;
	NVGTextShapingState shapingState;
	NVGOTFeature features[32];
	int nfeatures;
};

#endif // NVG_FONT_INTERNAL_H
