#ifndef NVG_FONT_TYPES_H
#define NVG_FONT_TYPES_H

#include <stdint.h>
#include <vulkan/vulkan.h>

// Font system configuration
#define NVG_FONT_MAX_FONTS 32
#define NVG_FONT_MAX_FALLBACKS 8
#define NVG_FONT_MAX_VAR_AXES 32
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
	VkColorSpaceKHR srcColorSpace;  // Source color space (identifies atlas)
	VkColorSpaceKHR dstColorSpace;  // Destination color space (identifies atlas)
	VkFormat format;                 // Texture format (identifies atlas)
	int subpixelMode;                // Subpixel mode (identifies atlas)
	unsigned int generation;         // For cache invalidation
} NVGCachedGlyph;

// Text iterator for shaped text
typedef struct {
	float x, y;           // Current position
	float x0, y0, x1, y1; // Glyph bounds
	float s0, t0, s1, t1; // Texture coords
	unsigned int codepoint;
	const char* str;      // Current position in string
	const char* next;     // Next position
	unsigned int glyphIndex; // Current HarfBuzz glyph index for iteration
	void* cachedShaping;  // Opaque pointer to NVGShapedTextEntry (NULL if using shared buffer)
} NVGTextIter;

// Subpixel rendering modes
typedef enum {
	NVG_SUBPIXEL_NONE = 0,      // No subpixel rendering (grayscale)
	NVG_SUBPIXEL_RGB = 1,       // LCD RGB horizontal
	NVG_SUBPIXEL_BGR = 2,       // LCD BGR horizontal
	NVG_SUBPIXEL_VRGB = 3,      // LCD RGB vertical
	NVG_SUBPIXEL_VBGR = 4       // LCD BGR vertical
} NVGSubpixelMode;

// Font rendering state
typedef struct {
	int fontId;
	float size;
	float spacing;
	float blur;
	int align;
	int hinting;
	int kerningEnabled;
	NVGSubpixelMode subpixelMode;
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
