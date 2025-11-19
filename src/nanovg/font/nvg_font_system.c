#include "nvg_font.h"
#include "nvg_font_internal.h"
#include "nvg_font_colr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Atlas manager internal helpers

static NVGAtlas* nvg__getOrCreateAtlas(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int width, int height) {
	if (!mgr) return NULL;

	// Match by color spaces, format AND subpixelMode (to allow multiple atlases with different subpixel modes)
	for (int i = 0; i < mgr->atlasCount; i++) {
		if (mgr->atlases[i].active &&
		    mgr->atlases[i].srcColorSpace == srcColorSpace &&
		    mgr->atlases[i].dstColorSpace == dstColorSpace &&
		    mgr->atlases[i].format == format &&
		    mgr->atlases[i].subpixelMode == subpixelMode) {
			return &mgr->atlases[i];
		}
	}

	if (mgr->atlasCount >= NVG_MAX_ATLAS_FORMATS) {
		return NULL;
	}

	NVGAtlas* atlas = &mgr->atlases[mgr->atlasCount++];
	atlas->srcColorSpace = srcColorSpace;
	atlas->dstColorSpace = dstColorSpace;
	atlas->format = format;
	atlas->subpixelMode = subpixelMode;
	atlas->width = width;
	atlas->height = height;
	atlas->textureId = 0;  // Will be set when texture is created
	atlas->cnodes = 256;
	atlas->nodes = (NVGAtlasNode*)calloc(atlas->cnodes, sizeof(NVGAtlasNode));
	if (!atlas->nodes) {
		mgr->atlasCount--;
		return NULL;
	}
	atlas->nnodes = 1;
	atlas->nodes[0].x = 0;
	atlas->nodes[0].y = 0;
	atlas->nodes[0].width = (short)width;
	atlas->active = 1;

	return atlas;
}

// Get atlas by format + color spaces + subpixelMode (composite key)
// Format and subpixelMode are ALWAYS part of the key to distinguish different atlas types
NVGAtlas* nvg__getAtlas(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode) {
	if (!mgr) return NULL;

	for (int i = 0; i < mgr->atlasCount; i++) {
		if (mgr->atlases[i].active &&
		    mgr->atlases[i].srcColorSpace == srcColorSpace &&
		    mgr->atlases[i].dstColorSpace == dstColorSpace &&
		    mgr->atlases[i].format == format &&
		    mgr->atlases[i].subpixelMode == subpixelMode) {
			return &mgr->atlases[i];
		}
	}

	return NULL;
}

// Atlas manager API - forward declarations from nvg_font_glyph.c
int nvg__allocAtlasNode(NVGAtlas* atlas, int w, int h, int* x, int* y);
void nvg__expandAtlasNodes(NVGAtlas* atlas, int n);

int nvgAtlasAlloc(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int w, int h, int* x, int* y) {
	NVGAtlas* atlas = nvg__getAtlas(mgr, srcColorSpace, dstColorSpace, format, subpixelMode);
	if (!atlas) {
		// Atlas doesn't exist - create it on-demand
		atlas = nvg__getOrCreateAtlas(mgr, srcColorSpace, dstColorSpace, format, subpixelMode, mgr->defaultAtlasWidth, mgr->defaultAtlasHeight);
		if (!atlas) return 0;
	}
	return nvg__allocAtlasNode(atlas, w, h, x, y);
}

void nvgAtlasUpdate(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int x, int y, int w, int h, const unsigned char* data) {
	if (!mgr || !mgr->textureCallback) {
		printf("[nvgAtlasUpdate] ERROR: mgr=%p, callback=%p\n", (void*)mgr, (void*)(mgr ? mgr->textureCallback : NULL));
		return;
	}
	printf("[nvgAtlasUpdate] Calling texture callback for (%d,%d) size %dx%d, srcCS=%u, dstCS=%u, format=%u, subpixel=%d\n",
		x, y, w, h, srcColorSpace, dstColorSpace, format, subpixelMode);
	mgr->textureCallback(mgr->textureUserdata, x, y, w, h, data, srcColorSpace, dstColorSpace, format, subpixelMode);
}

