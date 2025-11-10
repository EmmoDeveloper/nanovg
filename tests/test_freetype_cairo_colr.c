#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_COLOR_H
#include <cairo.h>
#include <cairo-ft.h>

// Test FreeType + Cairo COLR rendering directly without NanoVG
int main(void) {
	printf("=== FreeType/Cairo COLR Direct Test ===\n\n");

	// Initialize FreeType
	FT_Library ft_library;
	FT_Error error = FT_Init_FreeType(&ft_library);
	if (error) {
		printf("ERROR: Failed to initialize FreeType: %d\n", error);
		return 1;
	}
	printf("✓ FreeType initialized\n");

	// Load emoji font
	const char* font_path = "fonts/emoji/Noto-COLRv1.ttf";
	FT_Face face;
	error = FT_New_Face(ft_library, font_path, 0, &face);
	if (error) {
		printf("ERROR: Failed to load font '%s': %d\n", font_path, error);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Font loaded: %s\n", face->family_name);
	printf("  Num glyphs: %ld\n", face->num_glyphs);
	printf("  Num faces: %ld\n", face->num_faces);

	// Check for COLR support
	int has_colr = FT_HAS_COLOR(face);
	printf("  Has COLR: %s\n", has_colr ? "YES" : "NO");

	// Get palette info
	FT_Palette_Data palette_data;
	error = FT_Palette_Data_Get(face, &palette_data);
	if (!error) {
		printf("  Palettes: %d\n", palette_data.num_palettes);
		printf("  Palette entries: %d\n", palette_data.num_palette_entries);
	}

	// Test emoji codepoint: U+1F600 (grinning face)
	unsigned int codepoint = 0x1F600;
	printf("\n--- Testing emoji U+%04X ---\n", codepoint);

	// Get glyph index
	FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
	if (glyph_index == 0) {
		printf("ERROR: Glyph not found for U+%04X\n", codepoint);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Glyph index: %u\n", glyph_index);

	// Set size
	int pixel_size = 128;
	error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
	if (error) {
		printf("ERROR: Failed to set pixel size: %d\n", error);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Pixel size set: %d\n", pixel_size);

	// Load glyph
	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_COLOR);
	if (error) {
		printf("ERROR: Failed to load glyph: %d\n", error);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Glyph loaded\n");
	printf("  Format: %c%c%c%c\n",
		(face->glyph->format >> 24) & 0xFF,
		(face->glyph->format >> 16) & 0xFF,
		(face->glyph->format >> 8) & 0xFF,
		face->glyph->format & 0xFF);
	printf("  Metrics: %ld x %ld (1/64 pixels)\n",
		face->glyph->metrics.width,
		face->glyph->metrics.height);

	// Check for COLR layers
	FT_LayerIterator layer_iterator;
	layer_iterator.p = NULL;
	FT_UInt layer_glyph_index;
	FT_UInt layer_color_index;
	int layer_count = 0;
	while (FT_Get_Color_Glyph_Layer(face, glyph_index, &layer_glyph_index, &layer_color_index, &layer_iterator)) {
		layer_count++;
		printf("  Layer %d: glyph=%u, color_index=%u\n", layer_count, layer_glyph_index, layer_color_index);
	}
	if (layer_count == 0) {
		printf("  No COLR v0 layers found\n");
	}

	// Check for COLR v1 paint
	FT_OpaquePaint opaque_paint;
	int has_colr_v1 = FT_Get_Color_Glyph_Paint(face, glyph_index, FT_COLOR_INCLUDE_ROOT_TRANSFORM, &opaque_paint);
	printf("  Has COLR v1: %s\n", has_colr_v1 ? "YES" : "NO");
	if (has_colr_v1) {
		FT_COLR_Paint paint;
		error = FT_Get_Paint(face, opaque_paint, &paint);
		if (!error) {
			printf("  Paint format: %d\n", paint.format);
		}
	}

	// Create Cairo surface for rendering
	int width = 256;
	int height = 256;
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		printf("ERROR: Failed to create Cairo surface\n");
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("\n✓ Cairo surface created: %dx%d\n", width, height);

	cairo_t* cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		printf("ERROR: Failed to create Cairo context\n");
		cairo_surface_destroy(surface);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Cairo context created\n");

	// Clear to white background
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	// Create Cairo font face from FreeType face
	cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(face, 0);
	if (cairo_font_face_status(font_face) != CAIRO_STATUS_SUCCESS) {
		printf("ERROR: Failed to create Cairo font face\n");
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Cairo font face created\n");

	// Set font options for color rendering
	cairo_font_options_t* font_options = cairo_font_options_create();
	cairo_font_options_set_color_mode(font_options, CAIRO_COLOR_MODE_COLOR);
	printf("✓ Color mode set to CAIRO_COLOR_MODE_COLOR\n");

	// Create scaled font
	cairo_matrix_t font_matrix, ctm;
	cairo_matrix_init_scale(&font_matrix, pixel_size, pixel_size);
	cairo_matrix_init_identity(&ctm);
	cairo_scaled_font_t* scaled_font = cairo_scaled_font_create(font_face, &font_matrix, &ctm, font_options);
	if (cairo_scaled_font_status(scaled_font) != CAIRO_STATUS_SUCCESS) {
		printf("ERROR: Failed to create scaled font: %s\n",
			cairo_status_to_string(cairo_scaled_font_status(scaled_font)));
		cairo_font_options_destroy(font_options);
		cairo_font_face_destroy(font_face);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return 1;
	}
	printf("✓ Scaled font created\n");

	cairo_set_scaled_font(cr, scaled_font);

	// Position glyph
	cairo_glyph_t glyph;
	glyph.index = glyph_index;
	glyph.x = 64;  // Offset from left
	glyph.y = 64 + pixel_size;  // Baseline

	printf("\n--- Rendering glyph ---\n");
	printf("  Position: (%.0f, %.0f)\n", glyph.x, glyph.y);

	// Render the glyph
	cairo_show_glyphs(cr, &glyph, 1);

	cairo_status_t status = cairo_status(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		printf("ERROR: Cairo rendering failed: %s\n", cairo_status_to_string(status));
	} else {
		printf("✓ Glyph rendered successfully\n");
	}

	// Flush and analyze the surface
	cairo_surface_flush(surface);
	unsigned char* data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);

	// Check if we have color pixels
	int color_pixel_count = 0;
	int non_white_pixel_count = 0;
	int total_pixels = width * height;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char* pixel = data + y * stride + x * 4;
			// Cairo ARGB32 format (native endian): B G R A
			unsigned char b = pixel[0];
			unsigned char g = pixel[1];
			unsigned char r = pixel[2];
			unsigned char a = pixel[3];

			// Check if not white
			if (r != 255 || g != 255 || b != 255) {
				non_white_pixel_count++;

				// Check if it's a color pixel (not grayscale)
				if (r != g || r != b) {
					color_pixel_count++;
				}
			}
		}
	}

	printf("\n--- Analysis ---\n");
	printf("  Total pixels: %d\n", total_pixels);
	printf("  Non-white pixels: %d (%.1f%%)\n", non_white_pixel_count,
		100.0 * non_white_pixel_count / total_pixels);
	printf("  Color pixels: %d (%.1f%%)\n", color_pixel_count,
		100.0 * color_pixel_count / total_pixels);

	if (color_pixel_count > 0) {
		printf("\n✓ SUCCESS: COLR emoji rendered with color!\n");
	} else if (non_white_pixel_count > 0) {
		printf("\n⚠ WARNING: Glyph rendered but no color detected (grayscale?)\n");
	} else {
		printf("\n✗ FAILED: No pixels rendered\n");
	}

	// Save to PNG
	const char* output_file = "screendumps/freetype_cairo_colr_test.png";
	cairo_status_t write_status = cairo_surface_write_to_png(surface, output_file);
	if (write_status == CAIRO_STATUS_SUCCESS) {
		printf("\n✓ Output saved to: %s\n", output_file);
	} else {
		printf("\n✗ Failed to save PNG: %s\n", cairo_status_to_string(write_status));
	}

	// Cleanup
	cairo_scaled_font_destroy(scaled_font);
	cairo_font_options_destroy(font_options);
	cairo_font_face_destroy(font_face);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	FT_Done_Face(face);
	FT_Done_FreeType(ft_library);

	printf("\n=== Test Complete ===\n");
	return (color_pixel_count > 0) ? 0 : 1;
}
