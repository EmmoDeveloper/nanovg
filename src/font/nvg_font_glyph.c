#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal helpers

static int nvg__allocAtlasNode(NVGAtlasManager* atlas, int w, int h, int* x, int* y) {
	int i, bestIdx = -1, bestY = atlas->height, bestX = 0;

	// Find best node
	for (i = 0; i < atlas->nnodes; i++) {
		int node_y = atlas->nodes[i].y;
		if (atlas->nodes[i].width >= w) {
			if (node_y < bestY) {
				bestIdx = i;
				bestY = node_y;
				bestX = atlas->nodes[i].x;
			}
		}
	}

	if (bestIdx == -1) return 0;

	// Check if we fit vertically
	if (bestY + h > atlas->height) return 0;

	*x = bestX;
	*y = bestY;

	// Insert new node
	if (atlas->nnodes + 1 >= atlas->cnodes) {
		atlas->cnodes = atlas->cnodes == 0 ? 8 : atlas->cnodes * 2;
		atlas->nodes = (NVGAtlasNode*)realloc(atlas->nodes, sizeof(NVGAtlasNode) * atlas->cnodes);
	}

	for (i = atlas->nnodes; i > bestIdx; i--) {
		atlas->nodes[i] = atlas->nodes[i-1];
	}
	atlas->nodes[bestIdx].x = (short)(*x + w);
	atlas->nodes[bestIdx].y = (short)*y;
	atlas->nodes[bestIdx].width = (short)(atlas->width - (*x + w));
	atlas->nnodes++;

	// Update existing node
	if (bestIdx < atlas->nnodes - 1) {
		atlas->nodes[bestIdx+1].x = (short)*x;
		atlas->nodes[bestIdx+1].y = (short)(*y + h);
		atlas->nodes[bestIdx+1].width = (short)w;
	}

	return 1;
}

static NVGGlyphCacheEntry* nvg__findGlyph(NVGGlyphCache* cache, unsigned int codepoint, int fontId, float size) {
	for (int i = 0; i < cache->count; i++) {
		NVGGlyphCacheEntry* entry = &cache->entries[i];
		if (entry->valid && entry->codepoint == codepoint &&
			entry->fontId == fontId && fabsf(entry->size - size) < 0.01f) {
			return entry;
		}
	}
	return NULL;
}

static NVGGlyphCacheEntry* nvg__allocGlyph(NVGGlyphCache* cache) {
	if (cache->count >= NVG_FONT_GLYPH_CACHE_SIZE) {
		// Evict all entries and increment generation
		for (int i = 0; i < cache->count; i++) {
			cache->entries[i].valid = 0;
		}
		cache->count = 0;
		cache->generation++;
	}
	NVGGlyphCacheEntry* entry = &cache->entries[cache->count++];
	memset(entry, 0, sizeof(NVGGlyphCacheEntry));
	entry->valid = 1;
	entry->generation = cache->generation;
	return entry;
}

// Glyph-level API

int nvgFontGetGlyphCount(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;
	return (int)fs->fonts[fontId].face->num_glyphs;
}

int nvgFontGetGlyphMetrics(NVGFontSystem* fs, int fontId, unsigned int codepoint, NVGGlyphMetrics* metrics) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !metrics) return 0;

	FT_Face face = fs->fonts[fontId].face;
	FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
	if (glyph_index == 0) return 0;

	// Set size
	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

	// Load glyph to get metrics
	FT_Int32 load_flags = FT_LOAD_DEFAULT;
	if (!fs->state.hinting) {
		load_flags |= FT_LOAD_NO_HINTING;
	}
	if (FT_Load_Glyph(face, glyph_index, load_flags)) {
		return 0;
	}

	metrics->glyphIndex = glyph_index;
	metrics->bearingX = (float)face->glyph->metrics.horiBearingX / 64.0f;
	metrics->bearingY = (float)face->glyph->metrics.horiBearingY / 64.0f;
	metrics->advanceX = (float)face->glyph->metrics.horiAdvance / 64.0f;
	metrics->advanceY = (float)face->glyph->metrics.vertAdvance / 64.0f;
	metrics->width = (float)face->glyph->metrics.width / 64.0f;
	metrics->height = (float)face->glyph->metrics.height / 64.0f;

	return 1;
}

float nvgFontGetKerning(NVGFontSystem* fs, int fontId, unsigned int left_glyph, unsigned int right_glyph) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0.0f;
	if (!fs->state.kerningEnabled) return 0.0f;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_KERNING(face)) return 0.0f;

	FT_Vector kerning;
	FT_Get_Kerning(face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
	return (float)kerning.x / 64.0f;
}

