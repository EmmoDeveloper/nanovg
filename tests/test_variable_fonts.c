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
	printf("=== NanoVG Variable Font Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Variable Font Test");
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

	// Test with both variable and non-variable fonts
	printf("Testing with NotoSans variable font...\n");
	int var_font = nvgCreateFont(vg, "noto-var", "/opt/notofonts.github.io/fonts/NotoSans/googlefonts/variable-ttf/NotoSans[wdth,wght].ttf");
	if (var_font == -1) {
		fprintf(stderr, "Failed to load variable font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("Testing with DejaVu Sans (non-variable)...\n");
	int regular_font = nvgCreateFont(vg, "dejavu", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (regular_font == -1) {
		fprintf(stderr, "Failed to load regular font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("\n=== Testing Variable Font Detection ===\n\n");

	// Test variable font
	nvgFontFaceId(vg, var_font);
	int is_var = nvgFontIsVariable(vg);
	printf("NotoSans variable font:\n");
	printf("  Is variable: %s\n", is_var ? "yes" : "no");

	if (is_var) {
		int axis_count = nvgFontVariationAxisCount(vg);
		printf("  Axis count: %d\n", axis_count);

		for (int i = 0; i < axis_count; i++) {
			NVGvarAxis axis;
			if (nvgFontVariationAxis(vg, i, &axis) == 0) {
				printf("  Axis %d:\n", i);
				printf("    Name: %s\n", axis.name);
				printf("    Tag: ");
				print_tag(axis.tag);
				printf("\n");
				printf("    Range: %.1f to %.1f (default: %.1f)\n",
					axis.minimum, axis.maximum, axis.def);
			}
		}

		int instance_count = nvgFontNamedInstanceCount(vg);
		printf("  Named instances: %d\n", instance_count);
	}

	printf("\n");

	// Test regular font
	nvgFontFaceId(vg, regular_font);
	is_var = nvgFontIsVariable(vg);
	printf("DejaVu Sans (regular):\n");
	printf("  Is variable: %s\n", is_var ? "yes" : "no");
	printf("  Axis count: %d\n", nvgFontVariationAxisCount(vg));

	printf("\n=== Testing Variation Axis Manipulation ===\n\n");

	nvgFontFaceId(vg, var_font);
	int num_axes = nvgFontVariationAxisCount(vg);

	// Get current coordinates
	float* coords = (float*)malloc(num_axes * sizeof(float));
	if (nvgFontGetVariationAxes(vg, coords, num_axes) == 0) {
		printf("Current design coordinates:\n");
		for (int i = 0; i < num_axes; i++) {
			NVGvarAxis axis;
			nvgFontVariationAxis(vg, i, &axis);
			printf("  %s: %.1f\n", axis.name, coords[i]);
		}
	}

	printf("\nSetting weight to 700 (bold), width to 100 (condensed)...\n");
	coords[0] = 100.0f;  // Width
	coords[1] = 700.0f;  // Weight
	if (nvgFontSetVariationAxes(vg, coords, num_axes) == 0) {
		printf("  Success!\n");

		// Read back
		if (nvgFontGetVariationAxes(vg, coords, num_axes) == 0) {
			printf("  Verified coordinates:\n");
			for (int i = 0; i < num_axes; i++) {
				NVGvarAxis axis;
				nvgFontVariationAxis(vg, i, &axis);
				printf("    %s: %.1f\n", axis.name, coords[i]);
			}
		}
	}

	printf("\nResetting to defaults...\n");
	if (nvgFontSetVariationAxes(vg, NULL, 0) == 0) {
		printf("  Success!\n");
		if (nvgFontGetVariationAxes(vg, coords, num_axes) == 0) {
			printf("  Coordinates after reset:\n");
			for (int i = 0; i < num_axes; i++) {
				NVGvarAxis axis;
				nvgFontVariationAxis(vg, i, &axis);
				printf("    %s: %.1f\n", axis.name, coords[i]);
			}
		}
	}

	free(coords);

	printf("\n=== Test Complete ===\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	return 0;
}
