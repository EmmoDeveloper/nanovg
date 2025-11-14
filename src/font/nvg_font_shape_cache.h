#ifndef NVG_FONT_SHAPE_CACHE_H
#define NVG_FONT_SHAPE_CACHE_H

#include <hb.h>
#include <fribidi.h>

// Cache key for shaped text lookup
typedef struct {
	char* text;                    // UTF-8 string (heap allocated)
	int textLen;                   // Length in bytes
	int fontId;                    // Font ID
	float size;                    // Font size
	int hinting;                   // Hinting mode
	int subpixelMode;              // Subpixel rendering mode
	unsigned int varStateId;       // Variable font state ID

	// OpenType features (sorted by tag for consistent hashing)
	char featureTags[32][5];       // Up to 32 features (tag + null terminator)
	int featureValues[32];         // 0 or 1
	int nfeatures;                 // Number of active features
	int kerningEnabled;            // Separate kerning flag

	// BiDi settings
	int bidiEnabled;               // FriBidi enabled
	int baseDir;                   // FRIBIDI_TYPE_LTR, RTL, or ON

	uint32_t hash;                 // Pre-computed hash for fast lookup
} NVGShapeKey;

// Cache value containing shaped text result
typedef struct {
	// Shaped glyph data (copied from HarfBuzz output)
	hb_glyph_info_t* glyphInfo;    // Array of glyph info
	hb_glyph_position_t* glyphPos; // Array of glyph positions
	unsigned int glyphCount;       // Number of glyphs
	hb_direction_t direction;      // Text direction

	// LRU tracking
	unsigned int lastUsed;         // Frame counter for LRU

	// Memory management
	int valid;                     // Entry is valid
} NVGShapedTextEntry;

// Shaped text cache
#define NVG_SHAPED_TEXT_CACHE_SIZE 256

typedef struct {
	NVGShapeKey keys[NVG_SHAPED_TEXT_CACHE_SIZE];
	NVGShapedTextEntry values[NVG_SHAPED_TEXT_CACHE_SIZE];
	int count;                     // Number of valid entries
	unsigned int frameCounter;     // Increments each lookup for LRU
} NVGShapedTextCache;

// Cache lifecycle
NVGShapedTextCache* nvgShapeCache_create(void);
void nvgShapeCache_destroy(NVGShapedTextCache* cache);

// Cache operations
NVGShapedTextEntry* nvgShapeCache_lookup(NVGShapedTextCache* cache, const NVGShapeKey* key);
void nvgShapeCache_insert(NVGShapedTextCache* cache, NVGShapeKey* key, hb_buffer_t* hb_buffer);

// Cache invalidation
void nvgShapeCache_clear(NVGShapedTextCache* cache);
void nvgShapeCache_invalidateFont(NVGShapedTextCache* cache, int fontId);

// Helper functions
uint32_t nvgShapeCache_hash(const NVGShapeKey* key);
int nvgShapeCache_compareKeys(const NVGShapeKey* a, const NVGShapeKey* b);
void nvgShapeCache_sortFeatures(NVGShapeKey* key);

#endif // NVG_FONT_SHAPE_CACHE_H
