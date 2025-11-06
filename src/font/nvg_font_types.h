#ifndef NVG_FONT_TYPES_H
#define NVG_FONT_TYPES_H

#include <stdint.h>

// Font system configuration
#define NVG_FONT_MAX_FONTS 32
#define NVG_FONT_MAX_FALLBACKS 8
#define NVG_FONT_ATLAS_INITIAL_SIZE 512
#define NVG_FONT_GLYPH_CACHE_SIZE 1024

// Forward declarations
typedef struct NVGFontSystem NVGFontSystem;
typedef struct NVGFont NVGFont;
typedef struct NVGGlyphCache NVGGlyphCache;
typedef struct NVGAtlasManager NVGAtlasManager;

// Glyph metrics
typedef struct {
	float bearingX;
	float bearingY;
	float advanceX;
	float advanceY;
	float width;
	float height;
	unsigned int glyphIndex;
} NVGGlyphMetrics;

// Cached glyph with atlas position
typedef struct {
	unsigned int codepoint;
	float x0, y0, x1, y1;  // Glyph bounds in pixels
	float s0, t0, s1, t1;  // Texture coordinates in atlas
	float advanceX;
	float bearingX;
	float bearingY;
	int atlasIndex;
	unsigned int generation;  // For cache invalidation
} NVGCachedGlyph;

// Text iterator for shaped text
typedef struct {
	float x, y;           // Current position
	float x0, y0, x1, y1; // Glyph bounds
	float s0, t0, s1, t1; // Texture coords
	unsigned int codepoint;
	const char* str;      // Current position in string
	const char* next;     // Next position
} NVGTextIter;

// Font rendering state
typedef struct {
	int fontId;
	float size;
	float spacing;
	float blur;
	int align;
	int hinting;
	int kerningEnabled;
} NVGFontState;

// Variable font axis
typedef struct {
	char tag[5];          // 4-character tag + null terminator
	float minValue;
	float defaultValue;
	float maxValue;
	char name[64];
} NVGVarAxis;

#endif // NVG_FONT_TYPES_H
