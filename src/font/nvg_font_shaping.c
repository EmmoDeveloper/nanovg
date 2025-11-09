#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

// Text shaping with HarfBuzz and FriBidi

void nvgFontShapedTextIterInit(NVGFontSystem* fs, NVGTextIter* iter, float x, float y,
                                const char* string, const char* end, int bidi, void* state) {
	if (!fs || !iter || !string) return;
	(void)state; // Reserved for future use

	memset(iter, 0, sizeof(NVGTextIter));
	iter->x = x;
	iter->y = y;
	iter->str = string;
	iter->next = string;
	iter->glyphIndex = 0;

	if (!end) end = string + strlen(string);

	// Determine text direction
	hb_direction_t direction = HB_DIRECTION_LTR;
	FriBidiCharType base_dir = fs->shapingState.base_dir;

	// Bidirectional text processing
	if (bidi && fs->shapingState.bidi_enabled) {
		int text_len = (int)(end - string);

		// Convert UTF-8 to UTF-32 for FriBidi
		FriBidiChar* unicode_str = (FriBidiChar*)malloc(sizeof(FriBidiChar) * (text_len + 1));
		if (unicode_str) {
			FriBidiStrIndex unicode_len = 0;
			const char* p = string;

			while (p < end) {
				unsigned char c = (unsigned char)*p;
				FriBidiChar codepoint = 0;

				if (c < 128) {
					codepoint = c;
					p++;
				} else if ((c & 0xE0) == 0xC0) {
					codepoint = ((c & 0x1F) << 6) | (p[1] & 0x3F);
					p += 2;
				} else if ((c & 0xF0) == 0xE0) {
					codepoint = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
					p += 3;
				} else if ((c & 0xF8) == 0xF0) {
					codepoint = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) |
					            ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
					p += 4;
				} else {
					p++;
					continue;
				}

				unicode_str[unicode_len++] = codepoint;
			}

			// Get character types
			FriBidiCharType* char_types = (FriBidiCharType*)malloc(sizeof(FriBidiCharType) * unicode_len);
			if (char_types) {
				fribidi_get_bidi_types(unicode_str, unicode_len, char_types);

				// Determine base direction if AUTO
				if (base_dir == FRIBIDI_TYPE_ON) {
					// Use first strong character to determine direction
					for (FriBidiStrIndex i = 0; i < unicode_len; i++) {
						if (FRIBIDI_IS_RTL(char_types[i])) {
							base_dir = FRIBIDI_TYPE_RTL;
							break;
						} else if (FRIBIDI_IS_STRONG(char_types[i])) {
							// Strong LTR character
							base_dir = FRIBIDI_TYPE_LTR;
							break;
						}
					}
				}

				// Set HarfBuzz direction based on base direction
				direction = (base_dir == FRIBIDI_TYPE_RTL)
				           ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;

				free(char_types);
			}

			free(unicode_str);
		}
	}

	// Prepare HarfBuzz buffer
	hb_buffer_clear_contents(fs->shapingState.hb_buffer);
	hb_buffer_set_direction(fs->shapingState.hb_buffer, direction);
	hb_buffer_set_script(fs->shapingState.hb_buffer, HB_SCRIPT_COMMON);
	hb_buffer_set_language(fs->shapingState.hb_buffer, hb_language_from_string("en", -1));

	// Add text to buffer
	hb_buffer_add_utf8(fs->shapingState.hb_buffer, string, (int)(end - string), 0, (int)(end - string));

	// Apply OpenType features
	hb_feature_t features[32];
	unsigned int num_features = 0;
	for (int i = 0; i < fs->nfeatures && num_features < 32; i++) {
		features[num_features].tag = hb_tag_from_string(fs->features[i].tag, -1);
		features[num_features].value = fs->features[i].enabled ? 1 : 0;
		features[num_features].start = 0;
		features[num_features].end = (unsigned int)-1;
		num_features++;
	}

	// Shape text if font is set
	if (fs->state.fontId >= 0 && fs->state.fontId < fs->nfonts) {
		FT_Face face = fs->fonts[fs->state.fontId].face;

		static int shape_debug = 0;
		if (shape_debug++ < 10) {
			printf("[nvgFontShapedTextIterInit] Shaping with fontId=%d, varStateId=%u, size=%.1f\n",
				fs->state.fontId, fs->fonts[fs->state.fontId].varStateId, fs->state.size);
		}

		// Set FreeType face size first
		FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fs->state.size);

		// Re-apply variation coordinates to ensure FreeType face has correct settings
		if (fs->fonts[fs->state.fontId].varCoordsCount > 0) {
			FT_Fixed* ft_coords = (FT_Fixed*)malloc(sizeof(FT_Fixed) * fs->fonts[fs->state.fontId].varCoordsCount);
			if (ft_coords) {
				for (unsigned int i = 0; i < fs->fonts[fs->state.fontId].varCoordsCount; i++) {
					ft_coords[i] = (FT_Fixed)(fs->fonts[fs->state.fontId].varCoords[i] * 65536.0f);
				}
				FT_Set_Var_Design_Coordinates(face, fs->fonts[fs->state.fontId].varCoordsCount, ft_coords);
				free(ft_coords);
			}
		}

		// Create a fresh HarfBuzz font for this shaping to pick up current variation settings
		// HarfBuzz will read metrics from the FreeType face at the current size and variation
		hb_font_t* hb_font = hb_ft_font_create(face, NULL);
		if (hb_font) {
			// Note: We don't call hb_font_set_scale() because hb_ft_font_create() already
			// picks up the size from the FreeType face

			if (num_features > 0) {
				hb_shape(hb_font, fs->shapingState.hb_buffer, features, num_features);
			} else {
				hb_shape(hb_font, fs->shapingState.hb_buffer, NULL, 0);
			}

			// Destroy the temporary HarfBuzz font
			hb_font_destroy(hb_font);
		}
	}

	// Bidirectional text processing
	if (bidi && fs->shapingState.bidi_enabled) {
		// FriBidi integration would go here
		// For now, we skip bidirectional reordering
	}
}

