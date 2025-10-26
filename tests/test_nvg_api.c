#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	printf("=== NanoVG Vulkan API Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "NanoVG API Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context using new API
	printf("2. Creating NanoVG context with nvgCreateVk...\n");
	fflush(stdout);
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	printf("   nvgCreateVk returned: %p\n", (void*)vg);
	fflush(stdout);
	printf("   About to check if vg is null...\n");
	fflush(stdout);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");
	fflush(stdout);

	// TODO: Implement full rendering test once all features are complete
	// For now, just verify context creation succeeds

	// Cleanup
	printf("\n3. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== NanoVG API Test PASSED (Basic) ===\n");
	printf("NOTE: Full rendering test disabled until all features implemented\n");
	return 0;
}
