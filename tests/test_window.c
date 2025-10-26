#include "window_utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("Creating Vulkan window...\n");

	WindowVulkanContext* ctx = window_create_context(800, 600, "Test Window");
	if (!ctx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	printf("Window created successfully!\n");
	printf("Rendering a few frames...\n");

	// Render 60 frames (about 1 second at 60fps)
	int screenshotTaken = 0;
	for (int frame = 0; frame < 60; frame++) {
		uint32_t imageIndex;
		VkCommandBuffer cmd;
		if (!window_begin_frame(ctx, &imageIndex, &cmd)) {
			fprintf(stderr, "Failed to begin frame\n");
			break;
		}

		// Begin render pass
		VkRenderPassBeginInfo renderPassInfo = {0};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = ctx->renderPass;
		renderPassInfo.framebuffer = ctx->framebuffers[imageIndex];
		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = ctx->swapchainExtent;

		VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Nothing to render yet - just clear to dark gray

		vkCmdEndRenderPass(cmd);

		window_end_frame(ctx, imageIndex, cmd);

		// Take screenshot on frame 10
		if (frame == 10 && !screenshotTaken) {
			printf("Taking screenshot...\n");
			if (window_save_screenshot(ctx, imageIndex, "/tmp/test-window.ppm")) {
				printf("Screenshot saved successfully\n");
				screenshotTaken = 1;
			} else {
				fprintf(stderr, "Failed to save screenshot\n");
			}
		}

		if (window_should_close(ctx)) {
			break;
		}

		window_poll_events();
	}

	printf("Cleaning up...\n");
	window_destroy_context(ctx);
	printf("Test completed successfully!\n");

	return 0;
}
