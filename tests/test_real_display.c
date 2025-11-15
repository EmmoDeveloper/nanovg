#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"
#include "../src/vulkan/nvg_vk_display_real.h"

int main(void) {
	printf("=== Real Display Detection and Quaternion Color Space Test ===\n\n");

	// Create window and Vulkan context
	WindowVulkanContext* winCtx = window_create_context(1600, 1000, "Real Display Test");
	if (!winCtx) {
		printf("Failed to create window context\n");
		return 1;
	}

	// Create NanoVG context
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		printf("Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	// Load font
	int font = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		printf("Failed to load font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Create real display with detection
	printf("Detecting real display...\n");
	NVGVkRealDisplay* realDisplay = nvgvk_real_display_create(
		winCtx->physicalDevice,
		winCtx->surface
	);

	if (!realDisplay) {
		printf("Failed to create real display\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Print detailed display information
	nvgvk_real_display_print_info(realDisplay);
	printf("\n");

	// Choose optimal color space
	VkColorSpaceKHR optimalColorSpace = nvgvk_real_display_choose_color_space(
		realDisplay,
		0,  // preferHDR
		realDisplay->supportsWideGamut  // preferWideGamut if available
	);

	printf("Chosen Color Space: %s\n", nvgvk_color_space_get_name(optimalColorSpace));

	// Get optimal format
	VkSurfaceFormatKHR optimalFormat = nvgvk_real_display_choose_format(
		realDisplay, optimalColorSpace);

	printf("Chosen Format: %d\n", optimalFormat.format);
	printf("\n");

	// Update transform
	nvgvk_real_display_update_transform(realDisplay, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

	// Test color space transitions
	printf("Testing color space transitions...\n");

	VkColorSpaceKHR testSpaces[] = {
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
		VK_COLOR_SPACE_BT2020_LINEAR_EXT
	};

	for (int i = 0; i < 3; i++) {
		// Check if supported
		int supported = 0;
		for (uint32_t j = 0; j < realDisplay->surfaceFormatCount; j++) {
			if (realDisplay->surfaceFormats[j].colorSpace == testSpaces[i]) {
				supported = 1;
				break;
			}
		}

		if (supported) {
			printf("  Transition to %s...\n", nvgvk_color_space_get_name(testSpaces[i]));

			nvgvk_display_set_target_color_space(
				realDisplay->colorManager,
				testSpaces[i],
				VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
			);

			// Simulate animation frames
			for (int frame = 0; frame < 5; frame++) {
				nvgvk_display_update_transition(realDisplay->colorManager, 1.0f / 60.0f);
			}

			const NVGVkColorSpaceTransform* transform = nvgvk_real_display_get_transform(realDisplay);
			if (transform) {
				printf("    Valid: %s, Det: %.4f, Ortho: %.6f\n",
				       transform->isValid ? "Yes" : "No",
				       transform->determinant,
				       transform->orthogonalityError);
			}
		} else {
			printf("  %s: Not supported\n", nvgvk_color_space_get_name(testSpaces[i]));
		}
	}
	printf("\n");

	// Render visualization
	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.12f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1600, 1000, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1600, 1000, 1.0f);

	// Title
	nvgFontSize(vg, 40.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(100, 200, 255, 255));
	nvgText(vg, 50, 60, "Real Display Detection", NULL);

	// Display info visualization
	float y = 120;

	// Display name
	nvgFontSize(vg, 28.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 70, y, realDisplay->physicalInfo.name, NULL);
	y += 50;

	// Resolution and specs
	nvgFontSize(vg, 20.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));

	char resText[128];
	snprintf(resText, sizeof(resText), "Resolution: %dx%d @ %dHz",
	         realDisplay->physicalInfo.width,
	         realDisplay->physicalInfo.height,
	         realDisplay->physicalInfo.refreshRate);
	nvgText(vg, 90, y, resText, NULL);
	y += 35;

	char sizeText[128];
	snprintf(sizeText, sizeof(sizeText), "Size: %.1f\" diagonal, %.1f DPI",
	         realDisplay->diagonalInches, realDisplay->dpi);
	nvgText(vg, 90, y, sizeText, NULL);
	y += 35;

	char subpixelText[128];
	snprintf(subpixelText, sizeof(subpixelText), "Subpixel: %s (%s)",
	         display_subpixel_name(realDisplay->physicalInfo.subpixel),
	         realDisplay->subpixelRenderingEnabled ? "Enabled" : "Disabled");
	nvgText(vg, 90, y, subpixelText, NULL);
	y += 60;

	// Capabilities section
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 70, y, "Capabilities", NULL);
	y += 40;

	nvgFontSize(vg, 18.0f);

	// HDR support
	NVGcolor hdrColor = realDisplay->supportsHDR ?
		nvgRGBA(100, 255, 100, 255) : nvgRGBA(150, 150, 150, 255);
	nvgFillColor(vg, hdrColor);
	nvgText(vg, 90, y, realDisplay->supportsHDR ? "✓ HDR Support" : "✗ HDR Support", NULL);
	y += 35;

	// Wide gamut support
	NVGcolor wgColor = realDisplay->supportsWideGamut ?
		nvgRGBA(100, 255, 100, 255) : nvgRGBA(150, 150, 150, 255);
	nvgFillColor(vg, wgColor);
	nvgText(vg, 90, y, realDisplay->supportsWideGamut ? "✓ Wide Gamut" : "✗ Wide Gamut", NULL);
	y += 50;

	// Active color space
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 70, y, "Color Space Configuration", NULL);
	y += 40;

	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));

	char csText[128];
	snprintf(csText, sizeof(csText), "Recommended: %s",
	         nvgvk_color_space_get_name(realDisplay->recommendedColorSpace));
	nvgText(vg, 90, y, csText, NULL);
	y += 35;

	snprintf(csText, sizeof(csText), "Available Formats: %u",
	         realDisplay->surfaceFormatCount);
	nvgText(vg, 90, y, csText, NULL);
	y += 50;

	// Transform info
	const NVGVkColorSpaceTransform* transform = nvgvk_real_display_get_transform(realDisplay);
	if (transform && transform->isValid) {
		nvgFontSize(vg, 24.0f);
		nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
		nvgText(vg, 70, y, "Active Transform (Quaternion)", NULL);
		y += 40;

		nvgFontSize(vg, 16.0f);
		nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));

		char qText[128];
		snprintf(qText, sizeof(qText), "Quaternion: [%.3f, %.3f, %.3f, %.3f]",
		         transform->decomposed.rotation.w,
		         transform->decomposed.rotation.x,
		         transform->decomposed.rotation.y,
		         transform->decomposed.rotation.z);
		nvgText(vg, 90, y, qText, NULL);
		y += 30;

		snprintf(qText, sizeof(qText), "Scale: [%.3f, %.3f, %.3f]",
		         transform->decomposed.scale[0],
		         transform->decomposed.scale[1],
		         transform->decomposed.scale[2]);
		nvgText(vg, 90, y, qText, NULL);
		y += 30;

		nvgFillColor(vg, nvgRGBA(100, 255, 100, 255));
		snprintf(qText, sizeof(qText), "Determinant: %.6f | Ortho Error: %.8f",
		         transform->determinant, transform->orthogonalityError);
		nvgText(vg, 90, y, qText, NULL);
		y += 30;
	}

	// Subpixel rendering visualization
	if (realDisplay->subpixelRenderingEnabled) {
		y += 20;
		nvgFontSize(vg, 24.0f);
		nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
		nvgText(vg, 70, y, "Subpixel Rendering", NULL);
		y += 40;

		// Draw subpixel pattern
		float x = 90;
		float boxSize = 60;

		const char* labels[] = {"R", "G", "B"};
		NVGcolor colors[] = {
			nvgRGBA(255, 100, 100, 255),
			nvgRGBA(100, 255, 100, 255),
			nvgRGBA(100, 100, 255, 255)
		};

		for (int i = 0; i < 3; i++) {
			nvgBeginPath(vg);
			nvgRect(vg, x, y - 10, boxSize, boxSize);
			nvgFillColor(vg, colors[i]);
			nvgFill(vg);

			nvgFontSize(vg, 20.0f);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, x + boxSize/2 - 7, y + boxSize/2 + 7, labels[i], NULL);

			x += boxSize + 10;
		}
	}

	// Footer
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
	nvgText(vg, 50, 970, "Real display detection integrated with quaternion-based color space management", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSem[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSem;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSem[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSem;

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);
	vkWaitForFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame], VK_TRUE, UINT64_MAX);

	window_save_screenshot(winCtx, imageIndex, "screendumps/real_display.png");

	// Cleanup
	nvgvk_real_display_destroy(realDisplay);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("=== Real Display Test PASSED ===\n");
	printf("Screenshot saved: screendumps/real_display.png\n");

	return 0;
}
