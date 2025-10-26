#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Shapes Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Shapes Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");

	// Begin frame
	printf("3. Beginning frame...\n");
	nvgBeginFrame(vg, 800, 600, 1.0f);
	printf("   ✓ Frame begun\n\n");

	// Draw a simple red circle
	printf("4. Drawing red circle...\n");
	nvgBeginPath(vg);
	nvgCircle(vg, 400, 300, 80);
	nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
	nvgFill(vg);
	printf("   ✓ Circle drawn\n\n");

	// End frame
	printf("5. Ending frame...\n");
	nvgEndFrame(vg);
	printf("   ✓ Frame ended\n\n");

	// Cleanup
	printf("6. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Shapes Test PASSED ===\n");
	return 0;
}
