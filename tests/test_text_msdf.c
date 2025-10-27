#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

int main(void)
{
	printf("=== NanoVG MSDF Text Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "MSDF Text Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context with MSDF support
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS | (1 << 13));  // NVG_MSDF_TEXT
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created with MSDF support\n\n");

	// Load font
	printf("3. Loading font...\n");
	int font = nvgCreateFont(vg, "sans", FONT_PATH);
	if (font == -1) {
		fprintf(stderr, "Failed to load font: %s\n", FONT_PATH);
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Font loaded (id: %d)\n", font);

	// Enable MSDF mode for the font
	nvgSetFontMSDF(vg, font, 2);  // 2 = MSDF_MODE_MSDF
	printf("   ✓ MSDF mode enabled\n\n");

	// Acquire swapchain image
	printf("4. Acquiring swapchain image...\n");
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	printf("   ✓ Image acquired (index: %u)\n\n", imageIndex);

	// Get command buffer from NanoVG
	VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

	// Begin command buffer
	printf("5. Beginning command buffer...\n");
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	// Begin render pass
	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearValues[2] = {0};
	clearValues[0].color.float32[0] = 0.2f;
	clearValues[0].color.float32[1] = 0.2f;
	clearValues[0].color.float32[2] = 0.25f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	// Set viewport and scissor
	VkViewport viewport = {0};
	viewport.width = (float)winCtx->swapchainExtent.width;
	viewport.height = (float)winCtx->swapchainExtent.height;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG about the render pass (required for texture uploads)
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);
	printf("   ✓ Render pass begun\n\n");

	// Draw text with NanoVG
	printf("6. Drawing text with NanoVG...\n");
	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Draw background rectangle
	nvgBeginPath(vg);
	nvgRect(vg, 50, 50, 700, 500);
	nvgFillColor(vg, nvgRGBA(40, 40, 50, 255));
	nvgFill(vg);

	// Draw text samples at different sizes
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	// Large text
	nvgFontSize(vg, 72.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 100, 100, "Hello MSDF!", NULL);

	// Medium text
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(100, 200, 255, 255));
	nvgText(vg, 100, 200, "Text Rendering", NULL);

	// Small text
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 100, 280, "Multi-channel Signed Distance Fields", NULL);

	// Very small text
	nvgFontSize(vg, 16.0f);
	nvgFillColor(vg, nvgRGBA(200, 255, 200, 255));
	nvgText(vg, 100, 330, "Sharp text at any scale with GPU acceleration", NULL);

	nvgEndFrame(vg);
	printf("   ✓ Text drawn\n\n");

	// End render pass
	printf("7. Ending render pass...\n");
	vkCmdEndRenderPass(cmdBuf);
	vkEndCommandBuffer(cmdBuf);

	// Submit
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);
	printf("   ✓ Submitted and rendered\n\n");

	// Save screenshot
	printf("8. Saving screenshot...\n");
	window_save_screenshot(winCtx, imageIndex, "text_msdf_test.ppm");
	printf("   ✓ Screenshot saved to text_msdf_test.ppm\n\n");

	// Cleanup
	printf("9. Cleaning up...\n");
	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== MSDF Text Test PASSED ===\n");
	return 0;
}