int nvgFontRenderGlyph(NVGFontSystem* fs, int fontId, unsigned int glyph_index,
                       float x, float y, NVGCachedGlyph* quad) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !quad) return 0;

	// Check cache first (use glyph_index as the key)
	NVGGlyphCacheEntry* entry = nvg__findGlyph(fs->glyphCache, glyph_index, fontId, fs->state.size);
	if (entry) {
		static int cache_hit_count = 0;
		if (cache_hit_count++ < 5) {
			printf("[nvgFontRenderGlyph] Cache HIT for glyph %u (fontId=%d, size=%.1f)\n",
				glyph_index, fontId, fs->state.size);
		}
		quad->codepoint = glyph_index;
		quad->x0 = x + entry->bearingX;
		quad->y0 = y - entry->bearingY;
		quad->x1 = quad->x0 + entry->w;
		quad->y1 = quad->y0 + entry->h;
		quad->s0 = entry->s0;
		quad->t0 = entry->t0;
		quad->s1 = entry->s1;
		quad->t1 = entry->t1;
		quad->advanceX = entry->advanceX;
		quad->bearingX = entry->bearingX;
		quad->bearingY = entry->bearingY;
		quad->atlasIndex = entry->atlasIndex;
		quad->generation = entry->generation;
		return 1;
	}

	// Render glyph (glyph_index is already a FreeType glyph index from HarfBuzz)
	FT_Face face = fs->fonts[fontId].face;
	if (glyph_index == 0) return 0;

	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

	FT_Int32 load_flags = FT_LOAD_RENDER;
	if (!fs->state.hinting) {
		load_flags |= FT_LOAD_NO_HINTING;
	}
	if (FT_Load_Glyph(face, glyph_index, load_flags)) {
		return 0;
	}

	FT_GlyphSlot slot = face->glyph;
	if (slot->format != FT_GLYPH_FORMAT_BITMAP) {
		return 0;
	}

	// Allocate space in atlas
	int gw = (int)slot->bitmap.width;
	int gh = (int)slot->bitmap.rows;

	// Debug: check first few bytes
	static int first_glyph = 1;
	if (first_glyph && gw > 0 && gh > 0) {
		printf("[nvgFontRenderGlyph] First glyph: width=%d height=%d, first 10 bytes: ", gw, gh);
		for (int i = 0; i < 10 && i < gw * gh; i++) {
			printf("%d ", slot->bitmap.buffer[i]);
		}
		printf("\n");
		first_glyph = 0;
	}
	int ax, ay;
	if (!nvg__allocAtlasNode(fs->atlasManager, gw + 2, gh + 2, &ax, &ay)) {
		// Atlas full - could trigger resize or defrag here
		return 0;
	}

	// Create cache entry
	entry = nvg__allocGlyph(fs->glyphCache);
	entry->codepoint = glyph_index;
	entry->fontId = fontId;
	entry->size = fs->state.size;

	static int cache_miss_count = 0;
	if (cache_miss_count++ < 20) {
		printf("[nvgFontRenderGlyph] Cache MISS for glyph %u (fontId=%d, size=%.1f), cache entry #%d\n",
			glyph_index, fontId, fs->state.size, fs->glyphCache->count - 1);
	}
	entry->atlasIndex = 0;
	entry->x = (float)(ax + 1);
	entry->y = (float)(ay + 1);
	entry->w = (float)gw;
	entry->h = (float)gh;
	entry->s0 = entry->x / (float)fs->atlasManager->width;
	entry->t0 = entry->y / (float)fs->atlasManager->height;
	entry->s1 = (entry->x + entry->w) / (float)fs->atlasManager->width;
	entry->t1 = (entry->y + entry->h) / (float)fs->atlasManager->height;
	entry->advanceX = (float)slot->advance.x / 64.0f;
	entry->bearingX = (float)slot->bitmap_left;
	entry->bearingY = (float)slot->bitmap_top;

	// Fill quad
	quad->codepoint = glyph_index;
	quad->x0 = x + entry->bearingX;
	quad->y0 = y - entry->bearingY;
	quad->x1 = quad->x0 + entry->w;
	quad->y1 = quad->y0 + entry->h;
	quad->s0 = entry->s0;
	quad->t0 = entry->t0;
	quad->s1 = entry->s1;
	quad->t1 = entry->t1;
	quad->advanceX = entry->advanceX;
	quad->bearingX = entry->bearingX;
	quad->bearingY = entry->bearingY;
	quad->atlasIndex = entry->atlasIndex;
	quad->generation = entry->generation;

	// Upload bitmap data to atlas (grayscale format - FreeType outputs 1 byte per pixel)
	// Use bitmap.pitch instead of width as stride may include padding
	if (fs->atlasManager->textureCallback && gw > 0 && gh > 0) {
		int pitch = abs(slot->bitmap.pitch);  // pitch can be negative for top-down bitmaps
		// Copy with proper pitch handling (no Y-flip needed with vertex shader Y-down)
		unsigned char* data = (unsigned char*)malloc(gw * gh);
		if (data) {
			for (int y = 0; y < gh; y++) {
				memcpy(data + y * gw, slot->bitmap.buffer + y * pitch, gw);
			}
			fs->atlasManager->textureCallback(
				fs->atlasManager->textureUserdata,
				(int)entry->x,
				(int)entry->y,
				gw,
				gh,
				data
			);
			free(data);
		}
	}

	return 1;
}

// Font measurement

void nvgFontVertMetrics(NVGFontSystem* fs, float* ascender, float* descender, float* lineh) {
	if (!fs || fs->state.fontId < 0 || fs->state.fontId >= fs->nfonts) {
		if (ascender) *ascender = 0.0f;
		if (descender) *descender = 0.0f;
		if (lineh) *lineh = 0.0f;
		return;
	}

	FT_Face face = fs->fonts[fs->state.fontId].face;
	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

	float scale = fs->state.size / (float)face->units_per_EM;
	if (ascender) *ascender = (float)face->ascender * scale;
	if (descender) *descender = (float)face->descender * scale;
	if (lineh) *lineh = (float)face->height * scale;
}

float nvgFontLineBounds(NVGFontSystem* fs, float y, float* miny, float* maxy) {
	if (!fs || fs->state.fontId < 0 || fs->state.fontId >= fs->nfonts) {
		if (miny) *miny = 0.0f;
		if (maxy) *maxy = 0.0f;
		return 0.0f;
	}

	FT_Face face = fs->fonts[fs->state.fontId].face;
	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

	float scale = fs->state.size / (float)face->units_per_EM;
	float ascender = (float)face->ascender * scale;
	float descender = (float)face->descender * scale;

	if (miny) *miny = y + descender;
	if (maxy) *maxy = y + ascender;

	return ascender - descender;
}
