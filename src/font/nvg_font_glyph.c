#include "nvg_font.h"
#include "nvg_font_internal.h"
#include "nvg_font_colr.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <freetype/ftoutln.h>

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

static NVGGlyphCacheEntry* nvg__findGlyph(NVGGlyphCache* cache, unsigned int glyphIndex, int fontId, float size, unsigned int varStateId) {
	for (int i = 0; i < cache->count; i++) {
		NVGGlyphCacheEntry* entry = &cache->entries[i];
		if (entry->valid && entry->glyphIndex == glyphIndex &&
			entry->fontId == fontId && fabsf(entry->size - size) < 0.01f &&
			entry->varStateId == varStateId) {
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

// Check if glyph has COLR data
// NOTE: This function receives the FONT (not just face) so it can check the hasCOLR flag
// The hasCOLR flag is set at font load time, before any size is set on the face
static int nvg__hasColorLayers(NVGFontSystem* fs, int fontId, FT_Face face, unsigned int glyph_index) {
	(void)face;  // Unused
	(void)glyph_index;  // Unused

	// First check if font has COLR capability at all (checked at load time)
	if (!fs->fonts[fontId].hasCOLR) return 0;

	// Font has COLR capability. Since FT_Get_Color_Glyph_Paint doesn't work reliably
	// in FreeType 2.14.1 (always returns 0), we rely on the hasCOLR flag set at load time.
	// Cairo will handle the actual COLR rendering automatically via cairo_show_glyphs().
	// If a glyph doesn't have COLR data, Cairo will render it as grayscale, which we can
	// detect and handle appropriately.
	return 1;
}

int nvgFontRenderGlyph(NVGFontSystem* fs, int fontId, unsigned int glyph_index, unsigned int codepoint,
                       float x, float y, NVGCachedGlyph* quad) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !quad) return 0;

	// Check cache first (use glyph_index as the key)
	unsigned int varStateId = fs->fonts[fontId].varStateId;
	NVGGlyphCacheEntry* entry = nvg__findGlyph(fs->glyphCache, glyph_index, fontId, fs->state.size, varStateId);
	if (entry) {
		quad->codepoint = codepoint;
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

	// If glyph_index is 0 (.notdef), try fallback fonts
	if (glyph_index == 0 && codepoint != 0 && fs->fonts[fontId].nfallbacks > 0) {
		// Find which fallback font has this codepoint
		int fallbackFontId = nvg__findFontForCodepoint(fs, fontId, codepoint);
		if (fallbackFontId != fontId) {
			// Found a fallback font with this glyph
			FT_Face fallback_face = fs->fonts[fallbackFontId].face;
			unsigned int fallback_glyph_index = FT_Get_Char_Index(fallback_face, codepoint);
			if (fallback_glyph_index != 0) {
				// Recursively render with fallback font
				return nvgFontRenderGlyph(fs, fallbackFontId, fallback_glyph_index,
				                          codepoint, x, y, quad);
			}
		}
	}

	if (glyph_index == 0) return 0;

	// IMPORTANT: Check for COLR data BEFORE setting size!
	// FT_Set_Char_Size and FT_Set_Pixel_Sizes both break FT_Get_Color_Glyph_Paint
	int isColor = nvg__hasColorLayers(fs, fontId, face, glyph_index);

	// Now set size for rendering
	FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

	// Re-apply variation coordinates after setting size (which may reset them)
	if (fs->fonts[fontId].varCoordsCount > 0) {
		FT_Fixed* ft_coords = (FT_Fixed*)malloc(sizeof(FT_Fixed) * fs->fonts[fontId].varCoordsCount);
		if (ft_coords) {
			for (unsigned int i = 0; i < fs->fonts[fontId].varCoordsCount; i++) {
				ft_coords[i] = (FT_Fixed)(fs->fonts[fontId].varCoords[i] * 65536.0f);
			}
			FT_Set_Var_Design_Coordinates(face, fs->fonts[fontId].varCoordsCount, ft_coords);
			free(ft_coords);
		}
	}

	static int color_check_count = 0;
	if (color_check_count++ < 10 || glyph_index >= 2340) {
		printf("[nvgFontRenderGlyph] glyph %u: FT_HAS_COLOR(face)=%d, nvg__hasColorLayers returned %d\n",
		       glyph_index, FT_HAS_COLOR(face) ? 1 : 0, isColor);
	}

	int gw, gh;
	unsigned char* rgba_data = NULL;

	// Store bearing info for COLR glyphs
	float colr_bearingX = 0.0f;
	float colr_bearingY = 0.0f;

	if (isColor) {
		// Render COLR glyph using Cairo
		// Load glyph to get advance
		if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_SVG)) {
			return 0;
		}

		FT_GlyphSlot slot = face->glyph;
		int advance = (int)(slot->metrics.horiAdvance / 64);

		// Compute the actual bounding box using FreeType's clip box API
		FT_ClipBox clip_box;
		FT_Bool has_clip_box = FT_Get_Color_Glyph_ClipBox(face, glyph_index, &clip_box);

		if (has_clip_box) {
			// Successfully got the clip box - this gives us the actual bounding box
			// Convert from 26.6 fixed point to pixels and add padding
			// Use floor for min and ceil for max to ensure we don't clip any pixels
			int xMin = (int)floor(clip_box.bottom_left.x / 64.0);
			int yMin = (int)floor(clip_box.bottom_left.y / 64.0);
			int xMax = (int)ceil(clip_box.top_right.x / 64.0);
			int yMax = (int)ceil(clip_box.top_right.y / 64.0);

			// Add 2 pixels padding on all sides to ensure no clipping
			xMin -= 2;
			yMin -= 2;
			xMax += 2;
			yMax += 2;

			gw = xMax - xMin;
			gh = yMax - yMin;
			colr_bearingX = (float)xMin;
			colr_bearingY = (float)yMax;  // Distance from baseline to top
		} else {
			// Clip box not available, use font metrics with extra padding
			// Add 10% padding to ensure we capture the full glyph
			int ascent = (int)(face->size->metrics.ascender / 64);
			int descent = (int)(-face->size->metrics.descender / 64);
			gw = (int)(advance * 1.1f);
			gh = (int)((ascent + descent) * 1.1f);
			colr_bearingX = (float)(gw - advance) / 2.0f;  // Center horizontally
			colr_bearingY = (float)ascent + (float)(gh - (ascent + descent)) / 2.0f;  // Center vertically
		}

		if (!nvg__renderCOLRGlyph(fs, face, glyph_index, gw, gh, &rgba_data, colr_bearingX, colr_bearingY)) {
			// Fallback to regular rendering if COLR fails
			isColor = 0;
			free(rgba_data);
			rgba_data = NULL;
		}
	}

	if (!isColor) {
		// Regular grayscale rendering
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
		gw = (int)slot->bitmap.width;
		gh = (int)slot->bitmap.rows;
	} else {
		// For COLR, we already have gw/gh from metrics
	}

	FT_GlyphSlot slot = face->glyph;

	// Select atlas manager based on glyph type
	NVGAtlasManager* atlas = isColor ? fs->atlasManagerRGBA : fs->atlasManagerALPHA;

	int ax, ay;
	if (!nvg__allocAtlasNode(atlas, gw + 2, gh + 2, &ax, &ay)) {
		// Atlas full - could trigger resize or defrag here
		return 0;
	}

	// Create cache entry
	entry = nvg__allocGlyph(fs->glyphCache);
	entry->glyphIndex = glyph_index;
	entry->fontId = fontId;
	entry->size = fs->state.size;
	entry->hinting = fs->state.hinting;
	entry->varStateId = varStateId;
	entry->isColor = isColor;
	entry->atlasIndex = isColor ? 1 : 0;  // RGBA atlas for color, ALPHA atlas for grayscale

	entry->x = (float)(ax + 1);
	entry->y = (float)(ay + 1);
	entry->w = (float)gw;
	entry->h = (float)gh;
	entry->s0 = entry->x / (float)atlas->width;
	entry->t0 = entry->y / (float)atlas->height;
	entry->s1 = (entry->x + entry->w) / (float)atlas->width;
	entry->t1 = (entry->y + entry->h) / (float)atlas->height;

	static int coord_debug = 0;
	if (coord_debug++ < 35) {
		printf("[Tex coord calc #%d] glyph %u, entry (%.1f,%.1f) size (%.1f,%.1f), atlas dim (%d,%d) @ %p -> coords (%.4f,%.4f)-(%.4f,%.4f)\n",
			coord_debug, glyph_index,
			entry->x, entry->y, entry->w, entry->h,
			atlas->width, atlas->height, (void*)atlas,
			entry->s0, entry->t0, entry->s1, entry->t1);
	}
	entry->advanceX = (float)slot->advance.x / 64.0f;
	// For COLR glyphs, use metrics-based bearings; for bitmap glyphs, use bitmap_left/top
	if (isColor) {
		entry->bearingX = colr_bearingX;
		entry->bearingY = colr_bearingY;
	} else {
		entry->bearingX = (float)slot->bitmap_left;
		entry->bearingY = (float)slot->bitmap_top;
	}

	// Apply UV inset to prevent linear filtering from sampling adjacent glyphs
	// With linear filtering, sampling at edges blends with neighboring texels
	// Inset by 0.5 pixels to stay within glyph bounds
	float uvInsetX = 0.5f / (float)atlas->width;
	float uvInsetY = 0.5f / (float)atlas->height;
	entry->s0 += uvInsetX;
	entry->t0 += uvInsetY;
	entry->s1 -= uvInsetX;
	entry->t1 -= uvInsetY;

	// Fill quad
	quad->codepoint = codepoint;
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

	// Upload bitmap data to atlas
	if (atlas->textureCallback && gw > 0 && gh > 0) {
		int padded_w = gw + 2;
		int padded_h = gh + 2;

		if (isColor && rgba_data) {
			// Upload RGBA color emoji
			int bytes_per_pixel = 4;
			unsigned char* data = (unsigned char*)calloc(padded_w * padded_h * bytes_per_pixel, 1);
			if (data) {
				// Copy RGBA glyph data to center of padded buffer
				for (int y = 0; y < gh; y++) {
					memcpy(data + ((y + 1) * padded_w + 1) * bytes_per_pixel,
					       rgba_data + y * gw * bytes_per_pixel,
					       gw * bytes_per_pixel);
				}

				static int upload_debug_color = 0;
				if (upload_debug_color++ < 10) {
					printf("[Atlas RGBA upload #%d] glyph %u to (%d,%d) size %dx%d (glyph %dx%d + 2px padding)\n",
						upload_debug_color, glyph_index, ax, ay, padded_w, padded_h, gw, gh);
				}
				atlas->textureCallback(
					atlas->textureUserdata,
					(int)ax, (int)ay,
					padded_w, padded_h,
					data,
					1  // atlasIndex = 1 for RGBA
				);
				free(data);
			}
			free(rgba_data);
		} else {
			// Upload grayscale bitmap
			int pitch = abs(slot->bitmap.pitch);
			unsigned char* data = (unsigned char*)calloc(padded_w * padded_h, 1);
			if (data) {
				// Copy glyph data to center of padded buffer (leaving 1px border of zeros)
				for (int y = 0; y < gh; y++) {
					memcpy(data + (y + 1) * padded_w + 1,
					       slot->bitmap.buffer + y * pitch,
					       gw);
				}

				static int upload_debug = 0;
				if (upload_debug++ < 30) {
					printf("[Atlas ALPHA upload #%d] glyph %u to (%d,%d) size %dx%d (glyph %dx%d + 2px padding)\n",
						upload_debug, glyph_index, ax, ay, padded_w, padded_h, gw, gh);
				}
				atlas->textureCallback(
					atlas->textureUserdata,
					(int)ax, (int)ay,
					padded_w, padded_h,
					data,
					0  // atlasIndex = 0 for ALPHA
				);
				free(data);
			}
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