int nvgAtlasGrow(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int* newWidth, int* newHeight) {
	if (!mgr || !mgr->growCallback) return 0;
	return mgr->growCallback(mgr->growUserdata, srcColorSpace, dstColorSpace, format, subpixelMode, newWidth, newHeight);
}

void nvgAtlasGetSize(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int* w, int* h) {
	NVGAtlas* atlas = nvg__getAtlas(mgr, srcColorSpace, dstColorSpace, format, subpixelMode);
	if (atlas && w && h) {
		*w = atlas->width;
		*h = atlas->height;
	}
}

void nvgAtlasReset(NVGAtlasManager* mgr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int width, int height) {
	NVGAtlas* atlas = nvg__getAtlas(mgr, srcColorSpace, dstColorSpace, format, subpixelMode);
	if (atlas) {
		atlas->width = width;
		atlas->height = height;
		atlas->nnodes = 1;
		atlas->nodes[0].x = 0;
		atlas->nodes[0].y = 0;
		atlas->nodes[0].width = (short)width;
	}
}

// Font system lifecycle

NVGFontSystem* nvgFontCreate(int atlasWidth, int atlasHeight) {
	NVGFontSystem* fs = (NVGFontSystem*)calloc(1, sizeof(NVGFontSystem));
	if (!fs) return NULL;

	// Initialize FreeType
	if (FT_Init_FreeType(&fs->ftLibrary)) {
		free(fs);
		return NULL;
	}

	// Initialize glyph cache
	fs->glyphCache = (NVGGlyphCache*)calloc(1, sizeof(NVGGlyphCache));
	if (!fs->glyphCache) {
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}

	// Initialize atlas manager
	fs->atlasManager = (NVGAtlasManager*)calloc(1, sizeof(NVGAtlasManager));
	if (!fs->atlasManager) {
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}
	fs->atlasManager->defaultAtlasWidth = atlasWidth;
	fs->atlasManager->defaultAtlasHeight = atlasHeight;

	// Create default atlases for sRGB color space (most common case)
	// Grayscale atlas: sRGB -> sRGB, no subpixel
	if (!nvg__getOrCreateAtlas(fs->atlasManager,
	                            NVG_COLOR_SPACE_SRGB_NONLINEAR,
	                            NVG_COLOR_SPACE_SRGB_NONLINEAR,
	                            NVG_TEXTURE_FORMAT_R8_UNORM,
	                            0,  // NVG_SUBPIXEL_NONE
	                            atlasWidth, atlasHeight)) {
		free(fs->atlasManager);
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}

	// RGBA atlas: sRGB -> sRGB, no subpixel
	if (!nvg__getOrCreateAtlas(fs->atlasManager,
	                            NVG_COLOR_SPACE_SRGB_NONLINEAR,
	                            NVG_COLOR_SPACE_SRGB_NONLINEAR,
	                            NVG_TEXTURE_FORMAT_R8G8B8A8_UNORM,
	                            0,  // NVG_SUBPIXEL_NONE
	                            atlasWidth, atlasHeight)) {
		free(fs->atlasManager->atlases[0].nodes);
		free(fs->atlasManager);
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}

	// Initialize HarfBuzz buffer
	fs->shapingState.hb_buffer = hb_buffer_create();
	if (!fs->shapingState.hb_buffer) {
		for (int i = 0; i < fs->atlasManager->atlasCount; i++) {
			if (fs->atlasManager->atlases[i].nodes) {
				free(fs->atlasManager->atlases[i].nodes);
			}
		}
		free(fs->atlasManager);
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}

	// Default state
	fs->state.fontId = -1;
	fs->state.size = 12.0f;
	fs->state.spacing = 0.0f;
	fs->state.blur = 0.0f;
	fs->state.align = 0;
	fs->state.hinting = 1;
	fs->state.kerningEnabled = 1;
	fs->state.subpixelMode = NVG_SUBPIXEL_NONE;
	fs->shapingState.bidi_enabled = 1;
	fs->shapingState.base_dir = FRIBIDI_TYPE_ON;
	fs->shapingState.runs = NULL;
	fs->shapingState.runCount = 0;
	fs->shapingState.runCapacity = 0;

	// Initialize Cairo for COLR emoji rendering
	nvg__initCairoState(fs);

	// Initialize shaped text cache (Phase 14.2)
	fs->shapedTextCache = nvgShapeCache_create();
	if (!fs->shapedTextCache) {
		nvg__destroyCairoState(fs);
		for (int i = 0; i < fs->atlasManager->atlasCount; i++) {
			if (fs->atlasManager->atlases[i].nodes) {
				free(fs->atlasManager->atlases[i].nodes);
			}
		}
		free(fs->atlasManager);
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}

	// Initialize target color space to sRGB (default)
	fs->targetColorSpace = NVG_COLOR_SPACE_SRGB_NONLINEAR;

	return fs;
}

