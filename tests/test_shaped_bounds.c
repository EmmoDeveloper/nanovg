#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== Shaped Bounds Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Shaped Bounds Test");
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

	const char* text = "office fluffy";
	printf("Test text: \"%s\"\n\n", text);

	printf("=== Comparing nvgTextBounds vs nvgTextBoundsWithShaping ===\n\n");

	// Simple bounds (no shaping)
	float bounds1[4];
	float width1 = nvgTextBounds(vg, 0, 0, text, NULL, bounds1);
	printf("nvgTextBounds (simple):\n");
	printf("  Width: %.2f pixels\n", width1);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds1[0], bounds1[1], bounds1[2], bounds1[3]);

	// Shaped bounds (with HarfBuzz)
	float bounds2[4];
	float width2 = nvgTextBoundsWithShaping(vg, 0, 0, text, NULL, bounds2);
	printf("nvgTextBoundsWithShaping (with HarfBuzz):\n");
	printf("  Width: %.2f pixels\n", width2);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds2[0], bounds2[1], bounds2[2], bounds2[3]);

	if (width1 != width2) {
		printf("  Difference in width: %.2f pixels\n", width2 - width1);
		printf("  This shows shaped bounds differ from simple bounds.\n\n");
	} else {
		printf("  Widths are identical.\n\n");
	}

	printf("=== Testing Feature Impact on Shaped Bounds ===\n\n");

	// Disable ligatures
	nvgFontFeature(vg, NVG_FEATURE_LIGA, 0);

	float bounds3[4];
	float width3 = nvgTextBoundsWithShaping(vg, 0, 0, text, NULL, bounds3);
	printf("With ligatures disabled (liga=0):\n");
	printf("  Width: %.2f pixels\n", width3);
	printf("  Bounds: [%.2f, %.2f, %.2f, %.2f]\n\n",
		bounds3[0], bounds3[1], bounds3[2], bounds3[3]);

	if (width2 != width3) {
		printf("  Success! Ligature toggle affects shaped bounds.\n");
		printf("  Width changed from %.2f to %.2f (diff: %.2f)\n\n",
			width2, width3, width3 - width2);
	} else {
		printf("  Note: Width unchanged (font may not have ligatures for this text).\n\n");
	}

	// Reset features
	nvgFontFeaturesReset(vg);

	float bounds4[4];
	float width4 = nvgTextBoundsWithShaping(vg, 0, 0, text, NULL, bounds4);
	printf("After feature reset:\n");
	printf("  Width: %.2f pixels\n", width4);

	if (width4 == width2) {
		printf("  Success! Width matches original shaped bounds.\n");
	} else {
		printf("  Width differs from original.\n");
	}

	printf("\n=== Test Complete ===\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	return 0;
}
