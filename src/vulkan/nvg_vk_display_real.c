#include "nvg_vk_display_real.h"
#include "nvg_vk_color_space.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

float nvgvk_real_display_compute_dpi(
	int widthPixels,
	int heightPixels,
	int widthMM,
	int heightMM)
{
	if (widthMM <= 0 || heightMM <= 0) return 96.0f;

	// Compute diagonal in pixels
	float diagonalPixels = sqrtf((float)(widthPixels * widthPixels + heightPixels * heightPixels));

	// Compute diagonal in inches (1 inch = 25.4 mm)
	float diagonalMM = sqrtf((float)(widthMM * widthMM + heightMM * heightMM));
	float diagonalInches = diagonalMM / 25.4f;

	// DPI = diagonal pixels / diagonal inches
	return diagonalPixels / diagonalInches;
}

NVGVkRealDisplay* nvgvk_real_display_create(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface)
{
	NVGVkRealDisplay* display = (NVGVkRealDisplay*)calloc(1, sizeof(NVGVkRealDisplay));
	if (!display) return NULL;

	// Detect physical display characteristics
	nvgvk_real_display_detect_physical(display);

	// Query Vulkan surface capabilities
	nvgvk_real_display_query_surface(display, physicalDevice, surface);

	// Create color space manager
	display->colorManager = nvgvk_display_color_manager_create(physicalDevice, surface);
	if (!display->colorManager) {
		free(display->surfaceFormats);
		free(display);
		return NULL;
	}

	// Copy display info to color manager
	snprintf(display->colorManager->display.name,
	         sizeof(display->colorManager->display.name),
	         "%s", display->physicalInfo.name);

	// Compute DPI
	display->dpi = nvgvk_real_display_compute_dpi(
		display->physicalInfo.width,
		display->physicalInfo.height,
		display->physicalInfo.physicalWidthMM,
		display->physicalInfo.physicalHeightMM
	);

	if (display->physicalInfo.physicalWidthMM > 0 && display->physicalInfo.physicalHeightMM > 0) {
		float widthInches = display->physicalInfo.physicalWidthMM / 25.4f;
		float heightInches = display->physicalInfo.physicalHeightMM / 25.4f;
		display->diagonalInches = sqrtf(widthInches * widthInches + heightInches * heightInches);
		display->pixelDensity = display->physicalInfo.width / (float)display->physicalInfo.physicalWidthMM;
	}

	// Determine HDR support
	display->supportsHDR = nvgvk_real_display_supports_hdr(display);
	display->supportsWideGamut = nvgvk_real_display_supports_wide_gamut(display);

	// Choose recommended color space
	display->recommendedColorSpace = nvgvk_real_display_choose_color_space(display, 0, 0);

	// Enable subpixel rendering if supported
	display->subpixelRenderingEnabled =
		(display->physicalInfo.subpixel != DISPLAY_SUBPIXEL_NONE &&
		 display->physicalInfo.subpixel != DISPLAY_SUBPIXEL_UNKNOWN);

	return display;
}

void nvgvk_real_display_destroy(NVGVkRealDisplay* display)
{
	if (display) {
		if (display->colorManager) {
			nvgvk_display_color_manager_destroy(display->colorManager);
		}
		if (display->surfaceFormats) {
			free(display->surfaceFormats);
		}
		free(display);
	}
}

int nvgvk_real_display_detect_physical(NVGVkRealDisplay* display)
{
	if (!display) return 0;

	// Use display detection system
	if (detect_display_info(&display->physicalInfo) == 0) {
		printf("Warning: Failed to detect display info, using defaults\n");

		// Set defaults
		snprintf(display->physicalInfo.name, sizeof(display->physicalInfo.name), "Unknown Display");
		display->physicalInfo.width = 1920;
		display->physicalInfo.height = 1080;
		display->physicalInfo.refreshRate = 60;
		display->physicalInfo.physicalWidthMM = 508;  // ~24" 16:9
		display->physicalInfo.physicalHeightMM = 286;
		display->physicalInfo.subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_RGB;
		display->physicalInfo.scale = 1.0f;

		return 0;
	}

	return 1;
}

