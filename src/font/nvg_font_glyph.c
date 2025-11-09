#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal helpers
// Skyline-based atlas packing (adapted from fontstash.h)

static int nvg__atlasInsertNode(NVGAtlasManager* atlas, int idx, int x, int y, int w)
{
	int i;
	if (atlas->nnodes + 1 > atlas->cnodes) {
		atlas->cnodes = atlas->cnodes == 0 ? 8 : atlas->cnodes * 2;
		atlas->nodes = (NVGAtlasNode*)realloc(atlas->nodes, sizeof(NVGAtlasNode) * atlas->cnodes);
		if (atlas->nodes == NULL)
			return 0;
	}
	for (i = atlas->nnodes; i > idx; i--)
		atlas->nodes[i] = atlas->nodes[i-1];
	atlas->nodes[idx].x = (short)x;
	atlas->nodes[idx].y = (short)y;
	atlas->nodes[idx].width = (short)w;
	atlas->nnodes++;
	return 1;
}

static void nvg__atlasRemoveNode(NVGAtlasManager* atlas, int idx)
{
	int i;
	if (atlas->nnodes == 0) return;
	for (i = idx; i < atlas->nnodes - 1; i++)
		atlas->nodes[i] = atlas->nodes[i+1];
	atlas->nnodes--;
}

