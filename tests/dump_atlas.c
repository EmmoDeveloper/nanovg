#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <string.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

// Access internal font atlas data
extern const unsigned char* fonsGetTextureData(void* stash, int* width, int* height);

int main(void)
{
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Atlas Dump");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	
	int font = nvgCreateFont(vg, "sans", FONT_PATH);
	
	// Force glyph rasterization
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 48.0f);
	nvgTextBounds(vg, 0, 0, "Hello", NULL, NULL);
	
	// Get atlas data
	int atlasWidth, atlasHeight;
	const unsigned char* atlasData = fonsGetTextureData(vg->fs, &atlasWidth, &atlasHeight);
	
	printf("Atlas size: %dx%d\n", atlasWidth, atlasHeight);
	
	// Save as PGM (grayscale)
	FILE* f = fopen("atlas.pgm", "wb");
	fprintf(f, "P5\n%d %d\n255\n", atlasWidth, atlasHeight);
	fwrite(atlasData, 1, atlasWidth * atlasHeight, f);
	fclose(f);
	printf("Atlas saved to atlas.pgm\n");
	
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
