#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== Text Bounds Debug Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Bounds Debug");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "dejavu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	nvgFontFaceId(vg, font);
	nvgFontSize(vg, 48.0f);

	// Test simple text
	const char* text1 = "Hello";
	float bounds1[4];
	float width1 = nvgTextBounds(vg, 0, 0, text1, NULL, bounds1);
	printf("Text: \"%s\"\n", text1);
	printf("  Width: %.2f\n", width1);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds1[0], bounds1[1], bounds1[2], bounds1[3]);

	// Test text with potential ligatures
	const char* text2 = "office";
	float bounds2[4];
	float width2 = nvgTextBounds(vg, 0, 0, text2, NULL, bounds2);
	printf("Text: \"%s\"\n", text2);
	printf("  Width: %.2f\n", width2);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds2[0], bounds2[1], bounds2[2], bounds2[3]);

	// Disable ligatures and try again
	nvgFontFeature(vg, NVG_FEATURE_LIGA, 0);

	float bounds3[4];
	float width3 = nvgTextBounds(vg, 0, 0, text2, NULL, bounds3);
	printf("After disabling ligatures:\n");
	printf("  Width: %.2f\n", width3);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds3[0], bounds3[1], bounds3[2], bounds3[3]);

	printf("Width difference: %.2f\n", width3 - width2);

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
