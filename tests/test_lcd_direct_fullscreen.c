#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include "../src/tools/window_utils.h"

// Independent test: Render the same screen as test_subpixel_rendering.c
// but using ONLY FreeType + Vulkan, NO NanoVG API

typedef struct {
	unsigned char* rgba_data;
	int width;
	int height;
	int advance_x;
	int bearing_x;
	int bearing_y;
} RenderedGlyph;

void render_glyph(FT_Face face, char c, int subpixel_mode, RenderedGlyph* out) {
	FT_UInt glyph_index = FT_Get_Char_Index(face, c);

	FT_Int32 load_flags = FT_LOAD_RENDER;
	if (subpixel_mode == 1 || subpixel_mode == 2) {
		// RGB or BGR
		load_flags |= FT_LOAD_TARGET_LCD;
	}

	if (FT_Load_Glyph(face, glyph_index, load_flags)) {
		out->rgba_data = NULL;
		return;
	}

	FT_GlyphSlot slot = face->glyph;
	FT_Bitmap* bitmap = &slot->bitmap;

	int width, height;
	if (subpixel_mode == 1 || subpixel_mode == 2) {
		// LCD: width is 3x
		width = bitmap->width / 3;
		height = bitmap->rows;
	} else {
		// Grayscale
		width = bitmap->width;
		height = bitmap->rows;
	}

	out->width = width;
	out->height = height;
	out->advance_x = slot->advance.x >> 6;
	out->bearing_x = slot->bitmap_left;
	out->bearing_y = slot->bitmap_top;

	out->rgba_data = (unsigned char*)calloc(width * height * 4, 1);
	if (!out->rgba_data) return;

	int pitch = abs(bitmap->pitch);

	if (subpixel_mode == 1 || subpixel_mode == 2) {
		// LCD subpixel
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				unsigned char r = bitmap->buffer[y * pitch + x * 3 + 0];
				unsigned char g = bitmap->buffer[y * pitch + x * 3 + 1];
				unsigned char b = bitmap->buffer[y * pitch + x * 3 + 2];

				// Handle BGR vs RGB
				if (subpixel_mode == 2) {
					unsigned char tmp = r;
					r = b;
					b = tmp;
				}

				out->rgba_data[(y * width + x) * 4 + 0] = r;
				out->rgba_data[(y * width + x) * 4 + 1] = g;
				out->rgba_data[(y * width + x) * 4 + 2] = b;
				out->rgba_data[(y * width + x) * 4 + 3] = 255;
			}
		}
	} else {
		// Grayscale
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				unsigned char gray = bitmap->buffer[y * pitch + x];
				out->rgba_data[(y * width + x) * 4 + 0] = gray;
				out->rgba_data[(y * width + x) * 4 + 1] = gray;
				out->rgba_data[(y * width + x) * 4 + 2] = gray;
				out->rgba_data[(y * width + x) * 4 + 3] = 255;
			}
		}
	}
}

void render_text_to_image(FT_Face face, const char* text, int x, int y,
                          int subpixel_mode, unsigned char* image, int img_width, int img_height,
                          unsigned char text_r, unsigned char text_g, unsigned char text_b) {
	int pen_x = x;
	int pen_y = y;

	for (const char* p = text; *p; p++) {
		RenderedGlyph glyph;
		render_glyph(face, *p, subpixel_mode, &glyph);

		if (!glyph.rgba_data) {
			pen_x += glyph.advance_x;
			continue;
		}

		// Blit glyph to image
		int glyph_x = pen_x + glyph.bearing_x;
		int glyph_y = pen_y - glyph.bearing_y;

		for (int gy = 0; gy < glyph.height; gy++) {
			for (int gx = 0; gx < glyph.width; gx++) {
				int img_x = glyph_x + gx;
				int img_y = glyph_y + gy;

				if (img_x >= 0 && img_x < img_width && img_y >= 0 && img_y < img_height) {
					int g_idx = (gy * glyph.width + gx) * 4;
					int i_idx = (img_y * img_width + img_x) * 3;

					if (subpixel_mode == 1 || subpixel_mode == 2) {
						// LCD: use RGB coverage directly
						unsigned char r_cov = glyph.rgba_data[g_idx + 0];
						unsigned char g_cov = glyph.rgba_data[g_idx + 1];
						unsigned char b_cov = glyph.rgba_data[g_idx + 2];

						// Blend with background (simple alpha blend using average coverage)
						float alpha = (r_cov + g_cov + b_cov) / (3.0f * 255.0f);
						image[i_idx + 0] = (unsigned char)(r_cov * text_r / 255.0f * alpha + image[i_idx + 0] * (1.0f - alpha));
						image[i_idx + 1] = (unsigned char)(g_cov * text_g / 255.0f * alpha + image[i_idx + 1] * (1.0f - alpha));
						image[i_idx + 2] = (unsigned char)(b_cov * text_b / 255.0f * alpha + image[i_idx + 2] * (1.0f - alpha));
					} else {
						// Grayscale
						unsigned char alpha = glyph.rgba_data[g_idx + 0];
						float a = alpha / 255.0f;
						image[i_idx + 0] = (unsigned char)(text_r * a + image[i_idx + 0] * (1.0f - a));
						image[i_idx + 1] = (unsigned char)(text_g * a + image[i_idx + 1] * (1.0f - a));
						image[i_idx + 2] = (unsigned char)(text_b * a + image[i_idx + 2] * (1.0f - a));
					}
				}
			}
		}

		pen_x += glyph.advance_x;
		free(glyph.rgba_data);
	}
}