int nvgFontShapedTextIterNext(NVGFontSystem* fs, NVGTextIter* iter, NVGCachedGlyph* quad) {
	if (!fs || !iter || !quad) return 0;
	if (fs->state.fontId < 0 || fs->state.fontId >= fs->nfonts) return 0;

	// Get shaped glyphs from HarfBuzz
	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(fs->shapingState.hb_buffer, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(fs->shapingState.hb_buffer, &glyph_count);

	// Check if we've iterated through all glyphs
	if (iter->glyphIndex >= glyph_count) {
		return 0;
	}

	// Get current glyph
	unsigned int glyph_id = glyph_info[iter->glyphIndex].codepoint;
	unsigned int cluster = glyph_info[iter->glyphIndex].cluster;
	float x_offset = (float)glyph_pos[iter->glyphIndex].x_offset / 64.0f;
	float y_offset = (float)glyph_pos[iter->glyphIndex].y_offset / 64.0f;
	float x_advance = (float)glyph_pos[iter->glyphIndex].x_advance / 64.0f;

	// Render glyph and get quad
	if (!nvgFontRenderGlyph(fs, fs->state.fontId, glyph_id,
	                        iter->x + x_offset, iter->y + y_offset, quad)) {
		iter->glyphIndex++;
		iter->x += x_advance + fs->state.spacing;
		return nvgFontShapedTextIterNext(fs, iter, quad);
	}

	// Update iterator position
	iter->x += x_advance + fs->state.spacing;
	iter->glyphIndex++;

	// Update iterator bounds
	iter->x0 = quad->x0;
	iter->y0 = quad->y0;
	iter->x1 = quad->x1;
	iter->y1 = quad->y1;
	iter->s0 = quad->s0;
	iter->t0 = quad->t0;
	iter->s1 = quad->s1;
	iter->t1 = quad->t1;
	iter->codepoint = quad->codepoint;

	return 1;
}

void nvgFontTextIterFree(NVGTextIter* iter) {
	// Iterator doesn't own any resources currently
	(void)iter;
}

int nvgFontTextIterNext(NVGFontSystem* fs, NVGTextIter* iter, NVGCachedGlyph* quad) {
	// Simple unshaped iteration - just render codepoints one by one
	if (!fs || !iter || !quad || !iter->next || *iter->next == '\0') return 0;
	if (fs->state.fontId < 0 || fs->state.fontId >= fs->nfonts) return 0;

	// Decode UTF-8
	unsigned int codepoint = 0;
	const char* str = iter->next;
	unsigned char c = (unsigned char)*str;

	if (c < 128) {
		codepoint = c;
		iter->next = str + 1;
	} else if ((c & 0xE0) == 0xC0) {
		codepoint = ((c & 0x1F) << 6) | (str[1] & 0x3F);
		iter->next = str + 2;
	} else if ((c & 0xF0) == 0xE0) {
		codepoint = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
		iter->next = str + 3;
	} else if ((c & 0xF8) == 0xF0) {
		codepoint = ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) |
		            ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
		iter->next = str + 4;
	} else {
		iter->next = str + 1;
		return nvgFontTextIterNext(fs, iter, quad);
	}

	// Render glyph
	if (!nvgFontRenderGlyph(fs, fs->state.fontId, codepoint, iter->x, iter->y, quad)) {
		return nvgFontTextIterNext(fs, iter, quad);
	}

	// Update position
	iter->x += quad->advanceX + fs->state.spacing;

	// Update iterator bounds
	iter->x0 = quad->x0;
	iter->y0 = quad->y0;
	iter->x1 = quad->x1;
	iter->y1 = quad->y1;
	iter->s0 = quad->s0;
	iter->t0 = quad->t0;
	iter->s1 = quad->s1;
	iter->t1 = quad->t1;
	iter->codepoint = quad->codepoint;

	return 1;
}

