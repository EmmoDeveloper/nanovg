#include "nvg_font.h"
#include "nvg_font_internal.h"
#include "nvg_font_colr.h"
#include "../vknvg_msdf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <freetype/ftoutln.h>
#include <freetype/ftlcdfil.h>

// Forward declarations from nvg_font_system.c
int nvgAtlasAlloc(NVGAtlasManager* mgr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int subpixelMode, int w, int h, int* x, int* y);
void nvgAtlasUpdate(NVGAtlasManager* mgr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int subpixelMode, int x, int y, int w, int h, const unsigned char* data);
int nvgAtlasGrow(NVGAtlasManager* mgr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int subpixelMode, int* newWidth, int* newHeight);
NVGAtlas* nvg__getAtlas(NVGAtlasManager* mgr, VkColorSpaceKHR srcColorSpace, VkColorSpaceKHR dstColorSpace, VkFormat format, int subpixelMode);

// Internal helpers
// Skyline-based atlas packing (adapted from fontstash.h)

static int nvg__atlasInsertNode(NVGAtlas* atlas, int idx, int x, int y, int w)
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

static void nvg__atlasRemoveNode(NVGAtlas* atlas, int idx)
{
	int i;
	if (atlas->nnodes == 0) return;
	for (i = idx; i < atlas->nnodes - 1; i++)
		atlas->nodes[i] = atlas->nodes[i+1];
	atlas->nnodes--;
}

static int nvg__atlasAddSkylineLevel(NVGAtlas* atlas, int idx, int x, int y, int w, int h)
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

static int nvg__atlasRectFits(NVGAtlas* atlas, int i, int w, int h)
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

int nvg__allocAtlasNode(NVGAtlas* atlas, int w, int h, int* x, int* y)
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

