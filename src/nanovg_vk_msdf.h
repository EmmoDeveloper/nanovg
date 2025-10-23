//
// Copyright (c) 2025 NanoVG MSDF Generator
//
// Multi-channel Signed Distance Field generation for high-quality text rendering
//

#ifndef NANOVG_VK_MSDF_H
#define NANOVG_VK_MSDF_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

// MSDF generation parameters
typedef struct VKNVGmsdfParams {
	int width;           // Output texture width
	int height;          // Output texture height
	float range;         // Distance field range in pixels (typically 4.0)
	float scale;         // Scale factor for glyph
	int offsetX;         // X offset in output
	int offsetY;         // Y offset in output
} VKNVGmsdfParams;

// Generate MSDF from FreeType glyph outline
// Returns 1 on success, 0 on failure
// output: RGB buffer (width * height * 3 bytes)
int vknvg__generateMSDF(FT_GlyphSlot glyph, unsigned char* output, VKNVGmsdfParams* params);

// Generate SDF from FreeType glyph outline (single channel)
// output: Grayscale buffer (width * height bytes)
int vknvg__generateSDF(FT_GlyphSlot glyph, unsigned char* output, VKNVGmsdfParams* params);

#endif // NANOVG_VK_MSDF_H
