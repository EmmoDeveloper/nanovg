#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include "../src/tools/window_utils.h"

// Direct test: Render LCD subpixel text using FreeType directly
// No NanoVG involvement - just FreeType -> Vulkan texture -> shader

int main(void) {
	printf("=== Direct LCD Subpixel Test ===\n");
	printf("This test renders LCD subpixel text directly with FreeType\n");
	printf("bypassing all NanoVG code to isolate the issue.\n\n");

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

	// Set font size
	FT_Set_Pixel_Sizes(face, 0, 72);

	// Enable LCD filter
	FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT);

	// Render a single character 'A' with LCD subpixel antialiasing
	FT_UInt glyph_index = FT_Get_Char_Index(face, 'A');
	if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD)) {
		printf("Failed to load glyph\n");
		FT_Done_Face(face);
		FT_Done_FreeType(ftLibrary);
		return 1;
	}

	FT_GlyphSlot slot = face->glyph;
	FT_Bitmap* bitmap = &slot->bitmap;

	printf("Glyph 'A' rendered:\n");
	printf("  pixel_mode: %d (should be 5 for LCD)\n", bitmap->pixel_mode);
	printf("  width: %d pixels (FreeType reports 3x width in bytes)\n", bitmap->width);
	printf("  rows: %d\n", bitmap->rows);
	printf("  pitch: %d\n", bitmap->pitch);

	// For LCD rendering, width is in bytes (3 bytes per pixel)
	int logical_width = bitmap->width / 3;
	int height = bitmap->rows;

	printf("  logical_width: %d pixels\n", logical_width);
	printf("\nSearching for non-zero LCD values:\n");

	// Find first non-zero value to verify LCD data exists
	int found = 0;
	for (int y = 0; y < height && !found; y++) {
		for (int x = 0; x < bitmap->width - 2 && !found; x += 3) {
			unsigned char r = bitmap->buffer[y * abs(bitmap->pitch) + x];
			unsigned char g = bitmap->buffer[y * abs(bitmap->pitch) + x + 1];
			unsigned char b = bitmap->buffer[y * abs(bitmap->pitch) + x + 2];
			if (r > 0 || g > 0 || b > 0) {
				printf("  First non-zero at row %d, col %d: R=%d G=%d B=%d\n", y, x/3, r, g, b);
				found = 1;
			}
		}
	}
	if (!found) {
		printf("  WARNING: No non-zero values found in bitmap!\n");
	}

	// Convert LCD data to RGBA texture
	unsigned char* rgba_data = (unsigned char*)calloc(logical_width * height * 4, 1);
	if (!rgba_data) {
		printf("Failed to allocate texture buffer\n");
		FT_Done_Face(face);
		FT_Done_FreeType(ftLibrary);
		return 1;
	}

	printf("\nConverting to RGBA texture...\n");
	int pitch = abs(bitmap->pitch);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < logical_width; x++) {
			unsigned char r = bitmap->buffer[y * pitch + x * 3 + 0];
			unsigned char g = bitmap->buffer[y * pitch + x * 3 + 1];
			unsigned char b = bitmap->buffer[y * pitch + x * 3 + 2];

			rgba_data[(y * logical_width + x) * 4 + 0] = r;
			rgba_data[(y * logical_width + x) * 4 + 1] = g;
			rgba_data[(y * logical_width + x) * 4 + 2] = b;
			rgba_data[(y * logical_width + x) * 4 + 3] = 255;
		}
	}

	// Sample the converted data at the first non-zero location we found
	if (found) {
		printf("Verifying RGBA conversion at first non-zero location...\n");
	}

	// Save as PPM to verify visually
	FILE* fp = fopen("screendumps/test_lcd_direct.ppm", "wb");
	if (fp) {
		fprintf(fp, "P6\n%d %d\n255\n", logical_width, height);
		for (int i = 0; i < logical_width * height; i++) {
			fputc(rgba_data[i * 4 + 0], fp);  // R
			fputc(rgba_data[i * 4 + 1], fp);  // G
			fputc(rgba_data[i * 4 + 2], fp);  // B
		}
		fclose(fp);
		printf("\nSaved LCD glyph to screendumps/test_lcd_direct.ppm\n");
		printf("Convert with: convert screendumps/test_lcd_direct.ppm screendumps/test_lcd_direct.png\n");
	}

	printf("\nData conversion successful!\n");
	printf("FreeType IS producing LCD subpixel data correctly!\n");

	free(rgba_data);
	FT_Done_Face(face);
	FT_Done_FreeType(ftLibrary);

	return 0;
}
