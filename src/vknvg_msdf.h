#ifndef VKNVG_MSDF_H
#define VKNVG_MSDF_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

// MSDF generation parameters
typedef struct VKNVGmsdfParams {
	int width;           // Output bitmap width
	int height;          // Output bitmap height
	int stride;          // Output bitmap stride (bytes per row) - use width*4 if rendering to separate buffer
	float range;         // Distance field range in pixels
	float scale;         // Scale factor
	int offsetX;         // X offset (padding)
	int offsetY;         // Y offset (padding)
} VKNVGmsdfParams;

// Generate single-channel signed distance field (SDF) from FreeType glyph outline
// Output format: 1 byte per pixel (grayscale)
void vknvg__generateSDF(FT_GlyphSlot glyph, unsigned char* output, const VKNVGmsdfParams* params);

// Generate multi-channel signed distance field (MSDF) from FreeType glyph outline
// Output format: 3 bytes per pixel (RGB)
void vknvg__generateMSDF(FT_GlyphSlot glyph, unsigned char* output, const VKNVGmsdfParams* params);

#endif // VKNVG_MSDF_H