// Text measurement

float nvgFontTextBounds(NVGFontSystem* fs, float x, float y, const char* string, const char* end, float* bounds) {
	if (!fs || !string) {
		if (bounds) {
			bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		}
		return 0.0f;
	}

	if (!end) end = string + strlen(string);

	NVGTextIter iter;
	NVGCachedGlyph quad;
	float minx = x, miny = y, maxx = x, maxy = y;
	int first = 1;

	iter.x = x;
	iter.y = y;
	iter.str = string;
	iter.next = string;

	while (iter.next < end && nvgFontTextIterNext(fs, &iter, &quad)) {
		if (first) {
			minx = quad.x0;
			miny = quad.y0;
			maxx = quad.x1;
			maxy = quad.y1;
			first = 0;
		} else {
			if (quad.x0 < minx) minx = quad.x0;
			if (quad.y0 < miny) miny = quad.y0;
			if (quad.x1 > maxx) maxx = quad.x1;
			if (quad.y1 > maxy) maxy = quad.y1;
		}
	}

	if (bounds) {
		bounds[0] = minx;
		bounds[1] = miny;
		bounds[2] = maxx;
		bounds[3] = maxy;
	}

	return iter.x - x;
}

float nvgFontTextBoundsShaped(NVGFontSystem* fs, float x, float y, const char* string, const char* end,
                               float* bounds, int bidi, void* state) {
	if (!fs || !string) {
		if (bounds) {
			bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		}
		return 0.0f;
	}

	if (!end) end = string + strlen(string);
	if (fs->state.fontId < 0 || fs->state.fontId >= fs->nfonts) {
		if (bounds) {
			bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		}
		return 0.0f;
	}

	// Use HarfBuzz extents - it already knows the bounds!
	hb_buffer_t* hb_buffer = hb_buffer_create();
	hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
	hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
	hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
	hb_buffer_add_utf8(hb_buffer, string, (int)(end - string), 0, (int)(end - string));

	// Apply OpenType features
	hb_feature_t features[32];
	unsigned int num_features = 0;
	for (int i = 0; i < fs->nfeatures && num_features < 32; i++) {
		features[num_features].tag = hb_tag_from_string(fs->features[i].tag, -1);
		features[num_features].value = fs->features[i].enabled ? 1 : 0;
		features[num_features].start = 0;
		features[num_features].end = (unsigned int)-1;
		num_features++;
	}

	// Shape with HarfBuzz
	hb_font_t* hb_font = fs->fonts[fs->state.fontId].hb_font;
	int scale = (int)(fs->state.size * 64.0f);
	hb_font_set_scale(hb_font, scale, scale);

	if (num_features > 0) {
		hb_shape(hb_font, hb_buffer, features, num_features);
	} else {
		hb_shape(hb_font, hb_buffer, NULL, 0);
	}

	// Get shaped glyphs and use HarfBuzz font extents
	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

	// Let HarfBuzz tell us the extents directly
	hb_font_extents_t font_extents;
	hb_font_get_h_extents(hb_font, &font_extents);

	float cur_x = x;
	float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;

	// Use HarfBuzz glyph extents (no FreeType needed!)
	for (unsigned int i = 0; i < glyph_count; i++) {
		hb_glyph_extents_t extents;
		if (hb_font_get_glyph_extents(hb_font, glyph_info[i].codepoint, &extents)) {
			float x_offset = (float)glyph_pos[i].x_offset / 64.0f;
			float y_offset = (float)glyph_pos[i].y_offset / 64.0f;
			float x_advance = (float)glyph_pos[i].x_advance / 64.0f;

			// HarfBuzz extents in Y-up coordinates:
			// y_bearing = distance from baseline to glyph top (positive = above baseline)
			// height = signed vertical span (can be negative!)
			// Glyph bounds in Y-up: top = y_bearing, bottom = y_bearing + height
			// For Vulkan (Y-down, baseline at y):
			float glyph_top_y_up = (float)extents.y_bearing / 64.0f;
			float glyph_bottom_y_up = glyph_top_y_up + (float)extents.height / 64.0f;

			float glyph_x0 = cur_x + x_offset + (float)extents.x_bearing / 64.0f;
			float glyph_y0 = y - y_offset - glyph_top_y_up;      // Top in Vulkan (smaller Y)
			float glyph_x1 = glyph_x0 + (float)extents.width / 64.0f;
			float glyph_y1 = y - y_offset - glyph_bottom_y_up;  // Bottom in Vulkan (larger Y)

			if (glyph_x0 < minx) minx = glyph_x0;
			if (glyph_y0 < miny) miny = glyph_y0;
			if (glyph_x1 > maxx) maxx = glyph_x1;
			if (glyph_y1 > maxy) maxy = glyph_y1;

			cur_x += x_advance + fs->state.spacing;
		}
	}

	// Clamp bounds if no glyphs were processed
	if (minx > maxx) {
		minx = x;
		maxx = x;
		miny = y;
		maxy = y;
	}

	hb_buffer_destroy(hb_buffer);

	if (bounds) {
		bounds[0] = minx;
		bounds[1] = miny;
		bounds[2] = maxx;
		bounds[3] = maxy;
	}

	return cur_x - x;
}

