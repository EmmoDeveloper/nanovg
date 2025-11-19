#include "nvg_vk_hdr_metadata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Check if VK_EXT_hdr_metadata is available
int nvgvk_has_hdr_metadata_extension(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);

	if (extensionCount == 0) return 0;

	VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
	if (!extensions) return 0;

	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions);

	int found = 0;
	for (uint32_t i = 0; i < extensionCount; i++) {
		if (strcmp(extensions[i].extensionName, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
			found = 1;
			break;
		}
	}

	free(extensions);
	return found;
}

// Query HDR capabilities from physical device and surface
int nvgvk_query_hdr_capabilities(VkPhysicalDevice physicalDevice,
                                  VkSurfaceKHR surface,
                                  VkHdrMetadataEXT* metadata)
{
	if (!metadata) return 0;

	// Check if extension is available
	if (!nvgvk_has_hdr_metadata_extension(physicalDevice)) {
		return 0;
	}

	// Query surface capabilities for HDR
	VkSurfaceCapabilities2KHR surfaceCapabilities2 = {0};
	surfaceCapabilities2.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;

	VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {0};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
	surfaceInfo.surface = surface;

	// Try to get surface capabilities (may not be supported on all platforms)
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCapabilities2);
	if (result != VK_SUCCESS) {
		// Fallback: set default HDR10 metadata
		memset(metadata, 0, sizeof(VkHdrMetadataEXT));
		metadata->sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;

		// HDR10 standard primaries (BT.2020)
		metadata->displayPrimaryRed.x = 0.708f;
		metadata->displayPrimaryRed.y = 0.292f;
		metadata->displayPrimaryGreen.x = 0.170f;
		metadata->displayPrimaryGreen.y = 0.797f;
		metadata->displayPrimaryBlue.x = 0.131f;
		metadata->displayPrimaryBlue.y = 0.046f;
		metadata->whitePoint.x = 0.3127f;
		metadata->whitePoint.y = 0.3290f;

		// HDR10 luminance range
		metadata->maxLuminance = 1000.0f;  // cd/m² (nits)
		metadata->minLuminance = 0.0001f;  // cd/m²
		metadata->maxContentLightLevel = 1000.0f;
		metadata->maxFrameAverageLightLevel = 400.0f;

		return 1;  // Return default metadata
	}

	// TODO: Parse actual display capabilities if available
	// For now, use HDR10 defaults
	memset(metadata, 0, sizeof(VkHdrMetadataEXT));
	metadata->sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;

	// HDR10 standard
	metadata->displayPrimaryRed.x = 0.708f;
	metadata->displayPrimaryRed.y = 0.292f;
	metadata->displayPrimaryGreen.x = 0.170f;
	metadata->displayPrimaryGreen.y = 0.797f;
	metadata->displayPrimaryBlue.x = 0.131f;
	metadata->displayPrimaryBlue.y = 0.046f;
	metadata->whitePoint.x = 0.3127f;
	metadata->whitePoint.y = 0.3290f;

	metadata->maxLuminance = 1000.0f;
	metadata->minLuminance = 0.0001f;
	metadata->maxContentLightLevel = 1000.0f;
	metadata->maxFrameAverageLightLevel = 400.0f;

	return 1;
}

// Set HDR metadata on swapchain
void nvgvk_set_hdr_metadata(VkDevice device,
                             VkSwapchainKHR swapchain,
                             const VkHdrMetadataEXT* metadata)
{
	if (!device || !swapchain || !metadata) return;

	// Get function pointer (extension function)
	PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT =
		(PFN_vkSetHdrMetadataEXT)vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");

	if (!vkSetHdrMetadataEXT) {
		fprintf(stderr, "vkSetHdrMetadataEXT not available\n");
		return;
	}

	vkSetHdrMetadataEXT(device, 1, &swapchain, metadata);
}
