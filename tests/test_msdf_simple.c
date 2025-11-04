// Simple MSDF test using proper abstractions
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

int main(void)
{
	printf("=== Simple MSDF Test ===\n\n");

	// Create window context
	WindowVulkanContext* winCtx = window_create_context(800, 600, "MSDF Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window\n");
		return 1;
	}

	// Create NanoVG context using helper function
	NVGcontext* vg = window_create_nanovg_context(winCtx, NVG_ANTIALIAS | (1 << 13));
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	// Load font
	int font = nvgCreateFont(vg, "sans", FONT_PATH);
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Enable MSDF mode
	nvgSetFontMSDF(vg, font, 2);  // MSDF_MODE_MSDF
	printf("✓ Setup complete\n\n");

	// Render one frame
	printf("Rendering frame...\n");

	uint32_t imageIndex;
	VkCommandBuffer cmd;
	if (window_begin_frame(winCtx, &imageIndex, &cmd) == 1) {
		int width, height;
		window_get_framebuffer_size(winCtx, &width, &height);

		// Begin NanoVG frame
		nvgBeginFrame(vg, width, height, 1.0f);

		// Clear background
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, width, height);
		nvgFillColor(vg, nvgRGB(28, 30, 34));
		nvgFill(vg);

		// Draw MSDF text
		nvgFontSize(vg, 72.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGB(255, 255, 255));
		nvgText(vg, 100, 200, "Hello MSDF!", NULL);

		nvgFontSize(vg, 48.0f);
		nvgText(vg, 100, 300, "GPU Text Rendering", NULL);

		// End frame
		nvgEndFrame(vg);

		window_end_frame(winCtx, imageIndex, cmd);

		printf("✓ Frame rendered\n");

		// Save screenshot
		window_save_screenshot(winCtx, imageIndex, "msdf_simple_test.png");
		printf("✓ Screenshot saved\n");
	}

	// Cleanup
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n✓ Test complete\n");
	return 0;
}
