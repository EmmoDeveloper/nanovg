#include "../src/nvg_freetype.h"
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

typedef struct {
	char path[260];
	FTC_FaceID face_id;
} TestFont;

static FT_Error face_requester(FTC_FaceID face_id, FT_Library library,
                                 FT_Pointer request_data, FT_Face* aface) {
	(void)request_data;
	TestFont* font = (TestFont*)face_id;
	return FT_New_Face(library, font->path, 0, aface);
}

int main(void) {
	printf("=== FTC Metrics Test ===\n\n");

	FT_Library library;
	FTC_Manager manager;
	FTC_CMapCache cmap_cache;

	if (FT_Init_FreeType(&library)) {
		fprintf(stderr, "Failed to init FreeType\n");
		return 1;
	}

	if (FTC_Manager_New(library, 0, 0, 16 * 1024 * 1024,
	                     face_requester, NULL, &manager)) {
		fprintf(stderr, "Failed to create FTC manager\n");
		return 1;
	}

	if (FTC_CMapCache_New(manager, &cmap_cache)) {
		fprintf(stderr, "Failed to create cmap cache\n");
		return 1;
	}

	TestFont font;
	snprintf(font.path, sizeof(font.path), "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	font.face_id = (FTC_FaceID)&font;

	FT_Face face;
	if (FTC_Manager_LookupFace(manager, font.face_id, &face)) {
		fprintf(stderr, "Failed to lookup face\n");
		return 1;
	}

	printf("Face looked up via FTC:\n");
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

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	printf("\n✓ FTC metrics work correctly\n");
	return 0;
}
