#include "nvg_font_colr.h"
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to resolve FT_ColorIndex to FT_Color
static int nvg__resolveColorIndex(FT_Face face, FT_ColorIndex color_index, FT_Color* out_color) {
	// Get the palette (use palette 0 by default)
	FT_Color* palette_colors = NULL;

	if (FT_Palette_Select(face, 0, &palette_colors) != 0) {
		return 0;
	}

	// Get palette data to check bounds
	FT_Palette_Data palette_data;
	if (FT_Palette_Data_Get(face, &palette_data) != 0) {
		return 0;
	}

	if (color_index.palette_index >= palette_data.num_palette_entries) {
		return 0;
	}

	// Get the base color
	*out_color = palette_colors[color_index.palette_index];

	// Apply alpha modifier (FT_F2Dot14 is 2.14 fixed point, range 0-16384 maps to 0-1)
	if (color_index.alpha != 0x4000) {  // 0x4000 = 1.0 in 2.14 fixed point
		float alpha_mult = color_index.alpha / 16384.0f;
		out_color->alpha = (FT_Byte)(out_color->alpha * alpha_mult);
	}

	return 1;
}

// Initialize Cairo state for COLR emoji rendering
void nvg__initCairoState(NVGFontSystem* fs) {
	if (!fs) return;

	// Start with a small surface, we'll resize as needed
	fs->cairoState.surfaceWidth = 128;
	fs->cairoState.surfaceHeight = 128;
	fs->cairoState.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
	                                                      fs->cairoState.surfaceWidth,
	                                                      fs->cairoState.surfaceHeight);
	if (cairo_surface_status(fs->cairoState.surface) != CAIRO_STATUS_SUCCESS) {
		fs->cairoState.surface = NULL;
		fs->cairoState.cr = NULL;
		return;
	}

	fs->cairoState.cr = cairo_create(fs->cairoState.surface);
	if (cairo_status(fs->cairoState.cr) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(fs->cairoState.surface);
		fs->cairoState.surface = NULL;
		fs->cairoState.cr = NULL;
		return;
	}
}

// Destroy Cairo state
void nvg__destroyCairoState(NVGFontSystem* fs) {
	if (!fs) return;

	if (fs->cairoState.cr) {
		cairo_destroy(fs->cairoState.cr);
		fs->cairoState.cr = NULL;
	}
	if (fs->cairoState.surface) {
		cairo_surface_destroy(fs->cairoState.surface);
		fs->cairoState.surface = NULL;
	}
}

// Ensure Cairo surface is large enough for glyph
static void nvg__ensureCairoSize(NVGFontSystem* fs, int width, int height) {
	if (width <= fs->cairoState.surfaceWidth && height <= fs->cairoState.surfaceHeight) {
		return;  // Current surface is large enough
	}

	// Destroy old surface
	if (fs->cairoState.cr) cairo_destroy(fs->cairoState.cr);
	if (fs->cairoState.surface) cairo_surface_destroy(fs->cairoState.surface);

	// Create larger surface
	fs->cairoState.surfaceWidth = width > fs->cairoState.surfaceWidth ? width : fs->cairoState.surfaceWidth;
	fs->cairoState.surfaceHeight = height > fs->cairoState.surfaceHeight ? height : fs->cairoState.surfaceHeight;

	fs->cairoState.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
	                                                      fs->cairoState.surfaceWidth,
	                                                      fs->cairoState.surfaceHeight);
	fs->cairoState.cr = cairo_create(fs->cairoState.surface);
}

