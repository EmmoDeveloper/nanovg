#ifndef NVG_FONT_COLR_H
#define NVG_FONT_COLR_H

#include "nvg_font_internal.h"

// Initialize Cairo state for COLR emoji rendering
void nvg__initCairoState(NVGFontSystem* fs);

// Destroy Cairo state
void nvg__destroyCairoState(NVGFontSystem* fs);

// Render a COLR glyph to RGBA buffer
// Returns 1 on success, 0 if not a COLR glyph or error
// Allocates rgba_data (caller must free)
int nvg__renderCOLRGlyph(NVGFontSystem* fs, FT_Face face,
                         unsigned int glyph_index,
                         int width, int height,
                         unsigned char** rgba_data,
                         float bearingX, float bearingY);

#endif // NVG_FONT_COLR_H
