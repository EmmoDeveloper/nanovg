#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Font Information Query Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Font Info Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	// Test with multiple fonts
	const char* font_paths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"
	};
	const char* font_names[] = {"sans", "mono", "serif"};
	int fonts[3];

	for (int i = 0; i < 3; i++) {
		fonts[i] = nvgCreateFont(vg, font_names[i], font_paths[i]);
		if (fonts[i] == -1) {
			fprintf(stderr, "Failed to load font: %s\n", font_paths[i]);
			nvgDeleteVk(vg);
			window_destroy_context(winCtx);
			return 1;
		}
		printf("Loaded font %d: %s\n", i, font_names[i]);
	}

	printf("\n=== Testing Font Information Queries ===\n\n");

	for (int i = 0; i < 3; i++) {
		nvgFontFaceId(vg, fonts[i]);

		printf("Font %d (%s):\n", i, font_names[i]);

		const char* family = nvgFontFamilyName(vg);
		printf("  Family name:   %s\n", family ? family : "(null)");

		const char* style = nvgFontStyleName(vg);
		printf("  Style name:    %s\n", style ? style : "(null)");

		int glyph_count = nvgFontGlyphCount(vg);
		printf("  Glyph count:   %d\n", glyph_count);

		int is_scalable = nvgFontIsScalable(vg);
		printf("  Is scalable:   %s\n", is_scalable ? "yes" : "no");

		int is_fixed = nvgFontIsFixedWidth(vg);
		printf("  Is fixed-width: %s\n", is_fixed ? "yes" : "no");

		printf("\n");
	}

	// Test with invalid font (should handle gracefully)
	printf("Testing with invalid font ID (-1):\n");
	nvgFontFaceId(vg, -1);
	const char* invalid_family = nvgFontFamilyName(vg);
	printf("  Family name:   %s\n", invalid_family ? invalid_family : "(null)");
	printf("\n");

	printf("=== Test Complete ===\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