void nvgFontDestroy(NVGFontSystem* fs) {
	if (!fs) return;

	// Free all loaded fonts
	for (int i = 0; i < fs->nfonts; i++) {
		if (fs->fonts[i].hb_font) {
			hb_font_destroy(fs->fonts[i].hb_font);
		}
		if (fs->fonts[i].face) {
			FT_Done_Face(fs->fonts[i].face);
		}
		if (fs->fonts[i].freeData && fs->fonts[i].data) {
			free(fs->fonts[i].data);
		}
	}

	// Free HarfBuzz buffer
	if (fs->shapingState.hb_buffer) {
		hb_buffer_destroy(fs->shapingState.hb_buffer);
	}

	// Free font runs
	if (fs->shapingState.runs) {
		for (int i = 0; i < fs->shapingState.runCount; i++) {
			if (fs->shapingState.runs[i].glyphs) {
				free(fs->shapingState.runs[i].glyphs);
			}
			if (fs->shapingState.runs[i].positions) {
				free(fs->shapingState.runs[i].positions);
			}
		}
		free(fs->shapingState.runs);
	}

	// Destroy Cairo state
	nvg__destroyCairoState(fs);

	// Free atlas manager
	if (fs->atlasManager) {
		for (int i = 0; i < fs->atlasManager->atlasCount; i++) {
			if (fs->atlasManager->atlases[i].nodes) {
				free(fs->atlasManager->atlases[i].nodes);
			}
		}
		free(fs->atlasManager);
	}

	// Free glyph cache
	if (fs->glyphCache) {
		free(fs->glyphCache);
	}

	// Free shaped text cache (Phase 14.2)
	if (fs->shapedTextCache) {
		nvgShapeCache_destroy(fs->shapedTextCache);
	}

	// Shutdown FreeType
	FT_Done_FreeType(fs->ftLibrary);

	free(fs);
}

void nvgFontSetTextureCallback(NVGFontSystem* fs, void (*callback)(void* uptr, int x, int y, int w, int h, const unsigned char* data, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode), void* userdata) {
	if (!fs || !fs->atlasManager) return;
	fs->atlasManager->textureCallback = callback;
	fs->atlasManager->textureUserdata = userdata;
}

void nvgFontSetAtlasGrowCallback(NVGFontSystem* fs, int (*callback)(void* uptr, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int* newWidth, int* newHeight), void* userdata) {
	if (!fs || !fs->atlasManager) return;
	fs->atlasManager->growCallback = callback;
	fs->atlasManager->growUserdata = userdata;
}

void nvgFontSetColorSpace(NVGFontSystem* fs, NVGcolorSpace colorSpace) {
	if (!fs) return;
	fs->targetColorSpace = colorSpace;
}

// Font loading

