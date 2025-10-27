#include "../src/nvg_vk.h"
#include "../src/nvg_font.h"
#include "window_utils.h"
#include <stdio.h>
#include <string.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

int main(void)
{
	printf("=== Custom Font System Test ===\n\n");

	// Initialize FreeType
	if (!nvgfont_init()) {
		fprintf(stderr, "Failed to initialize font system\n");
		return 1;
	}

	// Load font
	NVGFont* font = nvgfont_create(FONT_PATH);
	if (!font) {
		nvgfont_shutdown();
		return 1;
	}

	// Set size and rasterize test string
	nvgfont_set_size(font, 48.0f);
	
	const char* text = "Hello!";
	printf("\nRasterizing '%s'...\n", text);
	
	for (const char* p = text; *p; p++) {
		NVGGlyph* glyph = nvgfont_get_glyph(font, *p);
		if (!glyph) {
			printf("Failed to get glyph for '%c'\n", *p);
		}
	}

	// Save atlas to file
	int atlasWidth, atlasHeight;
	const unsigned char* atlasData = nvgfont_get_atlas_data(font, &atlasWidth, &atlasHeight);
	
	FILE* f = fopen("custom_atlas.pgm", "wb");
	fprintf(f, "P5\n%d %d\n255\n", atlasWidth, atlasHeight);
	fwrite(atlasData, 1, atlasWidth * atlasHeight, f);
	fclose(f);
	printf("\nAtlas saved to custom_atlas.pgm (%dx%d)\n", atlasWidth, atlasHeight);

	// Cleanup
	nvgfont_destroy(font);
	nvgfont_shutdown();

	printf("\n=== Test PASSED ===\n");
	return 0;
}
