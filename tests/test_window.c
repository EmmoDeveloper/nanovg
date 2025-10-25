#include "window_utils.h"
#include <stdio.h>
#include <math.h>
#include "nanovg.h"
#include "nanovg_vk.h"

int main(void)
{
	printf("Creating Vulkan window context...\n");
	fflush(stdout);
	WindowVulkanContext* ctx = window_create_context(800, 600, "NanoVG Vulkan Test");
	if (!ctx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	printf("Window context created successfully!\n");
	fflush(stdout);

	printf("Creating NanoVG context...\n");
	fflush(stdout);
	NVGcontext* vg = window_create_nanovg_context(ctx, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(ctx);
		return 1;
	}

	printf("NanoVG context created successfully!\n");
	fflush(stdout);

	printf("Entering render loop...\n");
	fflush(stdout);
	float time = 0.0f;

	while (!window_should_close(ctx)) {
		window_poll_events();

		uint32_t imageIndex;
		VkCommandBuffer cmdBuf;

		if (!window_begin_frame(ctx, &imageIndex, &cmdBuf)) {
			continue;
		}

		VkImageView imageView = window_get_swapchain_image_view(ctx, imageIndex);
		nvgVkSetRenderTargets(vg, imageView, VK_NULL_HANDLE);

		int width, height;
		window_get_framebuffer_size(ctx, &width, &height);

		nvgBeginFrame(vg, width, height, 1.0f);

		nvgBeginPath(vg);
		nvgRect(vg, 100, 100, 200, 200);
		nvgFillColor(vg, nvgRGBA(255, 100, 100, 200));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, 400, 200, 80);
		nvgFillColor(vg, nvgRGBA(100, 200, 255, 200));
		nvgFill(vg);

		float x = 400 + cosf(time) * 150;
		float y = 400 + sinf(time) * 150;
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, 40);
		nvgFillColor(vg, nvgRGBA(100, 255, 100, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, 500, 400, 180, 120, 10);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);

		nvgEndFrame(vg);

		window_end_frame(ctx, imageIndex, cmdBuf);

		time += 0.016f;
	}

	printf("Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(ctx);

	printf("Test completed successfully\n");
	return 0;
}
