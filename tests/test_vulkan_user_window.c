#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "nanovg/nanovg.h"
#include "backends/vulkan/nvg_vk.h"
#include "window_utils.h"
#include "display_detection.h"

static void render_text_line(NVGcontext* vg, float* y, const char* text, int bold) {
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, bold ? 20.0f : 16.0f);
	nvgFillColor(vg, nvgRGBA(bold ? 255 : 220, bold ? 255 : 220, bold ? 255 : 220, 255));
	nvgText(vg, 20, *y, text, NULL);
	*y += bold ? 30 : 24;
}

int main(void) {
	printf("=== YOUR VULKAN SPACE ===\n\n");

	// Detect display configuration
	DisplayDetectionInfo displayInfo = {0};
	int hasDisplayDetection = detect_display_info(&displayInfo);

	if (hasDisplayDetection) {
		printf("Display Detection Results:\n");
		printf("  Name: %s\n", displayInfo.name);
		printf("  Resolution: %dx%d @ %dHz\n",
		       displayInfo.width, displayInfo.height, displayInfo.refreshRate);
		if (displayInfo.physicalWidthMM > 0) {
			printf("  Physical: %dmm x %dmm\n",
			       displayInfo.physicalWidthMM, displayInfo.physicalHeightMM);
		}
		if (displayInfo.scale > 0.0f) {
			printf("  Scale: %.1fx\n", displayInfo.scale);
		}
		printf("  Subpixel: %s\n", display_subpixel_name(displayInfo.subpixel));
		printf("\n");
	} else {
		printf("WARNING: Display detection failed\n\n");
	}

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "YOUR VULKAN SPACE");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (font == -1) {
		printf("Warning: Could not load font, text rendering may fail\n");
	}

	// Get Vulkan device properties
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(winCtx->physicalDevice, &deviceProps);

	// Get Vulkan memory properties
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(winCtx->physicalDevice, &memProps);

	// Get extension count
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(winCtx->physicalDevice, NULL, &extCount, NULL);

	// RUNTIME QUERY: Surface capabilities
	VkSurfaceCapabilitiesKHR surfaceCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(winCtx->physicalDevice, winCtx->surface, &surfaceCaps);

	// RUNTIME QUERY: Available surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(winCtx->physicalDevice, winCtx->surface, &formatCount, NULL);
	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(winCtx->physicalDevice, winCtx->surface, &formatCount, formats);

	// RUNTIME QUERY: Available present modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(winCtx->physicalDevice, winCtx->surface, &presentModeCount, NULL);
	VkPresentModeKHR* presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(winCtx->physicalDevice, winCtx->surface, &presentModeCount, presentModes);

	// RUNTIME QUERY: Queue family properties
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(winCtx->physicalDevice, &queueFamilyCount, NULL);
	VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(winCtx->physicalDevice, &queueFamilyCount, queueFamilies);

	// Prepare info strings
	char lines[60][256];
	int lineCount = 0;

	snprintf(lines[lineCount++], 256, "=== YOUR VULKAN SPACE ===");
	lines[lineCount][0] = '\0'; lineCount++;
	snprintf(lines[lineCount++], 256, "VULKAN DEVICE");
	snprintf(lines[lineCount++], 256, "");
	snprintf(lines[lineCount++], 256, "Device: %s", deviceProps.deviceName);
	snprintf(lines[lineCount++], 256, "Type: %s",
	         deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" :
	         deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "Integrated GPU" :
	         deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? "Virtual GPU" :
	         deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? "CPU" : "Other");
	snprintf(lines[lineCount++], 256, "API Version: %u.%u.%u",
	         VK_VERSION_MAJOR(deviceProps.apiVersion),
	         VK_VERSION_MINOR(deviceProps.apiVersion),
	         VK_VERSION_PATCH(deviceProps.apiVersion));
	snprintf(lines[lineCount++], 256, "Driver Version: %u.%u.%u",
	         VK_VERSION_MAJOR(deviceProps.driverVersion),
	         VK_VERSION_MINOR(deviceProps.driverVersion),
	         VK_VERSION_PATCH(deviceProps.driverVersion));
	snprintf(lines[lineCount++], 256, "Vendor ID: 0x%04X", deviceProps.vendorID);
	snprintf(lines[lineCount++], 256, "Device Extensions: %u", extCount);

	// Memory info
	snprintf(lines[lineCount++], 256, "");
	snprintf(lines[lineCount++], 256, "MEMORY HEAPS");
	for (uint32_t i = 0; i < memProps.memoryHeapCount && lineCount < 30; i++) {
		VkDeviceSize sizeGB = memProps.memoryHeaps[i].size / (1024 * 1024 * 1024);
		snprintf(lines[lineCount++], 256, "  Heap %u: %llu GB %s", i,
		         (unsigned long long)sizeGB,
		         (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "(Device Local)" : "");
	}

	// Display info
	if (hasDisplayDetection) {
		lines[lineCount][0] = '\0'; lineCount++;
		snprintf(lines[lineCount++], 256, "YOUR DISPLAY");
		if (displayInfo.name[0]) {
			snprintf(lines[lineCount++], 256, "Name: %s", displayInfo.name);
		}
		if (displayInfo.width > 0) {
			snprintf(lines[lineCount++], 256, "Resolution: %dx%d @ %dHz",
			         displayInfo.width, displayInfo.height, displayInfo.refreshRate);
		}
		if (displayInfo.physicalWidthMM > 0) {
			snprintf(lines[lineCount++], 256, "Physical: %dmm x %dmm",
			         displayInfo.physicalWidthMM, displayInfo.physicalHeightMM);
		}
		if (displayInfo.scale > 0.0f) {
			snprintf(lines[lineCount++], 256, "Scale Factor: %.1fx", displayInfo.scale);
		}
		snprintf(lines[lineCount++], 256, "Subpixel: %s",
		         display_subpixel_name(displayInfo.subpixel));

		// Recommendation
		lines[lineCount][0] = '\0'; lineCount++;
		snprintf(lines[lineCount++], 256, "OPTIMAL RENDERING");
		int nvgMode = display_subpixel_to_nvg(displayInfo.subpixel);
		snprintf(lines[lineCount++], 256, "Use: nvgTextSubpixelMode(vg, %d)", nvgMode);
		snprintf(lines[lineCount++], 256, "Mode: %s", display_subpixel_name(displayInfo.subpixel));
	}

	lines[lineCount][0] = '\0'; lineCount++;
	snprintf(lines[lineCount++], 256, "SURFACE CAPABILITIES (RUNTIME QUERY)");
	snprintf(lines[lineCount++], 256, "Min/Max Images: %u/%u",
	         surfaceCaps.minImageCount,
	         surfaceCaps.maxImageCount ? surfaceCaps.maxImageCount : 999);
	snprintf(lines[lineCount++], 256, "Current Extent: %ux%u",
	         surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height);
	snprintf(lines[lineCount++], 256, "Supported Transforms: 0x%X", surfaceCaps.supportedTransforms);
	snprintf(lines[lineCount++], 256, "Supported Composite Alpha: 0x%X", surfaceCaps.supportedCompositeAlpha);

	lines[lineCount][0] = '\0'; lineCount++;
	snprintf(lines[lineCount++], 256, "AVAILABLE SURFACE FORMATS (%u)", formatCount);
	for (uint32_t i = 0; i < formatCount && lineCount < 55; i++) {
		const char* formatName = "Unknown";
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) formatName = "BGRA8_UNORM";
		else if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) formatName = "RGBA8_UNORM";
		else if (formats[i].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) formatName = "RGB10A2_UNORM";

		const char* colorSpaceName = "SRGB";
		if (formats[i].colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT) colorSpaceName = "Display_P3";
		else if (formats[i].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) colorSpaceName = "HDR10_ST2084";
		else if (formats[i].colorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT) colorSpaceName = "HDR10_HLG";
		else if (formats[i].colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) colorSpaceName = "ExtSRGB_Linear";

		snprintf(lines[lineCount++], 256, "  %s + %s", formatName, colorSpaceName);
	}

	lines[lineCount][0] = '\0'; lineCount++;
	snprintf(lines[lineCount++], 256, "PRESENT MODES (%u)", presentModeCount);
	for (uint32_t i = 0; i < presentModeCount && lineCount < 58; i++) {
		const char* modeName = "Unknown";
		if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) modeName = "IMMEDIATE";
		else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) modeName = "MAILBOX";
		else if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) modeName = "FIFO";
		else if (presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) modeName = "FIFO_RELAXED";
		snprintf(lines[lineCount++], 256, "  %s", modeName);
	}

	lines[lineCount][0] = '\0'; lineCount++;
	snprintf(lines[lineCount++], 256, "ACTIVE SWAPCHAIN");
	const char* activeFormat = "Other";
	if (winCtx->swapchainImageFormat == VK_FORMAT_B8G8R8A8_UNORM) activeFormat = "BGRA8 UNORM";
	else if (winCtx->swapchainImageFormat == VK_FORMAT_R8G8B8A8_UNORM) activeFormat = "RGBA8 UNORM";
	else if (winCtx->swapchainImageFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32) activeFormat = "RGB10A2 UNORM";
	snprintf(lines[lineCount++], 256, "Format: %s", activeFormat);

	const char* activeColorSpace = "sRGB";
	if (winCtx->swapchainColorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT) {
		activeColorSpace = "Display P3";
	} else if (winCtx->swapchainColorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
		activeColorSpace = "HDR10 ST.2084 (PQ)";
	} else if (winCtx->swapchainColorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT) {
		activeColorSpace = "HDR10 HLG";
	} else if (winCtx->swapchainColorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
		activeColorSpace = "Extended sRGB Linear";
	}
	snprintf(lines[lineCount++], 256, "Color Space: %s", activeColorSpace);

	// Print runtime query results to console
	printf("Runtime Vulkan Queries:\n");
	printf("  Surface Formats: %u available\n", formatCount);
	printf("  Present Modes: %u available\n", presentModeCount);
	printf("  Queue Families: %u\n", queueFamilyCount);
	printf("  Swapchain Images: %u-%u\n", surfaceCaps.minImageCount,
	       surfaceCaps.maxImageCount ? surfaceCaps.maxImageCount : 999);
	printf("\n");

	// Free allocated query memory
	free(formats);
	free(presentModes);
	free(queueFamilies);

	// Get NanoVG's command buffer
	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);

	// Render loop
	int frameCount = 0;
	while (!window_should_close(winCtx) && frameCount++ < 60) {
		window_poll_events();

		// Reset and begin NanoVG's command buffer
		vkResetCommandBuffer(cmd, 0);
		VkCommandBufferBeginInfo beginInfo = {0};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmd, &beginInfo);

		// Acquire swapchain image
		uint32_t imageIndex;
		vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
		                      VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

		// Begin render pass
		VkClearValue clearValues[2];
		clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.15f, 1.0f}};
		clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo = {0};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = winCtx->renderPass;
		renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
		renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {0, 0, (float)winCtx->swapchainExtent.width,
		                       (float)winCtx->swapchainExtent.height, 0, 1};
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);
		nvgBeginFrame(vg, (float)winCtx->swapchainExtent.width,
		              (float)winCtx->swapchainExtent.height, 1.0f);

		// Render all info lines
		float y = 40;
		for (int i = 0; i < lineCount; i++) {
			int isBold = (strstr(lines[i], "VULKAN") || strstr(lines[i], "DISPLAY") ||
			              strstr(lines[i], "MEMORY") || strstr(lines[i], "SWAPCHAIN"));
			render_text_line(vg, &y, lines[i], isBold);
		}

		// Instructions
		y = (float)winCtx->swapchainExtent.height - 40;
		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		nvgText(vg, 20, y, "Press ESC or close window to exit", NULL);

		nvgEndFrame(vg);
		vkCmdEndRenderPass(cmd);
		vkEndCommandBuffer(cmd);

		// Submit commands
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;
		vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(winCtx->graphicsQueue);

		// Present
		VkPresentInfoKHR presentInfo = {0};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &winCtx->swapchain;
		presentInfo.pImageIndices = &imageIndex;
		vkQueuePresentKHR(winCtx->presentQueue, &presentInfo);
	}

	vkDeviceWaitIdle(winCtx->device);

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("Test completed successfully\n");
	return 0;
}
