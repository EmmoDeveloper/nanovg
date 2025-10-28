#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

int main() {
	FT_Library library;
	FT_Init_FreeType(&library);
	
	FT_Face face;
	if (FT_New_Face(library, "fonts/emoji/Noto-COLRv1.ttf", 0, &face) != 0) {
		printf("Failed to load font\n");
		return 1;
	}
	
	printf("Loaded font: %s %s\n", face->family_name, face->style_name);
	FT_Set_Pixel_Sizes(face, 0, 128);
	
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 128, 128);
	cairo_t* cr = cairo_create(surface);
	
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_paint(cr);
	cairo_translate(cr, 0, 102);
	
	cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(face, 0);
	cairo_font_options_t* font_options = cairo_font_options_create();
	cairo_font_options_set_color_mode(font_options, CAIRO_COLOR_MODE_COLOR);
	
	cairo_matrix_t font_matrix, ctm;
	cairo_matrix_init_scale(&font_matrix, 128, 128);
	cairo_matrix_init_identity(&ctm);
	
	cairo_scaled_font_t* scaled_font = cairo_scaled_font_create(font_face, &font_matrix, &ctm, font_options);
	cairo_set_scaled_font(cr, scaled_font);
	
	cairo_glyph_t glyph;
	glyph.index = 1875;  // U+1F600 grinning face emoji
	glyph.x = 0;
	glyph.y = 0;
	
	printf("Rendering glyph %u (U+1F600 grinning face emoji)...\n", glyph.index);
	cairo_show_glyphs(cr, &glyph, 1);
	
	cairo_status_t status = cairo_status(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		printf("Cairo error: %s\n", cairo_status_to_string(status));
		return 1;
	}
	
	cairo_surface_write_to_png(surface, "test_color_emoji_output.png");
	printf("SUCCESS: Rendered color emoji to test_color_emoji_output.png\n");
	
	cairo_scaled_font_destroy(scaled_font);
	cairo_font_options_destroy(font_options);
	cairo_font_face_destroy(font_face);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	
	return 0;
}