// Convert FreeType composite mode to Cairo operator
static cairo_operator_t nvg__ftCompositeToCairo(FT_Composite_Mode mode) {
	switch (mode) {
		case FT_COLR_COMPOSITE_CLEAR:          return CAIRO_OPERATOR_CLEAR;
		case FT_COLR_COMPOSITE_SRC:            return CAIRO_OPERATOR_SOURCE;
		case FT_COLR_COMPOSITE_DEST:           return CAIRO_OPERATOR_DEST;
		case FT_COLR_COMPOSITE_SRC_OVER:       return CAIRO_OPERATOR_OVER;
		case FT_COLR_COMPOSITE_DEST_OVER:      return CAIRO_OPERATOR_DEST_OVER;
		case FT_COLR_COMPOSITE_SRC_IN:         return CAIRO_OPERATOR_IN;
		case FT_COLR_COMPOSITE_DEST_IN:        return CAIRO_OPERATOR_DEST_IN;
		case FT_COLR_COMPOSITE_SRC_OUT:        return CAIRO_OPERATOR_OUT;
		case FT_COLR_COMPOSITE_DEST_OUT:       return CAIRO_OPERATOR_DEST_OUT;
		case FT_COLR_COMPOSITE_SRC_ATOP:       return CAIRO_OPERATOR_ATOP;
		case FT_COLR_COMPOSITE_DEST_ATOP:      return CAIRO_OPERATOR_DEST_ATOP;
		case FT_COLR_COMPOSITE_XOR:            return CAIRO_OPERATOR_XOR;
		case FT_COLR_COMPOSITE_PLUS:           return CAIRO_OPERATOR_ADD;
		case FT_COLR_COMPOSITE_SCREEN:         return CAIRO_OPERATOR_SCREEN;
		case FT_COLR_COMPOSITE_OVERLAY:        return CAIRO_OPERATOR_OVERLAY;
		case FT_COLR_COMPOSITE_DARKEN:         return CAIRO_OPERATOR_DARKEN;
		case FT_COLR_COMPOSITE_LIGHTEN:        return CAIRO_OPERATOR_LIGHTEN;
		case FT_COLR_COMPOSITE_COLOR_DODGE:    return CAIRO_OPERATOR_COLOR_DODGE;
		case FT_COLR_COMPOSITE_COLOR_BURN:     return CAIRO_OPERATOR_COLOR_BURN;
		case FT_COLR_COMPOSITE_HARD_LIGHT:     return CAIRO_OPERATOR_HARD_LIGHT;
		case FT_COLR_COMPOSITE_SOFT_LIGHT:     return CAIRO_OPERATOR_SOFT_LIGHT;
		case FT_COLR_COMPOSITE_DIFFERENCE:     return CAIRO_OPERATOR_DIFFERENCE;
		case FT_COLR_COMPOSITE_EXCLUSION:      return CAIRO_OPERATOR_EXCLUSION;
		case FT_COLR_COMPOSITE_MULTIPLY:       return CAIRO_OPERATOR_MULTIPLY;
		case FT_COLR_COMPOSITE_HSL_HUE:        return CAIRO_OPERATOR_HSL_HUE;
		case FT_COLR_COMPOSITE_HSL_SATURATION: return CAIRO_OPERATOR_HSL_SATURATION;
		case FT_COLR_COMPOSITE_HSL_COLOR:      return CAIRO_OPERATOR_HSL_COLOR;
		case FT_COLR_COMPOSITE_HSL_LUMINOSITY: return CAIRO_OPERATOR_HSL_LUMINOSITY;
		default:                               return CAIRO_OPERATOR_OVER;
	}
}

