#ifndef NVG_VK_DISPLAY_REAL_H
#define NVG_VK_DISPLAY_REAL_H

#include "nvg_vk_display_color_space.h"
#include "../tools/display_detection.h"
#include <vulkan/vulkan.h>

// Real display with physical characteristics
typedef struct NVGVkRealDisplay {
	// Display manager (quaternion color space system)
	NVGVkDisplayColorManager* colorManager;

	// Physical display info
	DisplayDetectionInfo physicalInfo;

	// Vulkan surface capabilities
	VkSurfaceCapabilitiesKHR surfaceCapabilities;

	// Available surface formats
	VkSurfaceFormatKHR* surfaceFormats;
	uint32_t surfaceFormatCount;

	// Active configuration
	VkSurfaceFormatKHR activeFormat;
	VkPresentModeKHR activePresentMode;

	// Computed display characteristics
	float dpi;
	float pixelDensity;		// pixels per mm
	float diagonalInches;

	// Color space mapping
	VkColorSpaceKHR recommendedColorSpace;
	int supportsHDR;
	int supportsWideGamut;

	// Subpixel rendering
	int subpixelRenderingEnabled;
} NVGVkRealDisplay;

// Create real display from Vulkan physical device and surface
NVGVkRealDisplay* nvgvk_real_display_create(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface
);

// Destroy real display
void nvgvk_real_display_destroy(NVGVkRealDisplay* display);

// Detect physical display characteristics
int nvgvk_real_display_detect_physical(NVGVkRealDisplay* display);

// Query Vulkan surface capabilities
int nvgvk_real_display_query_surface(
	NVGVkRealDisplay* display,
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface
);

// Choose optimal color space based on display capabilities
VkColorSpaceKHR nvgvk_real_display_choose_color_space(
	const NVGVkRealDisplay* display,
	int preferHDR,
	int preferWideGamut
);

// Choose optimal surface format
VkSurfaceFormatKHR nvgvk_real_display_choose_format(
	const NVGVkRealDisplay* display,
	VkColorSpaceKHR preferredColorSpace
);

// Update color space transform based on current configuration
void nvgvk_real_display_update_transform(
	NVGVkRealDisplay* display,
	VkColorSpaceKHR sourceColorSpace
);

// Get current color space transform
const NVGVkColorSpaceTransform* nvgvk_real_display_get_transform(
	const NVGVkRealDisplay* display
);

// Print detailed display information
void nvgvk_real_display_print_info(const NVGVkRealDisplay* display);

// Compute DPI from physical dimensions
float nvgvk_real_display_compute_dpi(
	int widthPixels,
	int heightPixels,
	int widthMM,
	int heightMM
);

// Determine if display supports HDR
int nvgvk_real_display_supports_hdr(const NVGVkRealDisplay* display);

// Determine if display supports wide gamut (P3, BT.2020)
int nvgvk_real_display_supports_wide_gamut(const NVGVkRealDisplay* display);

// Map display subpixel layout to rendering mode
int nvgvk_real_display_get_subpixel_mode(const NVGVkRealDisplay* display);

#endif // NVG_VK_DISPLAY_REAL_H
