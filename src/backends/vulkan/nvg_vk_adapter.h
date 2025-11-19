#ifndef NVG_VK_ADAPTER_H
#define NVG_VK_ADAPTER_H

#include "../../nanovg/nanovg_backend_types.h"
#include <vulkan/vulkan.h>

// Convert NanoVG color space to Vulkan color space
static inline VkColorSpaceKHR nvgvk_to_vk_color_space(NVGcolorSpace cs)
{
	// Handle sentinel value
	if (cs == NVG_COLOR_SPACE_UNDEFINED)
		return (VkColorSpaceKHR)-1;

	// Direct mapping (enum values are intentionally aligned)
	return (VkColorSpaceKHR)cs;
}

// Convert Vulkan color space to NanoVG color space
static inline NVGcolorSpace nvgvk_from_vk_color_space(VkColorSpaceKHR cs)
{
	// Handle sentinel value
	if (cs == (VkColorSpaceKHR)-1)
		return NVG_COLOR_SPACE_UNDEFINED;

	// Direct mapping
	return (NVGcolorSpace)cs;
}

// Convert NanoVG texture format to Vulkan format
static inline VkFormat nvgvk_to_vk_format(NVGtextureFormat fmt)
{
	// Direct mapping (enum values are intentionally aligned)
	return (VkFormat)fmt;
}

// Convert Vulkan format to NanoVG texture format
static inline NVGtextureFormat nvgvk_from_vk_format(VkFormat fmt)
{
	// Direct mapping
	return (NVGtextureFormat)fmt;
}

#endif // NVG_VK_ADAPTER_H
