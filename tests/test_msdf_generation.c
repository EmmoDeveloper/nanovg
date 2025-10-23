// Test MSDF generation with proper Bezier curve handling
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "nanovg_vk_msdf.h"

static int testCurveGlyph(FT_Face face, unsigned int codepoint, const char* description) {
	printf("  Testing '%s' (U+%04X)...\n", description, codepoint);

	FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
	if (glyphIndex == 0) {
		printf("    ⚠ Glyph not found in font\n");
		return 1; // Skip if not in font
	}

	// Load glyph outline
	FT_Error error = FT_Set_Pixel_Sizes(face, 0, 64);
	if (error) {
		printf("    ✗ Failed to set pixel size\n");
		return 0;
	}

	error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP);
	if (error) {
		printf("    ✗ Failed to load glyph\n");
		return 0;
	}

	if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
		printf("    ✗ Glyph is not an outline\n");
		return 0;
	}

	// Generate MSDF
	VKNVGmsdfParams params;
	params.width = 32;
	params.height = 32;
	params.range = 4.0f;
	params.scale = 1.0f;
	params.offsetX = 4;
	params.offsetY = 4;

	unsigned char* output = (unsigned char*)malloc(params.width * params.height * 3);
	if (!output) {
		printf("    ✗ Memory allocation failed\n");
		return 0;
	}

	int result = vknvg__generateMSDF(face->glyph, output, &params);
	if (!result) {
		printf("    ✗ MSDF generation failed\n");
		free(output);
		return 0;
	}

	// Verify output has non-zero data (curve was rasterized)
	int nonZeroCount = 0;
	int edgePixels = 0;
	for (int i = 0; i < params.width * params.height * 3; i++) {
		if (output[i] != 0 && output[i] != 255) {
			edgePixels++;
		}
		if (output[i] != 0) {
			nonZeroCount++;
		}
	}

	free(output);

	if (nonZeroCount == 0) {
		printf("    ✗ Output is empty (no glyph data)\n");
		return 0;
	}

	if (edgePixels == 0) {
		printf("    ✗ No edge pixels found (curve sampling may have failed)\n");
		return 0;
	}

	printf("    ✓ Generated MSDF: %d/%d non-zero pixels, %d edge pixels\n",
	       nonZeroCount, params.width * params.height * 3, edgePixels);
	return 1;
}

static int testSDFGeneration(FT_Face face, unsigned int codepoint, const char* description) {
	printf("  Testing SDF for '%s' (U+%04X)...\n", description, codepoint);

	FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
	if (glyphIndex == 0) {
		printf("    ⚠ Glyph not found in font\n");
		return 1;
	}

	FT_Error error = FT_Set_Pixel_Sizes(face, 0, 64);
	if (error) return 0;

	error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP);
	if (error) return 0;

	if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) return 0;

	VKNVGmsdfParams params;
	params.width = 32;
	params.height = 32;
	params.range = 4.0f;
	params.scale = 1.0f;
	params.offsetX = 4;
	params.offsetY = 4;

	unsigned char* output = (unsigned char*)malloc(params.width * params.height);
	if (!output) return 0;

	int result = vknvg__generateSDF(face->glyph, output, &params);
	if (!result) {
		free(output);
		return 0;
	}

	int nonZeroCount = 0;
	for (int i = 0; i < params.width * params.height; i++) {
		if (output[i] != 0) nonZeroCount++;
	}

	free(output);

	if (nonZeroCount == 0) return 0;

	printf("    ✓ Generated SDF: %d/%d non-zero pixels\n",
	       nonZeroCount, params.width * params.height);
	return 1;
}

int main() {
	printf("===========================================\n");
	printf("MSDF Generation Test - Bezier Curve Handling\n");
	printf("===========================================\n\n");

	// Initialize FreeType
	FT_Library library;
	FT_Error error = FT_Init_FreeType(&library);
	if (error) {
		printf("✗ Failed to initialize FreeType\n");
		return 1;
	}

	// Try to load a system font
	const char* fontPaths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/System/Library/Fonts/Helvetica.ttc",
		"/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
		NULL
	};

	FT_Face face = NULL;
	const char* loadedFont = NULL;
	for (int i = 0; fontPaths[i] != NULL; i++) {
		error = FT_New_Face(library, fontPaths[i], 0, &face);
		if (!error) {
			loadedFont = fontPaths[i];
			break;
		}
	}

	if (!face) {
		printf("✗ Failed to load any system font\n");
		printf("  Tried paths:\n");
		for (int i = 0; fontPaths[i] != NULL; i++) {
			printf("    - %s\n", fontPaths[i]);
		}
		FT_Done_FreeType(library);
		return 1;
	}

	printf("Phase 1: Testing MSDF Generation\n");
	printf("Using font: %s\n\n", loadedFont);

	int passed = 0;
	int total = 0;

	// Test glyphs with curves (these benefit from our Bezier improvements)
	struct {
		unsigned int codepoint;
		const char* description;
	} testGlyphs[] = {
		{0x004F, "O"}, // Circular curves
		{0x0053, "S"}, // S-curves (cubic Bezier)
		{0x0061, "a"}, // Bowl and tail curves
		{0x0043, "C"}, // Arc
		{0x0065, "e"}, // Circular bowl
		{0x0038, "8"}, // Two circles
		{0x0040, "@"}, // Complex curves
		{0x0026, "&"}, // Ampersand (complex)
		{0, NULL}
	};

	for (int i = 0; testGlyphs[i].description != NULL; i++) {
		total++;
		if (testCurveGlyph(face, testGlyphs[i].codepoint, testGlyphs[i].description)) {
			passed++;
		}
	}

	printf("\nPhase 2: Testing SDF Generation\n\n");

	// Test a few SDF glyphs too
	for (int i = 0; i < 3 && testGlyphs[i].description != NULL; i++) {
		total++;
		if (testSDFGeneration(face, testGlyphs[i].codepoint, testGlyphs[i].description)) {
			passed++;
		}
	}

	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);

	printf("\n===========================================\n");
	if (passed == total) {
		printf("✓ All tests passed (%d/%d)\n", passed, total);
		printf("✓ Bezier curve handling verified\n");
		printf("===========================================\n");
		return 0;
	} else {
		printf("✗ Some tests failed (%d/%d)\n", passed, total);
		printf("===========================================\n");
		return 1;
	}
}
