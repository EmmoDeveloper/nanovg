#include "nvg_font.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static FT_Library g_ftLibrary = NULL;

int nvgfont_init(void)
{
	if (g_ftLibrary) return 1;
	
	if (FT_Init_FreeType(&g_ftLibrary)) {
		fprintf(stderr, "Failed to initialize FreeType\n");
		return 0;
	}
	
	printf("FreeType initialized\n");
	return 1;
}

void nvgfont_shutdown(void)
{
	if (g_ftLibrary) {
		FT_Done_FreeType(g_ftLibrary);
		g_ftLibrary = NULL;
	}
}

static NVGFontAtlas* atlas_create(int width, int height)
{
	NVGFontAtlas* atlas = (NVGFontAtlas*)calloc(1, sizeof(NVGFontAtlas));
	atlas->width = width;
	atlas->height = height;
	atlas->data = (unsigned char*)calloc(width * height, 1);
	atlas->nextX = 1;  // 1px border
	atlas->nextY = 1;
	atlas->rowHeight = 0;
	return atlas;
}

static void atlas_destroy(NVGFontAtlas* atlas)
{
	if (atlas) {
		free(atlas->data);
		free(atlas);
	}
}

NVGFont* nvgfont_create(const char* path)
{
	if (!g_ftLibrary) {
		fprintf(stderr, "FreeType not initialized\n");
		return NULL;
	}
	
	NVGFont* font = (NVGFont*)calloc(1, sizeof(NVGFont));
	
	if (FT_New_Face(g_ftLibrary, path, 0, &font->face)) {
		fprintf(stderr, "Failed to load font: %s\n", path);
		free(font);
		return NULL;
	}
	
	font->atlas = atlas_create(512, 512);
	font->glyphCapacity = 256;
	font->glyphs = (NVGGlyph*)calloc(font->glyphCapacity, sizeof(NVGGlyph));
	font->size = 16.0f;
	
	// Set default size
	FT_Set_Pixel_Sizes(font->face, 0, 16);
	
	printf("Font loaded: %s\n", path);
	return font;
}

void nvgfont_destroy(NVGFont* font)
{
	if (font) {
		if (font->face) FT_Done_Face(font->face);
		if (font->atlas) atlas_destroy(font->atlas);
		free(font->glyphs);
		free(font);
	}
}

void nvgfont_set_size(NVGFont* font, float size)
{
	if (!font) return;
	font->size = size;
	FT_Set_Pixel_Sizes(font->face, 0, (FT_UInt)size);
}

NVGGlyph* nvgfont_get_glyph(NVGFont* font, uint32_t codepoint)
{
	if (!font) return NULL;
	
	// Check cache
	for (int i = 0; i < font->glyphCount; i++) {
		if (font->glyphs[i].codepoint == codepoint) {
			return &font->glyphs[i];
		}
	}
	
	// Not in cache - rasterize
	FT_UInt glyphIndex = FT_Get_Char_Index(font->face, codepoint);
	if (glyphIndex == 0) {
		printf("Glyph not found for codepoint %u\n", codepoint);
		return NULL;
	}
	
	if (FT_Load_Glyph(font->face, glyphIndex, FT_LOAD_DEFAULT)) {
		printf("Failed to load glyph %u\n", codepoint);
		return NULL;
	}
	
	if (FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_NORMAL)) {
		printf("Failed to render glyph %u\n", codepoint);
		return NULL;
	}
	
	FT_GlyphSlot slot = font->face->glyph;
	FT_Bitmap* bitmap = &slot->bitmap;
	
	// Check if glyph fits in current row
	if (font->atlas->nextX + bitmap->width + 2 > font->atlas->width) {
		// Move to next row
		font->atlas->nextX = 1;
		font->atlas->nextY += font->atlas->rowHeight + 1;
		font->atlas->rowHeight = 0;
	}
	
	// Check if atlas is full
	if (font->atlas->nextY + bitmap->rows + 2 > font->atlas->height) {
		printf("Font atlas full!\n");
		return NULL;
	}
	
	// Expand glyph cache if needed
	if (font->glyphCount >= font->glyphCapacity) {
		font->glyphCapacity *= 2;
		font->glyphs = (NVGGlyph*)realloc(font->glyphs, font->glyphCapacity * sizeof(NVGGlyph));
	}
	
	// Add to cache
	NVGGlyph* glyph = &font->glyphs[font->glyphCount++];
	glyph->codepoint = codepoint;
	glyph->x = font->atlas->nextX;
	glyph->y = font->atlas->nextY;
	glyph->width = bitmap->width;
	glyph->height = bitmap->rows;
	glyph->bearingX = slot->bitmap_left;
	glyph->bearingY = slot->bitmap_top;
	glyph->advance = slot->advance.x >> 6;  // 26.6 fixed point to pixels
	
	// Calculate texture coordinates (normalized)
	glyph->u0 = (float)glyph->x / font->atlas->width;
	glyph->v0 = (float)glyph->y / font->atlas->height;
	glyph->u1 = (float)(glyph->x + glyph->width) / font->atlas->width;
	glyph->v1 = (float)(glyph->y + glyph->height) / font->atlas->height;
	
	// Copy bitmap to atlas
	for (unsigned int y = 0; y < bitmap->rows; y++) {
		unsigned char* src = bitmap->buffer + y * bitmap->pitch;
		unsigned char* dst = font->atlas->data + (glyph->y + y) * font->atlas->width + glyph->x;
		memcpy(dst, src, bitmap->width);
	}
	
	// Update atlas position
	font->atlas->nextX += bitmap->width + 1;
	if (bitmap->rows > font->atlas->rowHeight) {
		font->atlas->rowHeight = bitmap->rows;
	}
	
	printf("Rasterized glyph '%c' (U+%04X): %dx%d at (%d,%d) bearingX=%d bearingY=%d advance=%d uv=(%.3f,%.3f)-(%.3f,%.3f)\n",
	       (codepoint >= 32 && codepoint < 127) ? (char)codepoint : '?',
	       codepoint, glyph->width, glyph->height, glyph->x, glyph->y,
	       glyph->bearingX, glyph->bearingY, glyph->advance,
	       glyph->u0, glyph->v0, glyph->u1, glyph->v1);
	
	return glyph;
}

const unsigned char* nvgfont_get_atlas_data(NVGFont* font, int* width, int* height)
{
	if (!font || !font->atlas) return NULL;
	*width = font->atlas->width;
	*height = font->atlas->height;
	return font->atlas->data;
}
