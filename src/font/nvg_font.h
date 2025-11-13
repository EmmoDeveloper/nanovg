#ifndef NVG_FONT_H
#define NVG_FONT_H

#include "nvg_font_types.h"
#include <vulkan/vulkan.h>

// Font system lifecycle
NVGFontSystem* nvgFontCreate(int atlasWidth, int atlasHeight);
void nvgFontDestroy(NVGFontSystem* fs);
void nvgFontSetTextureCallback(NVGFontSystem* fs, void (*callback)(void* uptr, int x, int y, int w, int h, const unsigned char* data, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format), void* userdata);
void nvgFontSetAtlasGrowCallback(NVGFontSystem* fs, int (*callback)(void* uptr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int* newWidth, int* newHeight), void* userdata);

// Color space configuration
void nvgFontSetColorSpace(NVGFontSystem* fs, VkColorSpaceKHR colorSpace);

// Font loading
int nvgFontAddFont(NVGFontSystem* fs, const char* name, const char* path);
int nvgFontAddFontMem(NVGFontSystem* fs, const char* name, unsigned char* data, int ndata, int freeData);
int nvgFontFindFont(NVGFontSystem* fs, const char* name);
void nvgFontAddFallback(NVGFontSystem* fs, int baseFont, int fallbackFont);
void nvgFontResetFallback(NVGFontSystem* fs, int baseFont);

// Font state
void nvgFontSetFont(NVGFontSystem* fs, int fontId);
void nvgFontSetSize(NVGFontSystem* fs, float size);
void nvgFontSetSpacing(NVGFontSystem* fs, float spacing);
void nvgFontSetBlur(NVGFontSystem* fs, float blur);
void nvgFontSetAlign(NVGFontSystem* fs, int align);
void nvgFontSetFontMSDF(NVGFontSystem* fs, int font, int msdfMode);
void nvgFontResetAtlas(NVGFontSystem* fs, int width, int height);

// Text layout and iteration
void nvgFontShapedTextIterInit(NVGFontSystem* fs, NVGTextIter* iter, float x, float y,
                                const char* string, const char* end, int bidi, void* state);
int nvgFontShapedTextIterNext(NVGFontSystem* fs, NVGTextIter* iter, NVGCachedGlyph* quad);
void nvgFontTextIterFree(NVGTextIter* iter);
int nvgFontTextIterNext(NVGFontSystem* fs, NVGTextIter* iter, NVGCachedGlyph* quad);

// Text measurement
float nvgFontTextBounds(NVGFontSystem* fs, float x, float y, const char* string, const char* end, float* bounds);
float nvgFontTextBoundsShaped(NVGFontSystem* fs, float x, float y, const char* string, const char* end,
                               float* bounds, int bidi, void* state);
void nvgFontVertMetrics(NVGFontSystem* fs, float* ascender, float* descender, float* lineh);
float nvgFontLineBounds(NVGFontSystem* fs, float y, float* miny, float* maxy);

// Variable fonts
int nvgFont__SetVarDesignCoords(NVGFontSystem* fs, int fontId, const float* coords, unsigned int num_coords);
int nvgFont__GetVarDesignCoords(NVGFontSystem* fs, int fontId, float* coords, unsigned int num_coords);
int nvgFont__GetNamedInstanceCount(NVGFontSystem* fs, int fontId);
int nvgFont__SetNamedInstance(NVGFontSystem* fs, int fontId, unsigned int instance_index);

// OpenType features
void nvgFontSetFeature(NVGFontSystem* fs, const char* tag, int enabled);
void nvgFontResetFeatures(NVGFontSystem* fs);

// Font configuration
void nvgFontSetHinting(NVGFontSystem* fs, int hinting);
void nvgFontSetKerning(NVGFontSystem* fs, int enabled);
void nvgFontSetTextDirection(NVGFontSystem* fs, int direction);

// Font information
const char* nvgFont__GetFamilyName(NVGFontSystem* fs, int fontId);
const char* nvgFont__GetStyleName(NVGFontSystem* fs, int fontId);
int nvgFont__IsVariable(NVGFontSystem* fs, int fontId);
int nvgFont__IsScalable(NVGFontSystem* fs, int fontId);
int nvgFont__IsFixedWidth(NVGFontSystem* fs, int fontId);
int nvgFont__GetVarAxisCount(NVGFontSystem* fs, int fontId);
int nvgFont__GetVarAxis(NVGFontSystem* fs, int fontId, unsigned int axis_index, NVGVarAxis* axis);

// Glyph-level API
int nvgFontGetGlyphCount(NVGFontSystem* fs, int fontId);
int nvgFontGetGlyphMetrics(NVGFontSystem* fs, int fontId, unsigned int codepoint, NVGGlyphMetrics* metrics);
float nvgFontGetKerning(NVGFontSystem* fs, int fontId, unsigned int left_glyph, unsigned int right_glyph);
int nvgFontRenderGlyph(NVGFontSystem* fs, int fontId, unsigned int glyph_index, unsigned int codepoint,
                       float x, float y, NVGCachedGlyph* quad);

#endif // NVG_FONT_H