int nvgvk_real_display_query_surface(
	NVGVkRealDisplay* display,
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface)
{
	if (!display) return 0;

	// Get surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &display->surfaceCapabilities);

	// Get surface formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &display->surfaceFormatCount, NULL);

	if (display->surfaceFormatCount > 0) {
		display->surfaceFormats = (VkSurfaceFormatKHR*)malloc(
			sizeof(VkSurfaceFormatKHR) * display->surfaceFormatCount);

		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
			&display->surfaceFormatCount, display->surfaceFormats);
	}

	return 1;
}

int nvgvk_real_display_supports_hdr(const NVGVkRealDisplay* display)
{
	if (!display) return 0;

	// Check if any surface format supports HDR color spaces
	for (uint32_t i = 0; i < display->surfaceFormatCount; i++) {
		VkColorSpaceKHR cs = display->surfaceFormats[i].colorSpace;
		if (cs == VK_COLOR_SPACE_HDR10_ST2084_EXT ||
		    cs == VK_COLOR_SPACE_HDR10_HLG_EXT ||
		    cs == VK_COLOR_SPACE_DOLBYVISION_EXT) {
			return 1;
		}
	}

	return 0;
}

int nvgvk_real_display_supports_wide_gamut(const NVGVkRealDisplay* display)
{
	if (!display) return 0;

	// Check if any surface format supports wide gamut color spaces
	for (uint32_t i = 0; i < display->surfaceFormatCount; i++) {
		VkColorSpaceKHR cs = display->surfaceFormats[i].colorSpace;
		if (cs == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT ||
		    cs == VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT ||
		    cs == VK_COLOR_SPACE_BT2020_LINEAR_EXT ||
		    cs == VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT) {
			return 1;
		}
	}

	return 0;
}

VkColorSpaceKHR nvgvk_real_display_choose_color_space(
	const NVGVkRealDisplay* display,
	int preferHDR,
	int preferWideGamut)
{
	if (!display || !display->surfaceFormats) {
		return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}

	// Priority order based on preferences
	VkColorSpaceKHR priorities[16];
	int priorityCount = 0;

	if (preferHDR) {
		priorities[priorityCount++] = VK_COLOR_SPACE_HDR10_ST2084_EXT;
		priorities[priorityCount++] = VK_COLOR_SPACE_HDR10_HLG_EXT;
	}

	if (preferWideGamut) {
		priorities[priorityCount++] = VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
		priorities[priorityCount++] = VK_COLOR_SPACE_BT2020_LINEAR_EXT;
		priorities[priorityCount++] = VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
	}

	// Always add sRGB as fallback
	priorities[priorityCount++] = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	// Find first supported color space from priority list
	for (int p = 0; p < priorityCount; p++) {
		for (uint32_t i = 0; i < display->surfaceFormatCount; i++) {
			if (display->surfaceFormats[i].colorSpace == priorities[p]) {
				return priorities[p];
			}
		}
	}

	// Fallback: return first available
	return display->surfaceFormats[0].colorSpace;
}

