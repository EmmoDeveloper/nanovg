#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
	fs->atlasManager->width = atlasWidth;
	fs->atlasManager->height = atlasHeight;
	fs->atlasManager->cnodes = 256;
	fs->atlasManager->nodes = (NVGAtlasNode*)calloc(fs->atlasManager->cnodes, sizeof(NVGAtlasNode));
	if (!fs->atlasManager->nodes) {
		free(fs->atlasManager);
		free(fs->glyphCache);
		FT_Done_FreeType(fs->ftLibrary);
		free(fs);
		return NULL;
	}
	fs->atlasManager->nnodes = 1;
	fs->atlasManager->nodes[0].x = 0;
	fs->atlasManager->nodes[0].y = 0;
	fs->atlasManager->nodes[0].width = (short)atlasWidth;

	// Initialize HarfBuzz buffer
	fs->shapingState.hb_buffer = hb_buffer_create();
	if (!fs->shapingState.hb_buffer) {
		free(fs->atlasManager->nodes);
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
	fs->shapingState.bidi_enabled = 1;
	fs->shapingState.base_dir = FRIBIDI_TYPE_ON;

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

	// Free atlas manager
	if (fs->atlasManager) {
		if (fs->atlasManager->nodes) {
			free(fs->atlasManager->nodes);
		}
		free(fs->atlasManager);
	}

	// Free glyph cache
	if (fs->glyphCache) {
		free(fs->glyphCache);
	}

	// Shutdown FreeType
	FT_Done_FreeType(fs->ftLibrary);

	free(fs);
}

void nvgFontSetTextureCallback(NVGFontSystem* fs, void (*callback)(void* uptr, int x, int y, int w, int h, const unsigned char* data), void* userdata) {
	if (!fs || !fs->atlasManager) return;
	fs->atlasManager->textureCallback = callback;
	fs->atlasManager->textureUserdata = userdata;
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

// Font state

void nvgFontSetFont(NVGFontSystem* fs, int fontId) {
	if (!fs) return;
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
	if (!fs || !fs->atlasManager || !fs->glyphCache) return;

	// Clear glyph cache
	memset(fs->glyphCache->entries, 0, sizeof(fs->glyphCache->entries));
	fs->glyphCache->count = 0;
	fs->glyphCache->generation++;

	// Reset atlas
	fs->atlasManager->width = width;
	fs->atlasManager->height = height;
	fs->atlasManager->nnodes = 1;
	fs->atlasManager->nodes[0].x = 0;
	fs->atlasManager->nodes[0].y = 0;
	fs->atlasManager->nodes[0].width = (short)width;
	fs->atlasManager->atlasCount = 0;
}
