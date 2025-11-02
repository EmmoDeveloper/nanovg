#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <stdlib.h>

static void print_tag(unsigned int tag) {
	printf("'%c%c%c%c'",
		(char)((tag >> 24) & 0xFF),
		(char)((tag >> 16) & 0xFF),
		(char)((tag >> 8) & 0xFF),
		(char)(tag & 0xFF));
}

int main(void)
{
	printf("=== NanoVG OpenType Features Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "OpenType Features Test");
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

	// Load a font with ligatures (DejaVu Sans has fi, fl ligatures)
	printf("Loading DejaVu Sans font...\n");
	int font = nvgCreateFont(vg, "dejavu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	nvgFontFaceId(vg, font);
	nvgFontSize(vg, 48.0f);

	printf("\n=== Testing Standard Ligatures (liga) ===\n\n");

	// Test text with ligatures: "office" contains "ffi" ligature, "fluffy" contains "ff" and "fl"
	const char* test_text = "office fluffyffle";

	printf("Test text: \"%s\"\n\n", test_text);

	// Measure with ligatures enabled (default)
	printf("With ligatures enabled (default):\n");
	float bounds1[4];
	float width1 = nvgTextBounds(vg, 0, 0, test_text, NULL, bounds1);
	printf("  Width: %.2f pixels\n", width1);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n",
		bounds1[0], bounds1[1], bounds1[2], bounds1[3]);

	// Disable standard ligatures
	printf("\nDisabling standard ligatures (liga):\n");
	nvgFontFeature(vg, NVG_FEATURE_LIGA, 0);

	float bounds2[4];
	float width2 = nvgTextBounds(vg, 0, 0, test_text, NULL, bounds2);
	printf("  Width: %.2f pixels\n", width2);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n",
		bounds2[0], bounds2[1], bounds2[2], bounds2[3]);

	if (width1 != width2) {
		printf("\n  Success! Width changed from %.2f to %.2f (diff: %.2f)\n",
			width1, width2, width2 - width1);
		printf("  This confirms ligatures were toggled.\n");
	} else {
		printf("\n  Warning: Width unchanged. Font may not support ligatures,\n");
		printf("  or ligatures don't affect these specific character combinations.\n");
	}

	// Reset features
	printf("\n=== Testing Feature Reset ===\n\n");
	nvgFontFeaturesReset(vg);
	printf("Features reset to defaults.\n");

	float bounds3[4];
	float width3 = nvgTextBounds(vg, 0, 0, test_text, NULL, bounds3);
	printf("  Width after reset: %.2f pixels\n", width3);

	if (width3 == width1) {
		printf("  Success! Width matches original (ligatures re-enabled).\n");
	} else {
		printf("  Warning: Width doesn't match original.\n");
	}

	printf("\n=== Testing Multiple Features ===\n\n");

	// Try contextual alternates and contextual ligatures
	printf("Enabling contextual alternates (calt) and contextual ligatures (clig):\n");
	nvgFontFeature(vg, NVG_FEATURE_CALT, 1);
	nvgFontFeature(vg, NVG_FEATURE_CLIG, 1);
	printf("  Features set (effects depend on font support).\n");

	// Test with numbers for tabular/oldstyle
	const char* number_text = "1234567890";
	printf("\nTesting tabular numbers with: \"%s\"\n", number_text);

	float num_width1 = nvgTextBounds(vg, 0, 0, number_text, NULL, NULL);
	printf("  Default width: %.2f pixels\n", num_width1);

	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_TNUM, 1);
	float num_width2 = nvgTextBounds(vg, 0, 0, number_text, NULL, NULL);
	printf("  Tabular numbers (tnum) width: %.2f pixels\n", num_width2);

	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_ONUM, 1);
	float num_width3 = nvgTextBounds(vg, 0, 0, number_text, NULL, NULL);
	printf("  Oldstyle numbers (onum) width: %.2f pixels\n", num_width3);

	printf("\n=== Testing Feature Tags ===\n\n");

	// Display some predefined feature tags
	printf("Predefined feature tag constants:\n");
	printf("  NVG_FEATURE_LIGA = "); print_tag(NVG_FEATURE_LIGA); printf("\n");
	printf("  NVG_FEATURE_DLIG = "); print_tag(NVG_FEATURE_DLIG); printf("\n");
	printf("  NVG_FEATURE_SMCP = "); print_tag(NVG_FEATURE_SMCP); printf("\n");
	printf("  NVG_FEATURE_SWSH = "); print_tag(NVG_FEATURE_SWSH); printf("\n");
	printf("  NVG_FEATURE_ZERO = "); print_tag(NVG_FEATURE_ZERO); printf("\n");
	printf("  NVG_FEATURE_FRAC = "); print_tag(NVG_FEATURE_FRAC); printf("\n");
	printf("  NVG_FEATURE_SS01 = "); print_tag(NVG_FEATURE_SS01); printf("\n");

	printf("\n=== Test Complete ===\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