int nvgFontAddFont(NVGFontSystem* fs, const char* name, const char* path) {
	if (!fs || !name || !path) return -1;
	if (fs->nfonts >= NVG_FONT_MAX_FONTS) return -1;

	// Check if font already exists
	for (int i = 0; i < fs->nfonts; i++) {
		if (strcmp(fs->fonts[i].name, name) == 0) {
			return i;
		}
	}

	// Load font face
	FT_Face face;
	if (FT_New_Face(fs->ftLibrary, path, 0, &face)) {
		return -1;
	}

	// CRITICAL: Detect if font has COLR data NOW, before any size is set
	// Once FT_Set_Pixel_Sizes is called, FT_Get_Color_Glyph_Paint stops working
	int hasCOLR = FT_HAS_COLOR(face) ? 1 : 0;

	// Create HarfBuzz font
	hb_font_t* hb_font = hb_ft_font_create(face, NULL);
	if (!hb_font) {
		FT_Done_Face(face);
		return -1;
	}

	// Add to font array
	int idx = fs->nfonts;
	strncpy(fs->fonts[idx].name, name, sizeof(fs->fonts[idx].name) - 1);
	fs->fonts[idx].name[sizeof(fs->fonts[idx].name) - 1] = '\0';
	fs->fonts[idx].face = face;
	fs->fonts[idx].hb_font = hb_font;
	fs->fonts[idx].data = NULL;
	fs->fonts[idx].dataSize = 0;
	fs->fonts[idx].freeData = 0;
	fs->fonts[idx].nfallbacks = 0;
	fs->fonts[idx].msdfMode = 0;
	fs->fonts[idx].varStateId = 0;  // Initial variation state
	fs->fonts[idx].varCoordsCount = 0;  // No variation coordinates yet
	fs->fonts[idx].hasCOLR = hasCOLR;  // Store COLR capability
	fs->nfonts++;

	return idx;
}

int nvgFontAddFontMem(NVGFontSystem* fs, const char* name, unsigned char* data, int ndata, int freeData) {
	if (!fs || !name || !data) return -1;
	if (fs->nfonts >= NVG_FONT_MAX_FONTS) return -1;

	// Check if font already exists
	for (int i = 0; i < fs->nfonts; i++) {
		if (strcmp(fs->fonts[i].name, name) == 0) {
			return i;
		}
	}

	// Load font face from memory
	FT_Face face;
	if (FT_New_Memory_Face(fs->ftLibrary, data, ndata, 0, &face)) {
		return -1;
	}

	// CRITICAL: Detect if font has COLR data NOW, before any size is set
	int hasCOLR = FT_HAS_COLOR(face) ? 1 : 0;

	// Create HarfBuzz font
	hb_font_t* hb_font = hb_ft_font_create(face, NULL);
	if (!hb_font) {
		FT_Done_Face(face);
		return -1;
	}

	// Add to font array
	int idx = fs->nfonts;
	strncpy(fs->fonts[idx].name, name, sizeof(fs->fonts[idx].name) - 1);
	fs->fonts[idx].name[sizeof(fs->fonts[idx].name) - 1] = '\0';
	fs->fonts[idx].face = face;
	fs->fonts[idx].hb_font = hb_font;
	fs->fonts[idx].data = data;
	fs->fonts[idx].dataSize = ndata;
	fs->fonts[idx].freeData = freeData;
	fs->fonts[idx].nfallbacks = 0;
	fs->fonts[idx].msdfMode = 0;
	fs->fonts[idx].varStateId = 0;  // Initial variation state
	fs->fonts[idx].varCoordsCount = 0;  // No variation coordinates yet
	fs->fonts[idx].hasCOLR = hasCOLR;  // Store COLR capability
	fs->nfonts++;

	return idx;
}

