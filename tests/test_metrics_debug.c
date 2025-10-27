#include "../src/nvg_freetype.h"
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int main(void) {
	printf("=== FreeType Metrics Debug ===\n\n");

	// Direct FreeType test
	FT_Library library;
	FT_Face face;

	if (FT_Init_FreeType(&library)) {
		fprintf(stderr, "Failed to init FreeType\n");
		return 1;
	}

	if (FT_New_Face(library, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 0, &face)) {
		fprintf(stderr, "Failed to load font\n");
		FT_Done_FreeType(library);
		return 1;
	}

	printf("Font loaded directly:\n");
	printf("  units_per_EM: %d\n", face->units_per_EM);
	printf("  ascender: %ld\n", face->ascender);
	printf("  descender: %ld\n", face->descender);
	printf("  height: %ld\n", face->height);
	printf("  face->size before set: %p\n", (void*)face->size);

	if (FT_Set_Pixel_Sizes(face, 0, 24)) {
		fprintf(stderr, "Failed to set pixel sizes\n");
	} else {
		printf("\nAfter FT_Set_Pixel_Sizes(24):\n");
		printf("  face->size: %p\n", (void*)face->size);
		if (face->size) {
			printf("  ascender: %ld (%.1f px)\n",
				face->size->metrics.ascender,
				face->size->metrics.ascender / 64.0f);
			printf("  descender: %ld (%.1f px)\n",
				face->size->metrics.descender,
				face->size->metrics.descender / 64.0f);
			printf("  height: %ld (%.1f px)\n",
				face->size->metrics.height,
				face->size->metrics.height / 64.0f);
		}
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	return 0;
}
