#ifndef NVG_VK_HDR_METADATA_H
#define NVG_VK_HDR_METADATA_H

#include <vulkan/vulkan.h>

// Query HDR capabilities from physical device and surface
// Returns 1 if HDR is supported, 0 otherwise
// If supported, fills metadata with display capabilities
int nvgvk_query_hdr_capabilities(VkPhysicalDevice physicalDevice,
                                  VkSurfaceKHR surface,
                                  VkHdrMetadataEXT* metadata);

// Set HDR metadata on swapchain
// Requires VK_EXT_hdr_metadata extension
void nvgvk_set_hdr_metadata(VkDevice device,
                             VkSwapchainKHR swapchain,
                             const VkHdrMetadataEXT* metadata);

// Check if VK_EXT_hdr_metadata is available
int nvgvk_has_hdr_metadata_extension(VkPhysicalDevice physicalDevice);

#endif // NVG_VK_HDR_METADATA_H