int nvgFontFindFont(NVGFontSystem* fs, const char* name) {
	if (!fs || !name) return -1;
	for (int i = 0; i < fs->nfonts; i++) {
		if (strcmp(fs->fonts[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

void nvgFontAddFallback(NVGFontSystem* fs, int baseFont, int fallbackFont) {
	if (!fs || baseFont < 0 || baseFont >= fs->nfonts) return;
	if (fallbackFont < 0 || fallbackFont >= fs->nfonts) return;
	NVGFont* font = &fs->fonts[baseFont];
	if (font->nfallbacks >= NVG_FONT_MAX_FALLBACKS) return;
	font->fallbacks[font->nfallbacks++] = fallbackFont;
}

void nvgFontResetFallback(NVGFontSystem* fs, int baseFont) {
	if (!fs || baseFont < 0 || baseFont >= fs->nfonts) return;
	fs->fonts[baseFont].nfallbacks = 0;
}

// Font fallback helper - finds which font (base or fallback) has a glyph for the codepoint
int nvg__findFontForCodepoint(NVGFontSystem* fs, int baseFontId, unsigned int codepoint) {
	if (!fs || baseFontId < 0 || baseFontId >= fs->nfonts) return -1;

	// Try base font first
	FT_Face face = fs->fonts[baseFontId].face;
	if (FT_Get_Char_Index(face, codepoint) != 0) {
		return baseFontId;
	}

	// Try fallback fonts
	for (int i = 0; i < fs->fonts[baseFontId].nfallbacks; i++) {
		int fallbackId = fs->fonts[baseFontId].fallbacks[i];
		if (fallbackId >= 0 && fallbackId < fs->nfonts) {
			face = fs->fonts[fallbackId].face;
			if (FT_Get_Char_Index(face, codepoint) != 0) {
				return fallbackId;
			}
		}
	}

	return baseFontId;  // Fallback to base font if nothing found
}

// Font state

void nvgFontSetFont(NVGFontSystem* fs, int fontId) {
	if (!fs) return;

	// Invalidate cache entries for old font when font changes
	if (fs->state.fontId != fontId && fs->shapedTextCache) {
		nvgShapeCache_invalidateFont(fs->shapedTextCache, fs->state.fontId);
	}

	fs->state.fontId = fontId;
}

void nvgFontSetSize(NVGFontSystem* fs, float size) {
	if (!fs) return;
	fs->state.size = size;
}

void nvgFontSetSpacing(NVGFontSystem* fs, float spacing) {
	if (!fs) return;
	fs->state.spacing = spacing;
}

void nvgFontSetBlur(NVGFontSystem* fs, float blur) {
	if (!fs) return;
	fs->state.blur = blur;
}

void nvgFontSetAlign(NVGFontSystem* fs, int align) {
	if (!fs) return;
	fs->state.align = align;
}

void nvgFontSetFontMSDF(NVGFontSystem* fs, int font, int msdfMode) {
	if (!fs || font < 0 || font >= fs->nfonts) return;
	fs->fonts[font].msdfMode = msdfMode;
}

void nvgFontResetAtlas(NVGFontSystem* fs, int width, int height) {
	if (!fs || !fs->glyphCache || !fs->atlasManager) return;

	// Clear glyph cache
	memset(fs->glyphCache->entries, 0, sizeof(fs->glyphCache->entries));
	fs->glyphCache->count = 0;
	fs->glyphCache->generation++;

	// Reset all atlases
	for (int i = 0; i < fs->atlasManager->atlasCount; i++) {
		NVGAtlas* atlas = &fs->atlasManager->atlases[i];
		if (atlas->active) {
			atlas->width = width;
			atlas->height = height;
			atlas->nnodes = 1;
			atlas->nodes[0].x = 0;
			atlas->nodes[0].y = 0;
			atlas->nodes[0].width = (short)width;
		}
	}
}

void nvgFontSetSubpixelMode(NVGFontSystem* fs, int mode) {
	if (!fs) return;
	printf("[nvgFontSetSubpixelMode] Setting mode to %d\n", mode);
	fs->state.subpixelMode = (NVGSubpixelMode)mode;
}

int nvgFontGetAtlasTexture(NVGFontSystem* fs, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode) {
	if (!fs || !fs->atlasManager) return 0;
	NVGAtlas* atlas = nvg__getAtlas(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode);
	return atlas ? atlas->textureId : 0;
}

void nvgFontSetAtlasTexture(NVGFontSystem* fs, NVGcolorSpace srcColorSpace, NVGcolorSpace dstColorSpace, NVGtextureFormat format, int subpixelMode, int textureId) {
	if (!fs || !fs->atlasManager) return;
	NVGAtlas* atlas = nvg__getAtlas(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode);
	if (atlas) {
		atlas->textureId = textureId;
	}
}