VkSurfaceFormatKHR nvgvk_real_display_choose_format(
	const NVGVkRealDisplay* display,
	VkColorSpaceKHR preferredColorSpace)
{
	VkSurfaceFormatKHR defaultFormat = {
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	};

	if (!display || !display->surfaceFormats || display->surfaceFormatCount == 0) {
		return defaultFormat;
	}

	// Try to find exact match
	for (uint32_t i = 0; i < display->surfaceFormatCount; i++) {
		if (display->surfaceFormats[i].colorSpace == preferredColorSpace) {
			// Prefer UNORM or SRGB formats
			VkFormat fmt = display->surfaceFormats[i].format;
			if (fmt == VK_FORMAT_B8G8R8A8_UNORM ||
			    fmt == VK_FORMAT_R8G8B8A8_UNORM ||
			    fmt == VK_FORMAT_B8G8R8A8_SRGB ||
			    fmt == VK_FORMAT_R8G8B8A8_SRGB) {
				return display->surfaceFormats[i];
			}
		}
	}

	// Fallback: any format with preferred color space
	for (uint32_t i = 0; i < display->surfaceFormatCount; i++) {
		if (display->surfaceFormats[i].colorSpace == preferredColorSpace) {
			return display->surfaceFormats[i];
		}
	}

	// Last resort: first available
	return display->surfaceFormats[0];
}

void nvgvk_real_display_update_transform(
	NVGVkRealDisplay* display,
	VkColorSpaceKHR sourceColorSpace)
{
	if (!display || !display->colorManager) return;

	nvgvk_display_set_target_color_space(
		display->colorManager,
		display->activeFormat.colorSpace,
		sourceColorSpace
	);
}

const NVGVkColorSpaceTransform* nvgvk_real_display_get_transform(
	const NVGVkRealDisplay* display)
{
	if (!display || !display->colorManager) return NULL;
	return nvgvk_display_get_current_transform(display->colorManager);
}

int nvgvk_real_display_get_subpixel_mode(const NVGVkRealDisplay* display)
{
	if (!display || !display->subpixelRenderingEnabled) return 0;

	// Map display detection subpixel layout to rendering mode
	return display_subpixel_to_nvg(display->physicalInfo.subpixel);
}

void nvgvk_real_display_print_info(const NVGVkRealDisplay* display)
{
	if (!display) return;

	printf("=== Real Display Information ===\n");
	printf("Name: %s\n", display->physicalInfo.name);
	printf("Resolution: %dx%d @ %dHz\n",
	       display->physicalInfo.width,
	       display->physicalInfo.height,
	       display->physicalInfo.refreshRate);

	printf("Physical Size: %d x %d mm (%.1f\")\n",
	       display->physicalInfo.physicalWidthMM,
	       display->physicalInfo.physicalHeightMM,
	       display->diagonalInches);

	printf("DPI: %.1f\n", display->dpi);
	printf("Pixel Density: %.2f px/mm\n", display->pixelDensity);

	printf("Subpixel Layout: %s\n", display_subpixel_name(display->physicalInfo.subpixel));
	printf("Subpixel Rendering: %s\n", display->subpixelRenderingEnabled ? "Enabled" : "Disabled");

	printf("\nCapabilities:\n");
	printf("  HDR Support: %s\n", display->supportsHDR ? "Yes" : "No");
	printf("  Wide Gamut: %s\n", display->supportsWideGamut ? "Yes" : "No");

	printf("\nSurface Formats: %u available\n", display->surfaceFormatCount);
	for (uint32_t i = 0; i < display->surfaceFormatCount && i < 10; i++) {
		const char* csName = nvgvk_color_space_get_name(display->surfaceFormats[i].colorSpace);
		printf("  %2u. Format=%d, ColorSpace=%s\n",
		       i + 1,
		       display->surfaceFormats[i].format,
		       csName);
	}
	if (display->surfaceFormatCount > 10) {
		printf("  ... and %u more\n", display->surfaceFormatCount - 10);
	}

	printf("\nRecommended Color Space: %s\n",
	       nvgvk_color_space_get_name(display->recommendedColorSpace));

	if (display->colorManager) {
		printf("\nColor Space Manager:\n");
		nvgvk_display_print_capabilities(&display->colorManager->display);
	}

	const NVGVkColorSpaceTransform* transform = nvgvk_real_display_get_transform(display);
	if (transform) {
		printf("\nActive Transform:\n");
		nvgvk_transform_print_info(transform);
	}

	printf("================================\n");
}