// Convert FreeType outline to Cairo path
static void nvg__outlineToCairoPath(FT_Outline* outline, cairo_t* cr) {
	FT_Vector* points = outline->points;
	char* tags = outline->tags;
	short* contours = outline->contours;
	int n_contours = outline->n_contours;

	int start = 0;
	for (int i = 0; i < n_contours; i++) {
		int end = contours[i];

		// Move to start point
		cairo_move_to(cr, points[start].x / 64.0, points[start].y / 64.0);

		int j = start;
		while (j <= end) {
			if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON) {
				// On-curve point - line to
				cairo_line_to(cr, points[j].x / 64.0, points[j].y / 64.0);
				j++;
			} else if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CONIC) {
				// Quadratic Bezier
				int control = j;
				int next = (j < end) ? j + 1 : start;

				// Convert quadratic to cubic Bezier
				double x0, y0;
				cairo_get_current_point(cr, &x0, &y0);
				double x1 = points[control].x / 64.0;
				double y1 = points[control].y / 64.0;
				double x3 = points[next].x / 64.0;
				double y3 = points[next].y / 64.0;

				// Cubic control points for quadratic approximation
				double cx1 = x0 + 2.0/3.0 * (x1 - x0);
				double cy1 = y0 + 2.0/3.0 * (y1 - y0);
				double cx2 = x3 + 2.0/3.0 * (x1 - x3);
				double cy2 = y3 + 2.0/3.0 * (y1 - y3);

				cairo_curve_to(cr, cx1, cy1, cx2, cy2, x3, y3);
				j += 2;
			} else {
				// Cubic Bezier
				int cp1 = j;
				int cp2 = j + 1;
				int next = (j + 2 <= end) ? j + 2 : start;

				cairo_curve_to(cr,
				              points[cp1].x / 64.0, points[cp1].y / 64.0,
				              points[cp2].x / 64.0, points[cp2].y / 64.0,
				              points[next].x / 64.0, points[next].y / 64.0);
				j += 3;
			}
		}

		cairo_close_path(cr);
		start = end + 1;
	}
}

// Recursive paint rendering (forward declaration)
static int nvg__renderPaint(NVGFontSystem* fs, FT_Face face, FT_OpaquePaint* opaque_paint);

// Render a COLR glyph using Cairo
// Returns 1 on success, 0 on failure
// rgba_data will be allocated and must be freed by caller
int nvg__renderCOLRGlyph(NVGFontSystem* fs, FT_Face face, unsigned int glyph_index,
                         int width, int height, unsigned char** rgba_data,
                         float bearingX, float bearingY) {
	if (!fs || !face || !rgba_data) {
		return 0;
	}

	// Ensure Cairo surface is large enough
	nvg__ensureCairoSize(fs, width, height);

	cairo_t* cr = fs->cairoState.cr;

	// Clear surface
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);

	// Create Cairo font face from FreeType face
	cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(face, 0);
	if (cairo_font_face_status(font_face) != CAIRO_STATUS_SUCCESS) {
		return 0;
	}

	// Set font options for color rendering
	cairo_font_options_t* font_options = cairo_font_options_create();
	cairo_font_options_set_color_mode(font_options, CAIRO_COLOR_MODE_COLOR);

	// Create scaled font - use the face's current size to match the clip box coordinates
	// The clip box from FreeType is computed at the face's current size
	float font_size = (float)(face->size->metrics.y_ppem);
	cairo_matrix_t font_matrix, ctm;
	cairo_matrix_init_scale(&font_matrix, (double)font_size, (double)font_size);
	cairo_matrix_init_identity(&ctm);
	cairo_scaled_font_t* scaled_font = cairo_scaled_font_create(font_face, &font_matrix, &ctm, font_options);

	if (cairo_scaled_font_status(scaled_font) != CAIRO_STATUS_SUCCESS) {
		cairo_font_options_destroy(font_options);
		cairo_font_face_destroy(font_face);
		return 0;
	}

	cairo_set_scaled_font(cr, scaled_font);

	// Position glyph using the provided bearings
	// bearingX is the horizontal offset from origin to left edge
	// bearingY is the vertical distance from baseline to top edge
	cairo_glyph_t glyph;
	glyph.index = glyph_index;
	glyph.x = -bearingX;  // Offset so left edge aligns at x=0
	glyph.y = bearingY;   // Baseline positioned so top edge aligns at y=0

	// Render the glyph - Cairo automatically handles COLR
	cairo_show_glyphs(cr, &glyph, 1);

	cairo_status_t status = cairo_status(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		cairo_scaled_font_destroy(scaled_font);
		cairo_font_options_destroy(font_options);
		cairo_font_face_destroy(font_face);
		return 0;
	}

	// Cleanup Cairo objects
	cairo_scaled_font_destroy(scaled_font);
	cairo_font_options_destroy(font_options);
	cairo_font_face_destroy(font_face);

	// Extract RGBA data from Cairo surface
	cairo_surface_flush(fs->cairoState.surface);
	unsigned char* surface_data = cairo_image_surface_get_data(fs->cairoState.surface);
	int stride = cairo_image_surface_get_stride(fs->cairoState.surface);

	// Allocate output buffer
	*rgba_data = (unsigned char*)malloc(width * height * 4);
	if (!*rgba_data) return 0;

	// Copy and convert from Cairo's ARGB32 (native endian) to premultiplied RGBA
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char* src = surface_data + y * stride + x * 4;
			unsigned char* dst = *rgba_data + (y * width + x) * 4;

			// Cairo ARGB32 format (native endian): B G R A
			unsigned char b = src[0];
			unsigned char g = src[1];
			unsigned char r = src[2];
			unsigned char a = src[3];

			// Cairo already premultiplies, so just convert to RGBA
			dst[0] = r;
			dst[1] = g;
			dst[2] = b;
			dst[3] = a;
		}
	}

	return 1;
}