// OpenType features

void nvgFontSetFeature(NVGFontSystem* fs, const char* tag, int enabled) {
	if (!fs || !tag) return;

	// Check if feature already exists
	for (int i = 0; i < fs->nfeatures; i++) {
		if (strcmp(fs->features[i].tag, tag) == 0) {
			fs->features[i].enabled = enabled;
			return;
		}
	}

	// Add new feature
	if (fs->nfeatures < 32) {
		strncpy(fs->features[fs->nfeatures].tag, tag, 4);
		fs->features[fs->nfeatures].tag[4] = '\0';
		fs->features[fs->nfeatures].enabled = enabled;
		fs->nfeatures++;
	}
}

void nvgFontResetFeatures(NVGFontSystem* fs) {
	if (!fs) return;
	fs->nfeatures = 0;
}

// Font configuration

void nvgFontSetHinting(NVGFontSystem* fs, int hinting) {
	if (!fs) return;
	fs->state.hinting = hinting;
}

void nvgFontSetTextDirection(NVGFontSystem* fs, int direction) {
	if (!fs) return;
	// Map NVGtextDirection to FriBidiCharType
	switch (direction) {
		case 0: // NVG_TEXT_DIR_AUTO
			fs->shapingState.base_dir = FRIBIDI_TYPE_ON;
			break;
		case 1: // NVG_TEXT_DIR_LTR
			fs->shapingState.base_dir = FRIBIDI_TYPE_LTR;
			break;
		case 2: // NVG_TEXT_DIR_RTL
			fs->shapingState.base_dir = FRIBIDI_TYPE_RTL;
			break;
		default:
			fs->shapingState.base_dir = FRIBIDI_TYPE_ON;
			break;
	}
}

void nvgFontSetKerning(NVGFontSystem* fs, int enabled) {
	if (!fs) return;
	fs->state.kerningEnabled = enabled;
}
