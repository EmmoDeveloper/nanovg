#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

// Test Chinese poem rendering with colors
// Using 静夜思 (Quiet Night Thought) by Li Bai (李白)

int main() {
	printf("=== Chinese Poem Color Rendering Test ===\n\n");

	// Initialize FreeType
	FT_Library library;
	if (FT_Init_FreeType(&library) != 0) {
		fprintf(stderr, "Failed to init FreeType\n");
		return 1;
	}

	// Load Chinese font
	FT_Face face;
	if (FT_New_Face(library, "fonts/cjk/NotoSansCJKsc-Regular.otf", 0, &face) != 0) {
		fprintf(stderr, "Failed to load Chinese font\n");
		FT_Done_FreeType(library);
		return 1;
	}

	printf("Loaded font: %s %s\n", face->family_name, face->style_name);
	printf("Num glyphs: %ld\n\n", face->num_glyphs);

	// Set font size
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Create Cairo surface (800x400 for poem)
	int width = 800, height = 400;
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t* cr = cairo_create(surface);

	// Background - gradient from dark blue to purple
	cairo_pattern_t* bg_pattern = cairo_pattern_create_linear(0, 0, 0, height);
	cairo_pattern_add_color_stop_rgb(bg_pattern, 0, 0.1, 0.1, 0.3);  // Dark blue
	cairo_pattern_add_color_stop_rgb(bg_pattern, 1, 0.2, 0.1, 0.3);  // Dark purple
	cairo_set_source(cr, bg_pattern);
	cairo_paint(cr);
	cairo_pattern_destroy(bg_pattern);

	// Create Cairo font face
	cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(face, 0);
	cairo_set_font_face(cr, font_face);
	cairo_set_font_size(cr, 48);

	// 静夜思 (Quiet Night Thought) by Li Bai
	const char* poem[] = {
		"静夜思",           // Title
		"",
		"床前明月光，",     // Line 1: Before my bed, the bright moonlight
		"疑是地上霜。",     // Line 2: I suspect it's frost on the ground
		"举头望明月，",     // Line 3: Lifting my head, I gaze at the bright moon
		"低头思故乡。",     // Line 4: Lowering my head, I think of my hometown
		"",
		"— 李白"            // Author: Li Bai
	};

	// Colors for different lines
	double colors[][3] = {
		{1.0, 0.9, 0.5},   // Title - gold
		{0.0, 0.0, 0.0},   // Empty
		{0.9, 0.9, 1.0},   // Line 1 - bright white-blue
		{0.8, 0.9, 1.0},   // Line 2 - light blue
		{1.0, 0.9, 0.7},   // Line 3 - warm white
		{0.9, 0.8, 1.0},   // Line 4 - light purple
		{0.0, 0.0, 0.0},   // Empty
		{0.7, 0.7, 0.8}    // Author - gray
	};

	printf("Rendering poem:\n");

	int y = 80;  // Starting Y position
	for (int i = 0; i < 8; i++) {
		if (strlen(poem[i]) == 0) {
			y += 20;  // Smaller spacing for empty lines
			continue;
		}

		printf("  Line %d: %s\n", i, poem[i]);

		// Set color
		cairo_set_source_rgb(cr, colors[i][0], colors[i][1], colors[i][2]);

		// Center the text
		cairo_text_extents_t extents;
		cairo_text_extents(cr, poem[i], &extents);
		double x = (width - extents.width) / 2.0 - extents.x_bearing;

		// Add subtle glow effect for title
		if (i == 0) {
			cairo_set_source_rgba(cr, 1.0, 0.9, 0.5, 0.3);
			for (int glow = 0; glow < 3; glow++) {
				cairo_move_to(cr, x + glow, y + glow);
				cairo_show_text(cr, poem[i]);
				cairo_move_to(cr, x - glow, y - glow);
				cairo_show_text(cr, poem[i]);
			}
			cairo_set_source_rgb(cr, colors[i][0], colors[i][1], colors[i][2]);
		}

		// Render text
		cairo_move_to(cr, x, y);
		cairo_show_text(cr, poem[i]);

		// Line spacing
		if (i == 0) {
			y += 70;  // Extra space after title
		} else if (i == 7) {
			y += 50;  // Normal spacing for author
		} else {
			y += 60;  // Normal line spacing
		}
	}

	// Add decorative elements - stars
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
	for (int i = 0; i < 20; i++) {
		double star_x = (i * 37 + 50) % width;
		double star_y = (i * 23 + 30) % (height / 2);
		cairo_arc(cr, star_x, star_y, 1 + (i % 3), 0, 2 * 3.14159);
		cairo_fill(cr);
	}

	// Check for Cairo errors
	cairo_status_t status = cairo_status(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "Cairo error: %s\n", cairo_status_to_string(status));
		cairo_font_face_destroy(font_face);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		return 1;
	}

	// Save to PNG
	cairo_surface_write_to_png(surface, "build/test/screendumps/chinese_poem_output.png");
	printf("\nSUCCESS: Rendered Chinese poem to build/test/screendumps/chinese_poem_output.png\n");

	// Print poem information
	printf("\nPoem Information:\n");
	printf("  Title: 静夜思 (Jìng Yè Sī - Quiet Night Thought)\n");
	printf("  Author: 李白 (Lǐ Bái - Li Bai, 701-762 CE)\n");
	printf("  Dynasty: Tang Dynasty (唐朝)\n");
	printf("  Theme: Homesickness under moonlight\n");
	printf("\nTranslation:\n");
	printf("  Before my bed, the bright moonlight,\n");
	printf("  I suspect it's frost on the ground.\n");
	printf("  Lifting my head, I gaze at the bright moon,\n");
	printf("  Lowering my head, I think of my hometown.\n");

	// Cleanup
	cairo_font_face_destroy(font_face);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	FT_Done_Face(face);
	FT_Done_FreeType(library);

	printf("\n=== Chinese Poem Test PASSED ===\n");
	return 0;
}