static int nvg__atlasAddSkylineLevel(NVGAtlasManager* atlas, int idx, int x, int y, int w, int h)
{
	int i;

	// Insert new node
	if (nvg__atlasInsertNode(atlas, idx, x, y + h, w) == 0)
		return 0;

	// Delete skyline segments that fall under the shadow of the new segment
	for (i = idx + 1; i < atlas->nnodes; i++) {
		if (atlas->nodes[i].x < atlas->nodes[i-1].x + atlas->nodes[i-1].width) {
			int shrink = atlas->nodes[i-1].x + atlas->nodes[i-1].width - atlas->nodes[i].x;
			atlas->nodes[i].x += (short)shrink;
			atlas->nodes[i].width -= (short)shrink;
			if (atlas->nodes[i].width <= 0) {
				nvg__atlasRemoveNode(atlas, i);
				i--;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	// Merge same height skyline segments that are next to each other
	for (i = 0; i < atlas->nnodes - 1; i++) {
		if (atlas->nodes[i].y == atlas->nodes[i+1].y) {
			atlas->nodes[i].width += atlas->nodes[i+1].width;
			nvg__atlasRemoveNode(atlas, i + 1);
			i--;
		}
	}

	return 1;
}

static int nvg__atlasRectFits(NVGAtlasManager* atlas, int i, int w, int h)
{
	// Check if there is enough space at the location of skyline span 'i'
	int x = atlas->nodes[i].x;
	int y = atlas->nodes[i].y;
	int spaceLeft;
	if (x + w > atlas->width)
		return -1;
	spaceLeft = w;
	while (spaceLeft > 0) {
		if (i == atlas->nnodes) return -1;
		if (atlas->nodes[i].y > y)
			y = atlas->nodes[i].y;
		if (y + h > atlas->height) return -1;
		spaceLeft -= atlas->nodes[i].width;
		++i;
	}
	return y;
}

static int nvg__allocAtlasNode(NVGAtlasManager* atlas, int w, int h, int* x, int* y)
{
	int besth = atlas->height, bestw = atlas->width, besti = -1;
	int bestx = -1, besty = -1, i;

	// Bottom left fit heuristic
	for (i = 0; i < atlas->nnodes; i++) {
		int y_fit = nvg__atlasRectFits(atlas, i, w, h);
		if (y_fit != -1) {
			if (y_fit + h < besth || (y_fit + h == besth && atlas->nodes[i].width < bestw)) {
				besti = i;
				bestw = atlas->nodes[i].width;
				besth = y_fit + h;
				bestx = atlas->nodes[i].x;
				besty = y_fit;
			}
		}
	}

	if (besti == -1)
		return 0;

	// Perform the actual packing
	if (nvg__atlasAddSkylineLevel(atlas, besti, bestx, besty, w, h) == 0)
		return 0;

	*x = bestx;
	*y = besty;

	return 1;
}

static NVGGlyphCacheEntry* nvg__findGlyph(NVGGlyphCache* cache, unsigned int glyphIndex, int fontId, float size) {
	for (int i = 0; i < cache->count; i++) {
		NVGGlyphCacheEntry* entry = &cache->entries[i];
		if (entry->valid && entry->glyphIndex == glyphIndex &&
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
		if (cache_hit_count++ < 50 || glyph_index == 36) {  // 36 = 'A' glyph
			printf("[nvgFontRenderGlyph] Cache HIT for glyph %u (fontId=%d, size=%.1f), entry size=%.1f, atlas pos (%.1f,%.1f), tex coords (%.4f,%.4f)-(%.4f,%.4f)\n",
				glyph_index, fontId, fs->state.size, entry->size,
				entry->x, entry->y,
				entry->s0, entry->t0, entry->s1, entry->t1);
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
	entry->glyphIndex = glyph_index;
	entry->fontId = fontId;
	entry->size = fs->state.size;
	entry->hinting = fs->state.hinting;

	static int cache_miss_count = 0;
	if (cache_miss_count++ < 50 || glyph_index == 36) {  // 36 = 'A' glyph
		printf("[nvgFontRenderGlyph] Cache MISS for glyph %u (fontId=%d, size=%.1f), atlas region (%d,%d) size (%d,%d) [COORDS NOT YET CALCULATED]\n",
			glyph_index, fontId, fs->state.size, ax, ay, gw+2, gh+2);
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

	static int coord_debug = 0;
	if (coord_debug++ < 35) {
		printf("[Tex coord calc #%d] glyph %u, entry (%.1f,%.1f) size (%.1f,%.1f), atlas dim (%d,%d) @ %p -> coords (%.4f,%.4f)-(%.4f,%.4f)\n",
			coord_debug, glyph_index,
			entry->x, entry->y, entry->w, entry->h,
			fs->atlasManager->width, fs->atlasManager->height, (void*)fs->atlasManager,
			entry->s0, entry->t0, entry->s1, entry->t1);
	}
	entry->advanceX = (float)slot->advance.x / 64.0f;
	entry->bearingX = (float)slot->bitmap_left;
	entry->bearingY = (float)slot->bitmap_top;

	// Apply UV inset to prevent linear filtering from sampling adjacent glyphs
	// With linear filtering, sampling at edges blends with neighboring texels
	// Inset by 0.5 pixels to stay within glyph bounds
	float uvInsetX = 0.5f / (float)fs->atlasManager->width;
	float uvInsetY = 0.5f / (float)fs->atlasManager->height;
	entry->s0 += uvInsetX;
	entry->t0 += uvInsetY;
	entry->s1 -= uvInsetX;
	entry->t1 -= uvInsetY;

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
	// Atlas allocated (gw+2) × (gh+2) for 1-pixel padding, so upload with padding cleared
	if (fs->atlasManager->textureCallback && gw > 0 && gh > 0) {
		int pitch = abs(slot->bitmap.pitch);
		int padded_w = gw + 2;
		int padded_h = gh + 2;

		// Allocate padded buffer and clear it (zero padding prevents texture bleeding)
		unsigned char* data = (unsigned char*)calloc(padded_w * padded_h, 1);
		if (data) {
			// Copy glyph data to center of padded buffer (leaving 1px border of zeros)
			for (int y = 0; y < gh; y++) {
				memcpy(data + (y + 1) * padded_w + 1,
				       slot->bitmap.buffer + y * pitch,
				       gw);
			}

			// Upload entire padded region starting at allocated atlas position
			static int upload_debug = 0;
			if (upload_debug++ < 30) {
				printf("[Atlas upload #%d] glyph %u to (%d,%d) size %dx%d (glyph %dx%d + 2px padding)\n",
					upload_debug, glyph_index, ax, ay, padded_w, padded_h, gw, gh);
			}
			fs->atlasManager->textureCallback(
				fs->atlasManager->textureUserdata,
				(int)ax,  // Upload to allocated position (not entry->x which has +1 offset)
				(int)ay,
				padded_w,
				padded_h,
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
