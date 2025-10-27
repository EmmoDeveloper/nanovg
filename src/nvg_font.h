#ifndef NVG_FONT_H
#define NVG_FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdint.h>

// Simple font rendering system - direct FreeType to Vulkan
// Replaces fontstash with a clean, debuggable implementation

typedef struct NVGFont NVGFont;
typedef struct NVGFontAtlas NVGFontAtlas;
typedef struct NVGGlyph NVGGlyph;

// Glyph cache entry
struct NVGGlyph {
	uint32_t codepoint;
	
	// Atlas position (pixels)
	uint16_t x, y;
	uint16_t width, height;
	
	// Glyph metrics (pixels)
	int16_t bearingX, bearingY;
	uint16_t advance;
	
	// Texture coordinates (normalized 0-1)
	float u0, v0, u1, v1;
};

// Font atlas - single 512x512 ALPHA texture
struct NVGFontAtlas {
	unsigned char* data;     // 512x512 grayscale bitmap
	int width, height;
	int nextX, nextY;        // Next free position
	int rowHeight;           // Current row height
};

// Font instance
struct NVGFont {
	FT_Face face;
	NVGFontAtlas* atlas;
	
	// Glyph cache (simple linear array)
	NVGGlyph* glyphs;
	int glyphCount;
	int glyphCapacity;
	
	// Current size
	float size;
};

// Initialize FreeType library (call once)
int nvgfont_init(void);
void nvgfont_shutdown(void);

// Create/destroy font
NVGFont* nvgfont_create(const char* path);
void nvgfont_destroy(NVGFont* font);

// Set font size (in pixels)
void nvgfont_set_size(NVGFont* font, float size);

// Get or rasterize glyph (returns NULL if error)
NVGGlyph* nvgfont_get_glyph(NVGFont* font, uint32_t codepoint);

// Get atlas data for texture upload
const unsigned char* nvgfont_get_atlas_data(NVGFont* font, int* width, int* height);

#endif // NVG_FONT_H
