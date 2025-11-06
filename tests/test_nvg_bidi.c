#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

// Visual BiDi (Bidirectional Text) Test
// Renders Arabic, Hebrew, and mixed LTR/RTL text using NanoVG

int main(void)
{
	printf("=== NanoVG BiDi Visual Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "BiDi Text Test");
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

	// Load fonts
	printf("3. Loading fonts...\n");
	int arabicFont = nvgCreateFont(vg, "arabic", "fonts/arabic/NotoSansArabic-Regular.ttf");
	if (arabicFont == -1) {
		fprintf(stderr, "Failed to load Arabic font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Arabic font loaded (id=%d)\n", arabicFont);

	int hebrewFont = nvgCreateFont(vg, "hebrew", "fonts/hebrew/NotoSansHebrew-Regular.ttf");
	if (hebrewFont == -1) {
		fprintf(stderr, "Failed to load Hebrew font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Hebrew font loaded (id=%d)\n", hebrewFont);

	int latinFont = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (latinFont == -1) {
		fprintf(stderr, "Failed to load Latin font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Latin font loaded (id=%d)\n\n", latinFont);

	printf("4. Rendering BiDi text (press ESC to close, window will auto-close after 5 seconds)...\n\n");

	// Text samples
	const char* texts[] = {
		"مرحبا بالعالم",      // Arabic: Hello World (RTL)
		"שלום עולם",          // Hebrew: Hello World (RTL)
		"Hello مرحبا World",  // Mixed: English + Arabic
		"Hello שלום World",   // Mixed: English + Hebrew
		"السلام عليكم",      // Arabic: Peace be upon you (RTL)
		"Testing BiDi"       // English LTR (control)
	};

	int textFonts[] = {arabicFont, hebrewFont, arabicFont, hebrewFont, arabicFont, latinFont};
	const char* labels[] = {"Arabic RTL:", "Hebrew RTL:", "Mixed AR+EN:", "Mixed HE+EN:", "Arabic RTL:", "English LTR:"};

	// Render loop - capture screenshot on first frame
	int frameCount = 0;
	int maxFrames = 300;  // 5 seconds at 60fps
	int screenshotSaved = 0;
	uint32_t imageIndex = 0;

	while (!glfwWindowShouldClose(winCtx->window) && frameCount < maxFrames) {
		glfwPollEvents();

		// Acquire swapchain image
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkSemaphore imageAvailableSemaphore;
		vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

		vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
		                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		// Get command buffer
		VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

		// Begin command buffer
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

		// Dark background
		VkClearValue clearValues[2] = {0};
		clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.15f, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		VkViewport viewport = {0};
		viewport.width = 800.0f;
		viewport.height = 600.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = {0};
		scissor.extent = winCtx->swapchainExtent;
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, 800, 600, 1.0f);

		// Title
		nvgFontFaceId(vg, latinFont);
		nvgFontSize(vg, 48.0f);
		nvgFillColor(vg, nvgRGBA(255, 230, 128, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgText(vg, 400.0f, 30.0f, "BiDi Text Rendering", NULL);

		// Render text samples
		float y = 120.0f;
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		for (int i = 0; i < 6; i++) {
			// Label in gray
			nvgFontFaceId(vg, latinFont);
			nvgFontSize(vg, 20.0f);
			nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
			nvgText(vg, 50.0f, y, labels[i], NULL);

			// Text sample in color
			nvgFontFaceId(vg, textFonts[i]);
			nvgFontSize(vg, 32.0f);
			NVGcolor colors[] = {
				nvgRGBA(100, 200, 255, 255),  // light blue
				nvgRGBA(255, 180, 100, 255),  // orange
				nvgRGBA(150, 255, 150, 255),  // green
				nvgRGBA(255, 150, 255, 255),  // pink
				nvgRGBA(255, 255, 150, 255),  // yellow
				nvgRGBA(200, 200, 255, 255)   // light purple
			};
			nvgFillColor(vg, colors[i]);
			nvgText(vg, 50.0f, y + 25.0f, texts[i], NULL);

			y += 80.0f;
		}

		// Footer
		nvgFontFaceId(vg, latinFont);
		nvgFontSize(vg, 18.0f);
		nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
		nvgText(vg, 400.0f, 580.0f, "HarfBuzz + FriBidi Integration", NULL);

		// End NanoVG frame
		nvgEndFrame(vg);

		// End render pass
		vkCmdEndRenderPass(cmdBuf);
		vkEndCommandBuffer(cmdBuf);

		// Submit
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.pWaitDstStageMask = waitStages;

		vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(winCtx->graphicsQueue);

		// Present
		VkPresentInfoKHR presentInfo = {0};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &winCtx->swapchain;
		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);

		vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

		// Save screenshot after first frame
		if (frameCount == 0 && !screenshotSaved) {
			printf("   Saving screenshot...\n");
			if (window_save_screenshot(winCtx, imageIndex, "build/test/screendumps/bidi_test.ppm")) {
				printf("   ✓ Screenshot saved to bidi_test.ppm\n");
				screenshotSaved = 1;
			}
		}

		frameCount++;
	}

	printf("   ✓ Rendered %d frames\n\n", frameCount);

	// Cleanup
	printf("5. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);
	printf("   ✓ Cleanup complete\n");

	printf("\n=== BiDi Visual Test Complete ===\n");
	return 0;
}