static NVGGlyphCacheEntry* nvg__findGlyph(NVGGlyphCache* cache, unsigned int glyphIndex, int fontId, float size, unsigned int varStateId, int hinting, int subpixelMode) {
	for (int i = 0; i < cache->count; i++) {
		NVGGlyphCacheEntry* entry = &cache->entries[i];
		if (entry->valid && entry->glyphIndex == glyphIndex &&
			entry->fontId == fontId && fabsf(entry->size - size) < 0.01f &&
			entry->varStateId == varStateId && entry->hinting == hinting &&
			entry->subpixelMode == subpixelMode) {
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
	static int total_calls = 0;
	total_calls++;
	printf("[RENDER_CALL #%d] glyph %u, subpixel=%d\n", total_calls, glyph_index, fs ? fs->state.subpixelMode : -1);

	if (!fs || fontId < 0 || fontId >= fs->nfonts || !quad) return 0;

	// Check cache first (use glyph_index, hinting, and subpixel mode as the key)
	unsigned int varStateId = fs->fonts[fontId].varStateId;
	NVGGlyphCacheEntry* entry = nvg__findGlyph(fs->glyphCache, glyph_index, fontId, fs->state.size, varStateId, fs->state.hinting, fs->state.subpixelMode);
	if (entry) {
		static int cache_hit_debug = 0;
		if (cache_hit_debug++ < 20) {
			printf("[Cache HIT #%d] glyph %u: UVs (%.4f,%.4f)-(%.4f,%.4f)\n",
				cache_hit_debug, glyph_index, entry->s0, entry->t0, entry->s1, entry->t1);
		}
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
		quad->srcColorSpace = entry->srcColorSpace;
		quad->dstColorSpace = entry->dstColorSpace;
		quad->format = entry->format;
		quad->subpixelMode = entry->subpixelMode;
		quad->generation = entry->generation;
		return 1;
	} else {
		static int cache_miss_debug = 0;
		if (cache_miss_debug++ < 20) {
			printf("[Cache MISS #%d] glyph %u will be rendered\n", cache_miss_debug, glyph_index);
		}
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
	int isCOLREmoji = nvg__hasColorLayers(fs, fontId, face, glyph_index);

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
		       glyph_index, FT_HAS_COLOR(face) ? 1 : 0, isCOLREmoji);
	}

	int gw, gh;
	unsigned char* rgba_data = NULL;

	// Store bearing info for COLR glyphs
	float colr_bearingX = 0.0f;
	float colr_bearingY = 0.0f;

	if (isCOLREmoji) {
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
			isCOLREmoji = 0;
			free(rgba_data);
			rgba_data = NULL;
		}
	}

	// Check if MSDF mode is enabled for this font
	int useMSDF = (fs->fonts[fontId].msdfMode > 0);

	if (!isCOLREmoji) {
		// Check if we should generate MSDF/SDF
		if (useMSDF) {
			// Load glyph outline for distance field generation
			FT_Int32 load_flags = FT_LOAD_NO_BITMAP;
			if (!fs->state.hinting) {
				load_flags |= FT_LOAD_NO_HINTING;
			}
			if (FT_Load_Glyph(face, glyph_index, load_flags)) {
				return 0;
			}

			FT_GlyphSlot slot = face->glyph;
			if (slot->format != FT_GLYPH_FORMAT_OUTLINE) {
				return 0;
			}

			// Calculate MSDF dimensions (larger than glyph for distance field range)
			// Use 64x64 as base size with distance field range
			int msdfRange = 16;  // pixels
			gw = 64;
			gh = 64;
		} else {
			// Regular grayscale or LCD subpixel rendering
			FT_Int32 load_flags = FT_LOAD_RENDER;
			if (!fs->state.hinting) {
				load_flags |= FT_LOAD_NO_HINTING;
			}

			// Set LCD filter and target for subpixel rendering
			if (fs->state.subpixelMode != NVG_SUBPIXEL_NONE) {
				FT_Library_SetLcdFilter(fs->ftLibrary, FT_LCD_FILTER_DEFAULT);

				switch (fs->state.subpixelMode) {
					case NVG_SUBPIXEL_RGB:
					case NVG_SUBPIXEL_BGR:
						load_flags |= FT_LOAD_TARGET_LCD;
						break;
					case NVG_SUBPIXEL_VRGB:
					case NVG_SUBPIXEL_VBGR:
						load_flags |= FT_LOAD_TARGET_LCD_V;
						break;
					default:
						break;
				}
			}

			if (FT_Load_Glyph(face, glyph_index, load_flags)) {
				return 0;
			}

			FT_GlyphSlot slot = face->glyph;
			if (slot->format != FT_GLYPH_FORMAT_BITMAP) {
				return 0;
			}

			// Allocate space in atlas
			// For LCD rendering, bitmap width is 3x wider (RGB subpixels)
			gw = (int)slot->bitmap.width;
			gh = (int)slot->bitmap.rows;

			// For horizontal LCD, width is already 3x, divide by 3 for logical width
			// For vertical LCD, height is 3x
			if (fs->state.subpixelMode == NVG_SUBPIXEL_RGB || fs->state.subpixelMode == NVG_SUBPIXEL_BGR) {
				// Horizontal LCD - width is 3x
				gw = gw / 3;
			} else if (fs->state.subpixelMode == NVG_SUBPIXEL_VRGB || fs->state.subpixelMode == NVG_SUBPIXEL_VBGR) {
				// Vertical LCD - height is 3x
				gh = gh / 3;
			}
		}
	} else {
		// For COLR, we already have gw/gh from metrics
	}

	FT_GlyphSlot slot = face->glyph;

	// Determine color spaces and format
	// Color spaces distinguish different atlas types:
	// - COLR emoji: sRGB source (Cairo outputs sRGB)
	// - LCD subpixel: No color space (linear RGB subpixels, not sRGB)
	// - MSDF: No color space (distance field)
	// - Grayscale: No color space (single channel alpha)
	int useSubpixel = (fs->state.subpixelMode != NVG_SUBPIXEL_NONE && !isCOLREmoji && !useMSDF);

	VkColorSpaceKHR srcColorSpace;
	VkColorSpaceKHR dstColorSpace;
	if (isCOLREmoji) {
		// COLR emoji uses sRGB (Cairo outputs sRGB)
		srcColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		dstColorSpace = fs->targetColorSpace;
	} else {
		// LCD, MSDF, and grayscale: intensity/coverage values, no color space conversion
		// Use (VkColorSpaceKHR)-1 as sentinel to indicate "no color space"
		srcColorSpace = (VkColorSpaceKHR)-1;
		dstColorSpace = (VkColorSpaceKHR)-1;
	}

	// Determine format based on rendering mode:
	// - COLR emoji: RGBA (4 channels)
	// - MSDF: RGBA (4 channels for multi-channel distance field)
	// - LCD subpixel: RGBA (3 RGB channels + alpha, stored as RGBA)
	// - Grayscale: ALPHA (1 channel)
	VkFormat format = isCOLREmoji ? VK_FORMAT_R8G8B8A8_UNORM :
	                  (useMSDF ? VK_FORMAT_R8G8B8A8_UNORM :
	                  (useSubpixel ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8_UNORM));

	static int alloc_debug = 0;
	if (alloc_debug++ < 20) {
		printf("[Glyph %u] gw=%d, gh=%d, srcCS=%u, dstCS=%u, fmt=%u\n",
			glyph_index, gw, gh, srcColorSpace, dstColorSpace, format);
	}

	int ax, ay;
	int subpixelMode = fs->state.subpixelMode;
	if (!nvgAtlasAlloc(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, gw + 2, gh + 2, &ax, &ay)) {
		// Atlas full - try to grow atlas
		printf("[nvgFontRenderGlyph] Atlas alloc failed for glyph %u (%dx%d), srcCS=%u, dstCS=%u, fmt=%u, subpixel=%d - trying to grow\n",
			glyph_index, gw + 2, gh + 2, srcColorSpace, dstColorSpace, format, subpixelMode);
		int newWidth = 0, newHeight = 0;
		if (nvgAtlasGrow(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, &newWidth, &newHeight)) {
			// Atlas was grown, try allocation again
			if (!nvgAtlasAlloc(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, gw + 2, gh + 2, &ax, &ay)) {
				// Still failed after growth
				printf("[nvgFontRenderGlyph] ERROR: Atlas alloc still failed after growth!\n");
				return 0;
			}
		} else {
			// Growth failed
			printf("[nvgFontRenderGlyph] ERROR: Atlas grow failed!\n");
			return 0;
		}
	}

	// Get atlas for dimensions (use format-aware lookup to distinguish ALPHA vs RGBA with same color spaces)
	NVGAtlas* atlas = nvg__getAtlas(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode);
	if (!atlas) {
		printf("[nvgFontRenderGlyph] ERROR: nvg__getAtlas returned NULL after allocation! srcCS=%u, dstCS=%u, fmt=%u\n",
			srcColorSpace, dstColorSpace, format);
		return 0;
	}

	// Create cache entry
	entry = nvg__allocGlyph(fs->glyphCache);
	entry->glyphIndex = glyph_index;
	entry->fontId = fontId;
	entry->size = fs->state.size;
	entry->hinting = fs->state.hinting;
	entry->subpixelMode = fs->state.subpixelMode;
	entry->varStateId = varStateId;

	static int entry_debug = 0;
	if (entry_debug++ < 10) {
		printf("[Cache entry created #%d] glyph %u, subpixelMode=%d\n",
			entry_debug, glyph_index, entry->subpixelMode);
	}
	entry->srcColorSpace = srcColorSpace;
	entry->dstColorSpace = dstColorSpace;
	entry->format = format;
	entry->subpixelMode = subpixelMode;

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
	if (isCOLREmoji) {
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
	quad->srcColorSpace = srcColorSpace;
	quad->dstColorSpace = dstColorSpace;
	quad->format = format;
	quad->subpixelMode = subpixelMode;
	quad->generation = entry->generation;

	// Upload bitmap data to atlas
	if (gw > 0 && gh > 0) {
		int padded_w = gw + 2;
		int padded_h = gh + 2;

		if (isCOLREmoji && rgba_data) {
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
				nvgAtlasUpdate(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, (int)ax, (int)ay, padded_w, padded_h, data);
				free(data);
			}
			free(rgba_data);
		} else if (useMSDF) {
			// Generate and upload MSDF
			int bytes_per_pixel = (fs->fonts[fontId].msdfMode == 2) ? 4 : 1;  // MSDF=RGB(4), SDF=ALPHA(1)
			unsigned char* msdf_data = (unsigned char*)calloc(gw * gh * bytes_per_pixel, 1);

			if (msdf_data) {
				// Set up MSDF generation parameters
				VKNVGmsdfParams params;
				params.width = gw;
				params.height = gh;
				params.stride = gw * bytes_per_pixel;
				params.range = 16.0f;  // Distance field range in pixels
				params.scale = 1.0f;
				params.offsetX = 0;
				params.offsetY = 0;

				// Generate MSDF or SDF
				if (fs->fonts[fontId].msdfMode == 2) {
					vknvg__generateMSDF(slot, msdf_data, &params);
				} else {
					vknvg__generateSDF(slot, msdf_data, &params);
				}

				// Create padded buffer
				unsigned char* data = (unsigned char*)calloc(padded_w * padded_h * bytes_per_pixel, 1);
				if (data) {
					// Copy MSDF data to center of padded buffer
					for (int y = 0; y < gh; y++) {
						memcpy(data + ((y + 1) * padded_w + 1) * bytes_per_pixel,
						       msdf_data + y * gw * bytes_per_pixel,
						       gw * bytes_per_pixel);
					}

					nvgAtlasUpdate(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, (int)ax, (int)ay, padded_w, padded_h, data);
					free(data);
				}
				free(msdf_data);
			}
		} else if (fs->state.subpixelMode != NVG_SUBPIXEL_NONE) {
			// Upload LCD subpixel bitmap (3 bytes per pixel RGB -> 4 bytes RGBA)
			int pitch = abs(slot->bitmap.pitch);
			unsigned char* data = (unsigned char*)calloc(padded_w * padded_h * 4, 1);
			if (data) {
				// DEBUG: Print bitmap info and sample bytes from middle of glyph
			printf("[LCD] glyph %u: pixel_mode=%d, width=%d, rows=%d, pitch=%d\n",
				glyph_index, slot->bitmap.pixel_mode, slot->bitmap.width, slot->bitmap.rows, pitch);
			if (slot->bitmap.buffer && slot->bitmap.rows > 5) {
				int mid_row = slot->bitmap.rows / 2;
				int mid_col = slot->bitmap.width / 2;
				if (mid_col >= 3) {
					mid_col = (mid_col / 3) * 3;  // Align to pixel boundary
					printf("[LCD] Mid glyph (row=%d, col=%d): bytes %d %d %d\n",
						mid_row, mid_col,
						slot->bitmap.buffer[mid_row*pitch+mid_col],
						slot->bitmap.buffer[mid_row*pitch+mid_col+1],
						slot->bitmap.buffer[mid_row*pitch+mid_col+2]);
				}
			}

			// Copy LCD RGB data to center of padded buffer as RGBA (leaving 1px border)
				for (int y = 0; y < gh; y++) {
					unsigned char* src = slot->bitmap.buffer + y * pitch;
					unsigned char* dst = data + ((y + 1) * padded_w + 1) * 4;

					for (int x = 0; x < gw; x++) {
						// For horizontal LCD, bitmap has 3x width in bytes
						// For vertical LCD, bitmap has 3x height
						int src_offset;
						if (fs->state.subpixelMode == NVG_SUBPIXEL_RGB || fs->state.subpixelMode == NVG_SUBPIXEL_BGR) {
							// Horizontal LCD: each pixel has 3 subpixels side-by-side
							src_offset = x * 3;
						} else {
							// Vertical LCD: subpixels are stacked vertically
							// Each logical pixel still has RGB components in sequence
							src_offset = x * 3;
						}

						unsigned char r = src[src_offset + 0];
						unsigned char g = src[src_offset + 1];
						unsigned char b = src[src_offset + 2];

						// Handle BGR vs RGB ordering
						if (fs->state.subpixelMode == NVG_SUBPIXEL_BGR || fs->state.subpixelMode == NVG_SUBPIXEL_VBGR) {
							// BGR modes - swap red and blue
							unsigned char tmp = r;
							r = b;
							b = tmp;
						}

						dst[x * 4 + 0] = r;
						dst[x * 4 + 1] = g;
						dst[x * 4 + 2] = b;
						dst[x * 4 + 3] = 255;  // Full opacity
					}
				}

				static int lcd_debug = 0;
				if (lcd_debug++ < 3) {
					printf("[Atlas LCD upload #%d] glyph %u to (%d,%d) size %dx%d (glyph %dx%d + 2px padding) subpixel=%d\n",
						lcd_debug, glyph_index, ax, ay, padded_w, padded_h, gw, gh, fs->state.subpixelMode);
					// Debug: print first few pixels
					printf("[LCD DEBUG] First pixel RGB: (%d,%d,%d), bitmap.pixel_mode=%d, bitmap.width=%d\n",
						data[4], data[5], data[6], slot->bitmap.pixel_mode, slot->bitmap.width);
				}
				nvgAtlasUpdate(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, (int)ax, (int)ay, padded_w, padded_h, data);
				free(data);
			}
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
				if (upload_debug++ < 3) {
					printf("[Atlas ALPHA upload #%d] glyph %u to (%d,%d) size %dx%d (glyph %dx%d + 2px padding)\n",
						upload_debug, glyph_index, ax, ay, padded_w, padded_h, gw, gh);
					if (gw > 5 && gh > 5) {
						int mid_y = gh / 2 + 1;  // +1 for padding
						int mid_x = gw / 2 + 1;  // +1 for padding
						printf("[ALPHA DEBUG] Mid pixel [%d,%d]: %d, Corner [1,1]: %d, raw bitmap [%d,%d]: %d\n",
							mid_y, mid_x, data[mid_y * padded_w + mid_x],
							data[1 * padded_w + 1],
							gh/2, gw/2, slot->bitmap.buffer[(gh/2)*pitch + (gw/2)]);
						// Print a small section of the bitmap
						printf("[BITMAP DUMP] First 5x5 pixels:\n");
						for (int y = 0; y < 5 && y < gh; y++) {
							for (int x = 0; x < 5 && x < gw; x++) {
								printf("%3d ", slot->bitmap.buffer[y*pitch + x]);
							}
							printf("\n");
						}
					}
				}
				nvgAtlasUpdate(fs->atlasManager, srcColorSpace, dstColorSpace, format, subpixelMode, (int)ax, (int)ay, padded_w, padded_h, data);
				free(data);
			}
		}
	}

	if (total_calls == 150) {
		printf("[FINAL] Total nvgFontRenderGlyph calls: %d\n", total_calls);
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