int main(void) {
	printf("=== Direct LCD Fullscreen Test with 4 Canvases ===\n");
	printf("Canvas 1: End-user display (final output)\n");
	printf("Canvas 2: LCD-ALPHA (grayscale, default)\n");
	printf("Canvas 3: LCD-RGB-SUBPIXEL\n");
	printf("Canvas 4: LCD-BGR-SUBPIXEL\n\n");

	// Initialize FreeType
	FT_Library ftLibrary;
	if (FT_Init_FreeType(&ftLibrary)) {
		printf("Failed to initialize FreeType\n");
		return 1;
	}

	// Load font
	FT_Face face;
	if (FT_New_Face(ftLibrary, "fonts/sans/NotoSans-Regular.ttf", 0, &face)) {
		printf("Failed to load font\n");
		FT_Done_FreeType(ftLibrary);
		return 1;
	}

	// Enable LCD filter
	FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT);

	int width = 1400;
	int height = 900;

	// Canvas 1: End-user display (final output)
	unsigned char* canvas_display = (unsigned char*)calloc(width * height * 3, 1);
	if (!canvas_display) {
		printf("Failed to allocate display canvas\n");
		FT_Done_Face(face);
		FT_Done_FreeType(ftLibrary);
		return 1;
	}

	// Canvas 2: LCD-ALPHA (grayscale text rendering)
	unsigned char* canvas_alpha = (unsigned char*)calloc(width * height * 3, 1);

	// Canvas 3: LCD-RGB-SUBPIXEL
	unsigned char* canvas_rgb = (unsigned char*)calloc(width * height * 3, 1);

	// Canvas 4: LCD-BGR-SUBPIXEL
	unsigned char* canvas_bgr = (unsigned char*)calloc(width * height * 3, 1);

	if (!canvas_alpha || !canvas_rgb || !canvas_bgr) {
		printf("Failed to allocate canvases\n");
		free(canvas_display);
		free(canvas_alpha);
		free(canvas_rgb);
		free(canvas_bgr);
		FT_Done_Face(face);
		FT_Done_FreeType(ftLibrary);
		return 1;
	}

	// Fill all canvases with light gray background (250, 250, 250)
	for (int i = 0; i < width * height * 3; i++) {
		canvas_display[i] = 250;
		canvas_alpha[i] = 250;
		canvas_rgb[i] = 250;
		canvas_bgr[i] = 250;
	}

	printf("Allocated 4 canvases: %dx%d each\n\n", width, height);

	const char* testText = "The quick brown fox jumps over the lazy dog";
	const char* testText2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789";

	// Render to Canvas 2: LCD-ALPHA (grayscale) - Section 1
	printf("Rendering to Canvas 2 (LCD-ALPHA): Section 1...\n");
	FT_Set_Pixel_Sizes(face, 0, 48);
	render_text_to_image(face, "1. GRAYSCALE (No Subpixel)", 50, 100, 0, canvas_alpha, width, height, 0, 0, 0);
	FT_Set_Pixel_Sizes(face, 0, 72);
	render_text_to_image(face, testText, 50, 180, 0, canvas_alpha, width, height, 40, 40, 40);
	render_text_to_image(face, testText2, 50, 240, 0, canvas_alpha, width, height, 40, 40, 40);

	// Render to Canvas 3: LCD-RGB-SUBPIXEL - Section 2
	printf("Rendering to Canvas 3 (LCD-RGB-SUBPIXEL): Section 2...\n");
	FT_Set_Pixel_Sizes(face, 0, 48);
	render_text_to_image(face, "2. RGB SUBPIXEL", 50, 380, 1, canvas_rgb, width, height, 0, 0, 0);
	FT_Set_Pixel_Sizes(face, 0, 72);
	render_text_to_image(face, testText, 50, 460, 1, canvas_rgb, width, height, 40, 40, 40);
	render_text_to_image(face, testText2, 50, 520, 1, canvas_rgb, width, height, 40, 40, 40);

	// Render to Canvas 4: LCD-BGR-SUBPIXEL - Section 3
	printf("Rendering to Canvas 4 (LCD-BGR-SUBPIXEL): Section 3...\n");
	FT_Set_Pixel_Sizes(face, 0, 48);
	render_text_to_image(face, "3. BGR SUBPIXEL", 50, 660, 2, canvas_bgr, width, height, 0, 0, 0);
	FT_Set_Pixel_Sizes(face, 0, 72);
	render_text_to_image(face, testText, 50, 740, 2, canvas_bgr, width, height, 40, 40, 40);
	render_text_to_image(face, testText2, 50, 800, 2, canvas_bgr, width, height, 40, 40, 40);

	// Composite all canvases to Canvas 1 (display)
	printf("\nCompositing canvases to display...\n");
	// For now, simple copy - section 1 from alpha, section 2 from rgb, section 3 from bgr
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			if (y < 300) {
				// Section 1: use canvas_alpha
				canvas_display[idx + 0] = canvas_alpha[idx + 0];
				canvas_display[idx + 1] = canvas_alpha[idx + 1];
				canvas_display[idx + 2] = canvas_alpha[idx + 2];
			} else if (y < 600) {
				// Section 2: use canvas_rgb
				canvas_display[idx + 0] = canvas_rgb[idx + 0];
				canvas_display[idx + 1] = canvas_rgb[idx + 1];
				canvas_display[idx + 2] = canvas_rgb[idx + 2];
			} else {
				// Section 3: use canvas_bgr
				canvas_display[idx + 0] = canvas_bgr[idx + 0];
				canvas_display[idx + 1] = canvas_bgr[idx + 1];
				canvas_display[idx + 2] = canvas_bgr[idx + 2];
			}
		}
	}

	// Save final display
	FILE* fp = fopen("screendumps/test_lcd_direct_fullscreen.ppm", "wb");
	if (fp) {
		fprintf(fp, "P6\n%d %d\n255\n", width, height);
		fwrite(canvas_display, 1, width * height * 3, fp);
		fclose(fp);
		printf("\nSaved Canvas 1 (display) to screendumps/test_lcd_direct_fullscreen.ppm\n");
	}

	// Save individual canvases for inspection
	fp = fopen("screendumps/canvas_alpha.ppm", "wb");
	if (fp) {
		fprintf(fp, "P6\n%d %d\n255\n", width, height);
		fwrite(canvas_alpha, 1, width * height * 3, fp);
		fclose(fp);
		printf("Saved Canvas 2 (LCD-ALPHA) to screendumps/canvas_alpha.ppm\n");
	}

	fp = fopen("screendumps/canvas_rgb.ppm", "wb");
	if (fp) {
		fprintf(fp, "P6\n%d %d\n255\n", width, height);
		fwrite(canvas_rgb, 1, width * height * 3, fp);
		fclose(fp);
		printf("Saved Canvas 3 (LCD-RGB) to screendumps/canvas_rgb.ppm\n");
	}

	fp = fopen("screendumps/canvas_bgr.ppm", "wb");
	if (fp) {
		fprintf(fp, "P6\n%d %d\n255\n", width, height);
		fwrite(canvas_bgr, 1, width * height * 3, fp);
		fclose(fp);
		printf("Saved Canvas 4 (LCD-BGR) to screendumps/canvas_bgr.ppm\n");
	}

	free(canvas_display);
	free(canvas_alpha);
	free(canvas_rgb);
	free(canvas_bgr);
	FT_Done_Face(face);
	FT_Done_FreeType(ftLibrary);

	printf("\nDone! 4 canvases rendered and composited.\n");
	return 0;
}
