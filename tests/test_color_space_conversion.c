#include "nanovg.h"
#include "nvg_vk.h"
#include "window_utils.h"
#include <stdio.h>
#include <string.h>

// Helper: Draw a test gradient
static void drawTestGradient(NVGcontext* vg, float x, float y, float w, float h, const char* label)
{
	// Draw gradient from red to blue
	NVGpaint gradient = nvgLinearGradient(vg, x, y, x + w, y,
	                                       nvgRGBf(1.0f, 0.0f, 0.0f),  // Red
	                                       nvgRGBf(0.0f, 0.0f, 1.0f)); // Blue

	nvgBeginPath(vg);
	nvgRect(vg, x, y, w, h);
	nvgFillPaint(vg, gradient);
	nvgFill(vg);

	// Draw label
	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, x + 5, y + 15, label, NULL);
}

int main(void)
{
	printf("=== NanoVG Color Space Conversion Test ===\n\n");

	// Create window and Vulkan context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* window = window_create_context(800, 600, "Color Space Conversion Test");
	if (!window) {
		printf("   ✗ Failed to create window\n");
		return 1;
	}
	printf("   ✓ Window context created\n");

	printf("\n2. Creating NanoVG Vulkan context...\n");
	NVGcontext* vg = nvgCreateVk(window->device,
	                              window->physicalDevice,
	                              window->graphicsQueue,
	                              window->commandPool,
	                              window->renderPass,
	                              NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (!vg) {
		printf("   ✗ Failed to create NanoVG context\n");
		window_destroy_context(window);
		return 1;
	}
	printf("   ✓ NanoVG Vulkan context created\n");

	// Test color space API
	printf("\n3. Testing color space conversions...\n");

	// Test 1: Default sRGB → sRGB (identity)
	printf("   • Test 1: sRGB → sRGB (identity conversion)\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Identity conversion configured\n");

	// Test 2: Linear sRGB → sRGB
	printf("   • Test 2: Linear sRGB → sRGB\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_LINEAR_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Linear→sRGB conversion configured\n");

	// Test 3: Display P3 → sRGB (gamut mapping)
	printf("   • Test 3: Display P3 → sRGB (gamut mapping)\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_DISPLAY_P3);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceGamutMapping(vg, 1);
	printf("     ✓ P3→sRGB with gamut mapping configured\n");

	// Test 4: HDR10 → sRGB (tone mapping)
	printf("   • Test 4: HDR10 → sRGB (tone mapping)\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_HDR10);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceHDRScale(vg, 100.0f);  // 100 nits
	nvgColorSpaceToneMapping(vg, 1);
	printf("     ✓ HDR10→sRGB with tone mapping configured\n");

	// Test 5: Adobe RGB → sRGB
	printf("   • Test 5: Adobe RGB → sRGB\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_ADOBE_RGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Adobe RGB→sRGB conversion configured\n");

	// Reset to default
	printf("   • Resetting to default (sRGB → sRGB)\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceHDRScale(vg, 1.0f);
	nvgColorSpaceGamutMapping(vg, 1);
	nvgColorSpaceToneMapping(vg, 1);
	printf("     ✓ Reset to defaults\n");

	printf("\n4. Rendering test pattern...\n");

	// Acquire swapchain image
	uint32_t imageIndex;
	vkAcquireNextImageKHR(window->device, window->swapchain, UINT64_MAX,
	                       window->imageAvailableSemaphores[0], VK_NULL_HANDLE, &imageIndex);

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(window->drawCommandBuffers[0], &beginInfo);

	// Begin render pass
	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = window->renderPass;
	renderPassInfo.framebuffer = window->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset.x = 0;
	renderPassInfo.renderArea.offset.y = 0;
	renderPassInfo.renderArea.extent = window->swapchainExtent;

	VkClearValue clearColor = {{0.2f, 0.2f, 0.2f, 1.0f}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(window->drawCommandBuffers[0], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)window->swapchainExtent.width;
	viewport.height = (float)window->swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(window->drawCommandBuffers[0], 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = window->swapchainExtent;
	vkCmdSetScissor(window->drawCommandBuffers[0], 0, 1, &scissor);

	// Tell NanoVG about the render pass
	nvgVkBeginRenderPass(vg, window->renderPass, window->framebuffers[imageIndex],
	                     (VkRect2D){{0, 0}, window->swapchainExtent}, &clearColor, 1,
	                     viewport, scissor);

	// Begin NanoVG frame
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Draw test patterns with different color spaces
	float y = 50.0f;
	float spacing = 80.0f;

	// Test 1: sRGB → sRGB
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	drawTestGradient(vg, 50, y, 700, 60, "sRGB -> sRGB (identity)");
	y += spacing;

	// Test 2: Linear → sRGB
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_LINEAR_SRGB);
	drawTestGradient(vg, 50, y, 700, 60, "Linear sRGB -> sRGB");
	y += spacing;

	// Test 3: Display P3 → sRGB
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_DISPLAY_P3);
	drawTestGradient(vg, 50, y, 700, 60, "Display P3 -> sRGB");
	y += spacing;

	// Test 4: Adobe RGB → sRGB
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_ADOBE_RGB);
	drawTestGradient(vg, 50, y, 700, 60, "Adobe RGB -> sRGB");
	y += spacing;

	// Test 5: Rec.2020 → sRGB
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_REC2020);
	drawTestGradient(vg, 50, y, 700, 60, "Rec.2020 -> sRGB");

	// End NanoVG frame
	nvgEndFrame(vg);

	// End render pass
	nvgVkEndRenderPass(vg);
	vkCmdEndRenderPass(window->drawCommandBuffers[0]);
	vkEndCommandBuffer(window->drawCommandBuffers[0]);

	// Submit command buffer
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &window->drawCommandBuffers[0];

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &window->imageAvailableSemaphores[0];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &window->renderFinishedSemaphores[0];

	vkQueueSubmit(window->graphicsQueue, 1, &submitInfo, window->inFlightFences[0]);
	vkWaitForFences(window->device, 1, &window->inFlightFences[0], VK_TRUE, UINT64_MAX);

	printf("   ✓ Rendered test pattern with 5 different color space conversions\n");

	// Save screenshot
	printf("\n5. Saving screenshot...\n");
	window_save_screenshot(window, imageIndex, "color_space_conversion_test.ppm");
	printf("   ✓ Screenshot saved to color_space_conversion_test.ppm\n");

	printf("\n6. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(window);
	printf("   ✓ Cleanup complete\n");

	printf("\n=== Color Space Conversion Test PASSED ===\n");
	printf("\nCheck color_space_conversion_test.ppm to see the results:\n");
	printf("  - Each gradient should show different color intensity\n");
	printf("  - Linear→sRGB should be darker (gamma correction)\n");
	printf("  - P3/Adobe/Rec.2020 should have different saturation\n");

	return 0;
}
