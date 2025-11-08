#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "window_utils.h"

int main(void) {
	printf("=== NanoVG Basic Text Test ===\n\n");

	// 1. Create window
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Text Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// 2. Create NanoVG context
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");

	// 3. Load font
	printf("3. Loading font...\n");
	int font = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		fprintf(stderr, "Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Font loaded (ID: %d)\n\n", font);

	// 4. Acquire swapchain image
	printf("4. Acquiring swapchain image...\n");
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain,
	                                        UINT64_MAX,
	                                        winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                                        VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to acquire swapchain image\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Image acquired (index: %d)\n\n", imageIndex);

	// 5. Begin rendering
	printf("5. Beginning command buffer...\n");
	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset.x = 0;
	renderPassInfo.renderArea.offset.y = 0;
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearColor = {{{0.2f, 0.2f, 0.2f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport and scissor
	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);
	printf("   ✓ Render pass begun\n\n");

	// 6. Draw text with NanoVG
	printf("6. Drawing text with NanoVG...\n");

	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Draw background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 800, 600);
	nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
	nvgFill(vg);

	// Draw text test
	nvgFontSize(vg, 48.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgText(vg, 50, 50, "Hello, World!", NULL);

	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 255, 255));
	nvgText(vg, 50, 120, "The quick brown fox", NULL);

	nvgEndFrame(vg);
	printf("   ✓ Text rendered\n\n");

	// 7. End rendering
	printf("7. Ending render pass...\n");
	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkSemaphore signalSemaphores[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);

	if (vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo,
	                  winCtx->inFlightFences[winCtx->currentFrame]) != VK_SUCCESS) {
		fprintf(stderr, "Failed to submit draw command buffer\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	vkWaitForFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame],
	                VK_TRUE, UINT64_MAX);
	printf("   ✓ Submitted and rendered\n\n");

	// 8. Save screenshot
	printf("8. Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/text_basic_test.ppm")) {
		printf("   ✓ Screenshot saved to build/test/screendumps/text_basic_test.ppm\n\n");
	} else {
		fprintf(stderr, "   ✗ Failed to save screenshot\n\n");
	}

	// 9. Cleanup
	printf("9. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Basic Text Test PASSED ===\n");
	return 0;
}