// Render a paint node recursively
static int nvg__renderPaint(NVGFontSystem* fs, FT_Face face, FT_OpaquePaint* opaque_paint) {
	FT_COLR_Paint paint;
	memset(&paint, 0, sizeof(paint));

	// FT_Get_Paint returns FT_Bool: 1 on success, 0 on failure
	FT_Bool success = FT_Get_Paint(face, *opaque_paint, &paint);

	if (!success) {
		return 0;
	}

	cairo_t* cr = fs->cairoState.cr;

	switch (paint.format) {
		case FT_COLR_PAINTFORMAT_COLR_LAYERS: {
			// COLRv0 format - iterate through layers
			FT_LayerIterator layer_iterator = paint.u.colr_layers.layer_iterator;
			FT_OpaquePaint layer_paint;

			while (FT_Get_Paint_Layers(face, &layer_iterator, &layer_paint)) {
				if (!nvg__renderPaint(fs, face, &layer_paint)) {
					return 0;
				}
			}
			break;
		}

		case FT_COLR_PAINTFORMAT_SOLID: {
			// Solid color paint
			FT_Color color;
			if (!nvg__resolveColorIndex(face, paint.u.solid.color, &color)) {
				return 0;
			}
			cairo_set_source_rgba(cr,
			                      color.red / 255.0,
			                      color.green / 255.0,
			                      color.blue / 255.0,
			                      color.alpha / 255.0);
			cairo_paint(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT: {
			// Linear gradient
			FT_ColorLine* colorline = &paint.u.linear_gradient.colorline;

			// Create Cairo gradient
			cairo_pattern_t* pattern = cairo_pattern_create_linear(
				paint.u.linear_gradient.p0.x / 65536.0,
				paint.u.linear_gradient.p0.y / 65536.0,
				paint.u.linear_gradient.p1.x / 65536.0,
				paint.u.linear_gradient.p1.y / 65536.0
			);

			// Add color stops
			FT_ColorStop stop;
			FT_ColorStopIterator iterator = colorline->color_stop_iterator;
			while (FT_Get_Colorline_Stops(face, &stop, &iterator)) {
				FT_Color color;
				if (!nvg__resolveColorIndex(face, stop.color, &color)) {
					cairo_pattern_destroy(pattern);
					return 0;
				}
				cairo_pattern_add_color_stop_rgba(pattern,
				                                   stop.stop_offset / 65536.0,  // FT_Fixed is 16.16 fixed point
				                                   color.red / 255.0,
				                                   color.green / 255.0,
				                                   color.blue / 255.0,
				                                   color.alpha / 255.0);
			}

			// Set extend mode
			switch (colorline->extend) {
				case FT_COLR_PAINT_EXTEND_PAD:     cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD); break;
				case FT_COLR_PAINT_EXTEND_REPEAT:  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT); break;
				case FT_COLR_PAINT_EXTEND_REFLECT: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT); break;
			}

			cairo_set_source(cr, pattern);
			cairo_paint(cr);
			cairo_pattern_destroy(pattern);
			break;
		}

		case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT: {
			// Radial gradient
			FT_ColorLine* colorline = &paint.u.radial_gradient.colorline;

			cairo_pattern_t* pattern = cairo_pattern_create_radial(
				paint.u.radial_gradient.c0.x / 65536.0,
				paint.u.radial_gradient.c0.y / 65536.0,
				paint.u.radial_gradient.r0 / 65536.0,
				paint.u.radial_gradient.c1.x / 65536.0,
				paint.u.radial_gradient.c1.y / 65536.0,
				paint.u.radial_gradient.r1 / 65536.0
			);

			// Add color stops
			FT_ColorStop stop;
			FT_ColorStopIterator iterator = colorline->color_stop_iterator;
			while (FT_Get_Colorline_Stops(face, &stop, &iterator)) {
				FT_Color color;
				if (!nvg__resolveColorIndex(face, stop.color, &color)) {
					cairo_pattern_destroy(pattern);
					return 0;
				}
				cairo_pattern_add_color_stop_rgba(pattern,
				                                   stop.stop_offset / 65536.0,  // FT_Fixed is 16.16 fixed point
				                                   color.red / 255.0,
				                                   color.green / 255.0,
				                                   color.blue / 255.0,
				                                   color.alpha / 255.0);
			}

			// Set extend mode
			switch (colorline->extend) {
				case FT_COLR_PAINT_EXTEND_PAD:     cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD); break;
				case FT_COLR_PAINT_EXTEND_REPEAT:  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT); break;
				case FT_COLR_PAINT_EXTEND_REFLECT: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT); break;
			}

			cairo_set_source(cr, pattern);
			cairo_paint(cr);
			cairo_pattern_destroy(pattern);
			break;
		}

		case FT_COLR_PAINTFORMAT_GLYPH: {
			// Paint another glyph outline
			FT_UInt base_glyph = paint.u.glyph.glyphID;
			FT_Error glyph_error = FT_Load_Glyph(face, base_glyph, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
			if (!glyph_error && face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
				// Convert FreeType outline to Cairo path
				nvg__outlineToCairoPath(&face->glyph->outline, cr);

				// Recursively render the child paint
				nvg__renderPaint(fs, face, &paint.u.glyph.paint);

				// Fill the path
				cairo_fill(cr);
			}
			break;
		}

		case FT_COLR_PAINTFORMAT_TRANSFORM: {
			// Apply affine transform
			FT_Affine23* affine = &paint.u.transform.affine;
			cairo_matrix_t matrix;
			cairo_matrix_init(&matrix,
			                  affine->xx / 65536.0, affine->yx / 65536.0,
			                  affine->xy / 65536.0, affine->yy / 65536.0,
			                  affine->dx / 65536.0, affine->dy / 65536.0);

			cairo_save(cr);
			cairo_transform(cr, &matrix);
			nvg__renderPaint(fs, face, &paint.u.transform.paint);
			cairo_restore(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_COMPOSITE: {
			// Composite two paints
			// Render backdrop
			cairo_push_group(cr);
			nvg__renderPaint(fs, face, &paint.u.composite.backdrop_paint);
			cairo_pop_group_to_source(cr);
			cairo_paint(cr);

			// Render source with composite operator
			cairo_push_group(cr);
			nvg__renderPaint(fs, face, &paint.u.composite.source_paint);
			cairo_pop_group_to_source(cr);

			cairo_operator_t op = nvg__ftCompositeToCairo(paint.u.composite.composite_mode);
			cairo_set_operator(cr, op);
			cairo_paint(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);  // Reset to default
			break;
		}

		case FT_COLR_PAINTFORMAT_TRANSLATE: {
			// Apply translation transform
			cairo_save(cr);
			cairo_translate(cr,
			                paint.u.translate.dx / 65536.0,
			                paint.u.translate.dy / 65536.0);
			nvg__renderPaint(fs, face, &paint.u.translate.paint);
			cairo_restore(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_SCALE: {
			// Apply scale transform
			cairo_save(cr);
			// Scale around center point
			cairo_translate(cr,
			                paint.u.scale.center_x / 65536.0,
			                paint.u.scale.center_y / 65536.0);
			cairo_scale(cr,
			            paint.u.scale.scale_x / 65536.0,
			            paint.u.scale.scale_y / 65536.0);
			cairo_translate(cr,
			                -(paint.u.scale.center_x / 65536.0),
			                -(paint.u.scale.center_y / 65536.0));
			nvg__renderPaint(fs, face, &paint.u.scale.paint);
			cairo_restore(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_ROTATE: {
			// Apply rotation transform
			float angle_radians = (paint.u.rotate.angle / 65536.0) * (M_PI / 180.0);
			cairo_save(cr);
			// Rotate around center point
			cairo_translate(cr,
			                paint.u.rotate.center_x / 65536.0,
			                paint.u.rotate.center_y / 65536.0);
			cairo_rotate(cr, angle_radians);
			cairo_translate(cr,
			                -(paint.u.rotate.center_x / 65536.0),
			                -(paint.u.rotate.center_y / 65536.0));
			nvg__renderPaint(fs, face, &paint.u.rotate.paint);
			cairo_restore(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_SKEW: {
			// Apply skew transform
			float x_skew_radians = (paint.u.skew.x_skew_angle / 65536.0) * (M_PI / 180.0);
			float y_skew_radians = (paint.u.skew.y_skew_angle / 65536.0) * (M_PI / 180.0);

			cairo_matrix_t skew_matrix;
			cairo_matrix_init(&skew_matrix,
			                  1.0, tan(y_skew_radians),
			                  tan(x_skew_radians), 1.0,
			                  0.0, 0.0);

			cairo_save(cr);
			// Skew around center point
			cairo_translate(cr,
			                paint.u.skew.center_x / 65536.0,
			                paint.u.skew.center_y / 65536.0);
			cairo_transform(cr, &skew_matrix);
			cairo_translate(cr,
			                -(paint.u.skew.center_x / 65536.0),
			                -(paint.u.skew.center_y / 65536.0));
			nvg__renderPaint(fs, face, &paint.u.skew.paint);
			cairo_restore(cr);
			break;
		}

		case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT: {
			// Sweep (conic) gradient
			FT_ColorLine* colorline = &paint.u.sweep_gradient.colorline;

			// Cairo doesn't have native conic gradients, so we approximate with a radial gradient
			// For production use, consider using a custom shader or mesh pattern for true conic gradients

			float center_x = paint.u.sweep_gradient.center.x / 65536.0;
			float center_y = paint.u.sweep_gradient.center.y / 65536.0;

			// Use radial gradient as approximation
			cairo_pattern_t* pattern = cairo_pattern_create_radial(
				center_x, center_y, 0.0,
				center_x, center_y, 100.0);

			// Add color stops using iterator
			FT_ColorStop stop;
			FT_ColorStopIterator iterator = colorline->color_stop_iterator;
			while (FT_Get_Colorline_Stops(face, &stop, &iterator)) {
				FT_Color color;
				if (!nvg__resolveColorIndex(face, stop.color, &color)) {
					cairo_pattern_destroy(pattern);
					return 0;
				}
				cairo_pattern_add_color_stop_rgba(pattern,
				                                   stop.stop_offset / 65536.0,
				                                   color.red / 255.0,
				                                   color.green / 255.0,
				                                   color.blue / 255.0,
				                                   color.alpha / 255.0);
			}

			// Set extend mode
			switch (colorline->extend) {
				case FT_COLR_PAINT_EXTEND_PAD:     cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD); break;
				case FT_COLR_PAINT_EXTEND_REPEAT:  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT); break;
				case FT_COLR_PAINT_EXTEND_REFLECT: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT); break;
			}

			cairo_set_source(cr, pattern);
			cairo_paint(cr);
			cairo_pattern_destroy(pattern);
			break;
		}

		default:
			// Unsupported paint format - return success anyway to allow fallback
			return 1;
	}

	return 1;
}
