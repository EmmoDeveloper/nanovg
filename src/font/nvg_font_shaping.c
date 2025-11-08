#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>

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

	// Prepare HarfBuzz buffer
	hb_buffer_clear_contents(fs->shapingState.hb_buffer);
	hb_buffer_set_direction(fs->shapingState.hb_buffer, HB_DIRECTION_LTR);
	hb_buffer_set_script(fs->shapingState.hb_buffer, HB_SCRIPT_LATIN);
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
		hb_font_t* hb_font = fs->fonts[fs->state.fontId].hb_font;
		if (hb_font) {
			// Set HarfBuzz font scale to match font size
			// HarfBuzz uses 26.6 fixed point (units of 1/64), FreeType uses pixels
			// Scale both x and y to the font size in pixels * 64
			int scale = (int)(fs->state.size * 64.0f);
			hb_font_set_scale(hb_font, scale, scale);

			if (num_features > 0) {
				hb_shape(hb_font, fs->shapingState.hb_buffer, features, num_features);
			} else {
				hb_shape(hb_font, fs->shapingState.hb_buffer, NULL, 0);
			}
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
	float x_offset = (float)glyph_pos[iter->glyphIndex].x_offset / 64.0f;
	float y_offset = (float)glyph_pos[iter->glyphIndex].y_offset / 64.0f;
	float x_advance = (float)glyph_pos[iter->glyphIndex].x_advance / 64.0f;

	static int glyph_debug = 0;
	if (glyph_debug++ < 25) {
		printf("[nvgFontShapedTextIterNext] cluster=%u glyph_id=%u x_advance=%.1f\n",
			glyph_info[iter->glyphIndex].cluster, glyph_id, x_advance);
	}

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

	NVGTextIter iter;
	NVGCachedGlyph quad;
	float minx = x, miny = y, maxx = x, maxy = y;
	int first = 1;

	nvgFontShapedTextIterInit(fs, &iter, x, y, string, end, bidi, state);

	while (nvgFontShapedTextIterNext(fs, &iter, &quad)) {
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

void nvgFontSetKerning(NVGFontSystem* fs, int enabled) {
	if (!fs) return;
	fs->state.kerningEnabled = enabled;
}
