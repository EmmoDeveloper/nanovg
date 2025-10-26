#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Multi-Shape Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "Multi-Shape Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	printf("Drawing multiple shapes...\n");
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Red circle
	nvgBeginPath(vg);
	nvgCircle(vg, 200, 200, 80);
	nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
	nvgFill(vg);

	// Green rectangle
	nvgBeginPath(vg);
	nvgRect(vg, 350, 150, 150, 100);
	nvgFillColor(vg, nvgRGBA(0, 255, 0, 255));
	nvgFill(vg);

	// Blue rounded rectangle with stroke
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 100, 350, 200, 150, 10);
	nvgFillColor(vg, nvgRGBA(0, 0, 255, 200));
	nvgFill(vg);
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgStrokeWidth(vg, 3.0f);
	nvgStroke(vg);

	// Yellow circle with gradient
	nvgBeginPath(vg);
	nvgCircle(vg, 600, 400, 100);
	NVGpaint gradient = nvgLinearGradient(vg, 550, 350, 650, 450,
	                                       nvgRGBA(255, 255, 0, 255),
	                                       nvgRGBA(255, 128, 0, 255));
	nvgFillPaint(vg, gradient);
	nvgFill(vg);

	nvgEndFrame(vg);
	printf("All shapes drawn successfully!\n\n");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Multi-Shape Test PASSED ===\n");
	return 0;
}
