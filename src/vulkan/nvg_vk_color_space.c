#include "nvg_vk_color_space.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Color space descriptor table - maps every VK_COLOR_SPACE_* to Khronos Data Format enums
static const NVGVkColorSpaceDesc colorSpaceDescriptors[] = {
	// VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	{
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		KHR_DF_PRIMARIES_BT709,
		KHR_DF_TRANSFER_SRGB,
		KHR_DF_MODEL_RGBSDA,
		"sRGB Non-linear",
		"IEC 61966-2-1 sRGB",
		80.0f,		// Typical SDR display
		0,		// SDR
		0		// Non-linear
	},
	// VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT
	{
		VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
		KHR_DF_PRIMARIES_DISPLAYP3,
		KHR_DF_TRANSFER_SRGB,
		KHR_DF_MODEL_RGBSDA,
		"Display P3 Non-linear",
		"SMPTE EG 432-1 Display P3",
		80.0f,
		0,
		0
	},
	// VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT
	{
		VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
		KHR_DF_PRIMARIES_BT709,
		KHR_DF_TRANSFER_LINEAR,
		KHR_DF_MODEL_RGBSDA,
		"Extended sRGB Linear",
		"IEC 61966-2-1 extended sRGB (linear)",
		80.0f,
		0,
		1		// Linear
	},
	// VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT
	{
		VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT,
		KHR_DF_PRIMARIES_DISPLAYP3,
		KHR_DF_TRANSFER_LINEAR,
		KHR_DF_MODEL_RGBSDA,
		"Display P3 Linear",
		"SMPTE EG 432-1 Display P3 (linear)",
		80.0f,
		0,
		1
	},
	// VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT
	{
		VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT,
		KHR_DF_PRIMARIES_DISPLAYP3,
		KHR_DF_TRANSFER_DCIP3,
		KHR_DF_MODEL_RGBSDA,
		"DCI-P3 Non-linear",
		"SMPTE RP 431-2 DCI P3",
		48.0f,		// DCI projector calibration
		0,
		0
	},
	// VK_COLOR_SPACE_BT709_LINEAR_EXT
	{
		VK_COLOR_SPACE_BT709_LINEAR_EXT,
		KHR_DF_PRIMARIES_BT709,
		KHR_DF_TRANSFER_LINEAR,
		KHR_DF_MODEL_RGBSDA,
		"BT.709 Linear",
		"ITU-R BT.709 (linear)",
		80.0f,
		0,
		1
	},
	// VK_COLOR_SPACE_BT709_NONLINEAR_EXT
	{
		VK_COLOR_SPACE_BT709_NONLINEAR_EXT,
		KHR_DF_PRIMARIES_BT709,
		KHR_DF_TRANSFER_ITU,
		KHR_DF_MODEL_RGBSDA,
		"BT.709 Non-linear",
		"ITU-R BT.709",
		80.0f,
		0,
		0
	},
	// VK_COLOR_SPACE_BT2020_LINEAR_EXT
	{
		VK_COLOR_SPACE_BT2020_LINEAR_EXT,
		KHR_DF_PRIMARIES_BT2020,
		KHR_DF_TRANSFER_LINEAR,
		KHR_DF_MODEL_RGBSDA,
		"BT.2020 Linear",
		"ITU-R BT.2020 (linear)",
		1000.0f,	// Can support HDR
		0,
		1
	},
	// VK_COLOR_SPACE_HDR10_ST2084_EXT
	{
		VK_COLOR_SPACE_HDR10_ST2084_EXT,
		KHR_DF_PRIMARIES_BT2020,
		KHR_DF_TRANSFER_PQ_EOTF,
		KHR_DF_MODEL_RGBSDA,
		"HDR10 ST.2084 PQ",
		"ITU-R BT.2100 PQ (SMPTE ST 2084)",
		10000.0f,	// HDR10: 10,000 cd/m²
		1,		// HDR
		0
	},
	// VK_COLOR_SPACE_DOLBYVISION_EXT (deprecated)
	{
		VK_COLOR_SPACE_DOLBYVISION_EXT,
		KHR_DF_PRIMARIES_BT2020,
		KHR_DF_TRANSFER_PQ_EOTF,
		KHR_DF_MODEL_RGBSDA,
		"Dolby Vision (deprecated)",
		"Dolby Vision",
		10000.0f,
		1,
		0
	},
	// VK_COLOR_SPACE_HDR10_HLG_EXT
	{
		VK_COLOR_SPACE_HDR10_HLG_EXT,
		KHR_DF_PRIMARIES_BT2020,
		KHR_DF_TRANSFER_HLG_EOTF,
		KHR_DF_MODEL_RGBSDA,
		"HDR10 HLG",
		"ITU-R BT.2100 HLG",
		1000.0f,	// HLG: 1,000 cd/m²
		1,
		0
	},
	// VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT
	{
		VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT,
		KHR_DF_PRIMARIES_ADOBERGB,
		KHR_DF_TRANSFER_LINEAR,
		KHR_DF_MODEL_RGBSDA,
		"Adobe RGB Linear",
		"Adobe RGB (1998) linear",
		80.0f,
		0,
		1
	},
	// VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT
	{
		VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT,
		KHR_DF_PRIMARIES_ADOBERGB,
		KHR_DF_TRANSFER_ADOBERGB,
		KHR_DF_MODEL_RGBSDA,
		"Adobe RGB Non-linear",
		"Adobe RGB (1998)",
		80.0f,
		0,
		0
	},
	// VK_COLOR_SPACE_PASS_THROUGH_EXT
	{
		VK_COLOR_SPACE_PASS_THROUGH_EXT,
		KHR_DF_PRIMARIES_UNSPECIFIED,
		KHR_DF_TRANSFER_UNSPECIFIED,
		KHR_DF_MODEL_UNSPECIFIED,
		"Pass-through",
		"No color space conversion",
		0.0f,
		0,
		1		// Treated as linear
	},
	// VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT
	{
		VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT,
		KHR_DF_PRIMARIES_BT709,
		KHR_DF_TRANSFER_SRGB,
		KHR_DF_MODEL_RGBSDA,
		"Extended sRGB Non-linear (scRGB)",
		"IEC 61966-2-2 scRGB",
		80.0f,		// Can represent HDR values
		0,
		0
	},
	// VK_COLOR_SPACE_DISPLAY_NATIVE_AMD
	{
		VK_COLOR_SPACE_DISPLAY_NATIVE_AMD,
		KHR_DF_PRIMARIES_UNSPECIFIED,
		KHR_DF_TRANSFER_UNSPECIFIED,
		KHR_DF_MODEL_UNSPECIFIED,
		"Display Native (AMD)",
		"AMD FreeSync2 native display",
		0.0f,
		0,
		0
	}
};

static const int numColorSpaceDescriptors = sizeof(colorSpaceDescriptors) / sizeof(colorSpaceDescriptors[0]);

const NVGVkColorSpaceDesc* nvgvk_color_space_get_vk_desc(VkColorSpaceKHR vkColorSpace)
{
	for (int i = 0; i < numColorSpaceDescriptors; i++) {
		if (colorSpaceDescriptors[i].vkColorSpace == vkColorSpace) {
			return &colorSpaceDescriptors[i];
		}
	}
	return &colorSpaceDescriptors[0];  // Default to sRGB
}

NVGVkColorSpace* nvgvk_color_space_create(void)
{
	return nvgvk_color_space_create_from_vk(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
}

NVGVkColorSpace* nvgvk_color_space_create_from_vk(VkColorSpaceKHR vkColorSpace)
{
	NVGVkColorSpace* cs = (NVGVkColorSpace*)calloc(1, sizeof(NVGVkColorSpace));
	if (!cs) return NULL;

	cs->desc = nvgvk_color_space_get_vk_desc(vkColorSpace);
	cs->swapchainColorSpace = vkColorSpace;
	cs->swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;  // Default

	// Texture formats
	cs->alphaFormat = VK_FORMAT_R8_UNORM;
	cs->rgbaFormat = VK_FORMAT_R8G8B8A8_UNORM;
	cs->msdfFormat = VK_FORMAT_R8G8B8A8_UNORM;

	// Depth/stencil
	cs->depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	return cs;
}

NVGVkColorSpace* nvgvk_color_space_create_from_surface(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface
)
{
	return nvgvk_color_space_create_from_surface_with_preference(
		physicalDevice, surface, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
}

NVGVkColorSpace* nvgvk_color_space_create_from_surface_with_preference(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkColorSpaceKHR preferredColorSpace
)
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
	if (formatCount == 0) {
		return nvgvk_color_space_create_from_vk(preferredColorSpace);
	}

	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

	VkSurfaceFormatKHR chosen = nvgvk_color_space_choose_surface_format(formats, formatCount, preferredColorSpace);
	free(formats);

	NVGVkColorSpace* cs = nvgvk_color_space_create_from_vk(chosen.colorSpace);
	cs->swapchainFormat = chosen.format;
	return cs;
}

void nvgvk_color_space_destroy(NVGVkColorSpace* cs)
{
	if (cs) {
		free(cs);
	}
}

VkFormat nvgvk_color_space_get_format(const NVGVkColorSpace* cs, int nvgTextureType)
{
	if (!cs) return VK_FORMAT_UNDEFINED;

	switch (nvgTextureType) {
		case NVG_TEXTURE_ALPHA:
			return cs->alphaFormat;
		case NVG_TEXTURE_RGBA:
			return cs->rgbaFormat;
		case NVG_TEXTURE_MSDF:
			return cs->msdfFormat;
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

int nvgvk_color_space_get_bytes_per_pixel(const NVGVkColorSpace* cs, int nvgTextureType)
{
	if (!cs) return 0;

	switch (nvgTextureType) {
		case NVG_TEXTURE_ALPHA:
			return 1;
		case NVG_TEXTURE_RGBA:
		case NVG_TEXTURE_MSDF:
			return 4;
		default:
			return 0;
	}
}

const NVGVkColorSpaceDesc* nvgvk_color_space_get_desc(const NVGVkColorSpace* cs)
{
	return cs ? cs->desc : NULL;
}

int nvgvk_color_space_is_hdr(const NVGVkColorSpace* cs)
{
	return cs && cs->desc ? cs->desc->isHDR : 0;
}

int nvgvk_color_space_is_linear(const NVGVkColorSpace* cs)
{
	return cs && cs->desc ? cs->desc->isLinear : 0;
}

int nvgvk_color_space_is_srgb_format(VkFormat format)
{
	switch (format) {
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
			return 1;
		default:
			return 0;
	}
}

VkSurfaceFormatKHR nvgvk_color_space_choose_surface_format(
	const VkSurfaceFormatKHR* availableFormats,
	uint32_t formatCount,
	VkColorSpaceKHR preferredColorSpace
)
{
	if (formatCount == 0) {
		VkSurfaceFormatKHR defaultFormat;
		defaultFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		defaultFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		return defaultFormat;
	}

	// Try to find exact match
	for (uint32_t i = 0; i < formatCount; i++) {
		if (availableFormats[i].colorSpace == preferredColorSpace &&
			(availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
			 availableFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM)) {
			return availableFormats[i];
		}
	}

	// Fallback: find any with preferred color space
	for (uint32_t i = 0; i < formatCount; i++) {
		if (availableFormats[i].colorSpace == preferredColorSpace) {
			return availableFormats[i];
		}
	}

	// Last resort: use first available
	return availableFormats[0];
}

int nvgvk_color_space_query_available(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkColorSpaceKHR* outColorSpaces,
	uint32_t* outCount
)
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
	if (formatCount == 0) {
		*outCount = 0;
		return 0;
	}

	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

	uint32_t uniqueCount = 0;
	for (uint32_t i = 0; i < formatCount; i++) {
		int isDuplicate = 0;
		for (uint32_t j = 0; j < uniqueCount; j++) {
			if (outColorSpaces[j] == formats[i].colorSpace) {
				isDuplicate = 1;
				break;
			}
		}
		if (!isDuplicate && uniqueCount < *outCount) {
			outColorSpaces[uniqueCount++] = formats[i].colorSpace;
		}
	}

	free(formats);
	*outCount = uniqueCount;
	return 1;
}

const char* nvgvk_color_space_get_name(VkColorSpaceKHR vkColorSpace)
{
	const NVGVkColorSpaceDesc* desc = nvgvk_color_space_get_vk_desc(vkColorSpace);
	return desc ? desc->name : "Unknown";
}

const char* nvgvk_color_space_get_standard(VkColorSpaceKHR vkColorSpace)
{
	const NVGVkColorSpaceDesc* desc = nvgvk_color_space_get_vk_desc(vkColorSpace);
	return desc ? desc->standard : "Unknown";
}

float nvgvk_color_space_get_max_luminance(const NVGVkColorSpace* cs)
{
	return cs && cs->desc ? cs->desc->maxLuminance : 0.0f;
}

void nvgvk_color_space_print_info(const NVGVkColorSpace* cs)
{
	if (!cs || !cs->desc) {
		printf("Color Space: [Invalid]\n");
		return;
	}

	const NVGVkColorSpaceDesc* desc = cs->desc;
	printf("Color Space Information:\n");
	printf("  Name: %s\n", desc->name);
	printf("  Standard: %s\n", desc->standard);
	printf("  Type: %s\n", desc->isHDR ? "HDR" : "SDR");
	printf("  Transfer: %s\n", desc->isLinear ? "Linear" : "Non-linear");
	printf("  Max Luminance: %.0f cd/m²\n", desc->maxLuminance);
	printf("  Primaries: ");
	switch (desc->primaries) {
		case KHR_DF_PRIMARIES_BT709: printf("BT.709/sRGB\n"); break;
		case KHR_DF_PRIMARIES_BT2020: printf("BT.2020\n"); break;
		case KHR_DF_PRIMARIES_DISPLAYP3: printf("Display P3\n"); break;
		case KHR_DF_PRIMARIES_ADOBERGB: printf("Adobe RGB\n"); break;
		default: printf("Other (%d)\n", desc->primaries); break;
	}
	printf("  Transfer Function: ");
	switch (desc->transfer) {
		case KHR_DF_TRANSFER_LINEAR: printf("Linear\n"); break;
		case KHR_DF_TRANSFER_SRGB: printf("sRGB\n"); break;
		case KHR_DF_TRANSFER_PQ_EOTF: printf("PQ (SMPTE ST.2084)\n"); break;
		case KHR_DF_TRANSFER_HLG_EOTF: printf("HLG\n"); break;
		case KHR_DF_TRANSFER_ITU: printf("ITU (BT.709/601)\n"); break;
		default: printf("Other (%d)\n", desc->transfer); break;
	}
}

int nvgvk_color_space_print_available(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	VkColorSpaceKHR colorSpaces[32];
	uint32_t count = 32;

	if (!nvgvk_color_space_query_available(physicalDevice, surface, colorSpaces, &count)) {
		printf("Failed to query available color spaces\n");
		return 0;
	}

	printf("Available Color Spaces (%u):\n", count);
	for (uint32_t i = 0; i < count; i++) {
		const NVGVkColorSpaceDesc* desc = nvgvk_color_space_get_vk_desc(colorSpaces[i]);
		if (desc) {
			printf("  %2u. %s", i + 1, desc->name);
			if (desc->isHDR) {
				printf(" (HDR, %.0f cd/m²)", desc->maxLuminance);
			}
			printf("\n");
			printf("      %s\n", desc->standard);
		}
	}

	return 1;
}

// Helper: Map Khronos transfer enum to shader transfer ID
static NVGVkTransferFunctionID nvgvk__map_transfer_to_id(khr_df_transfer_e transfer)
{
	switch (transfer) {
		case KHR_DF_TRANSFER_LINEAR:
			return NVGVK_TRANSFER_LINEAR;
		case KHR_DF_TRANSFER_SRGB:
			return NVGVK_TRANSFER_SRGB;
		case KHR_DF_TRANSFER_ITU:
		case KHR_DF_TRANSFER_BT1886:
			return NVGVK_TRANSFER_ITU;
		case KHR_DF_TRANSFER_PQ_EOTF:
		case KHR_DF_TRANSFER_PQ_OETF:
			return NVGVK_TRANSFER_PQ;
		case KHR_DF_TRANSFER_HLG_EOTF:
		case KHR_DF_TRANSFER_HLG_OETF:
			return NVGVK_TRANSFER_HLG;
		case KHR_DF_TRANSFER_DCIP3:
			return NVGVK_TRANSFER_DCI_P3;
		case KHR_DF_TRANSFER_ADOBERGB:
			return NVGVK_TRANSFER_ADOBE_RGB;
		default:
			return NVGVK_TRANSFER_LINEAR;
	}
}

// Helper: Map Khronos primaries enum to NVGVkColorPrimaries pointer
static const NVGVkColorPrimaries* nvgvk__map_primaries_to_struct(khr_df_primaries_e primaries)
{
	switch (primaries) {
		case KHR_DF_PRIMARIES_BT709:
			return &NVGVK_PRIMARIES_BT709;
		case KHR_DF_PRIMARIES_BT2020:
			return &NVGVK_PRIMARIES_BT2020;
		case KHR_DF_PRIMARIES_DISPLAYP3:
			return &NVGVK_PRIMARIES_DISPLAYP3;
		case KHR_DF_PRIMARIES_ADOBERGB:
			return &NVGVK_PRIMARIES_ADOBERGB;
		default:
			return &NVGVK_PRIMARIES_BT709;
	}
}

void nvgvk_color_space_build_conversion(const NVGVkColorSpace* src,
                                        const NVGVkColorSpace* dst,
                                        NVGVkColorConversionPath* path)
{
	if (!src || !dst || !path) {
		return;
	}

	const NVGVkColorSpaceDesc* srcDesc = src->desc;
	const NVGVkColorSpaceDesc* dstDesc = dst->desc;

	if (!srcDesc || !dstDesc) {
		return;
	}

	// Step 1: Map transfer functions to shader IDs
	path->srcTransferID = nvgvk__map_transfer_to_id(srcDesc->transfer);
	path->dstTransferID = nvgvk__map_transfer_to_id(dstDesc->transfer);

	// Step 2: Get primaries structures
	const NVGVkColorPrimaries* srcPrimaries = nvgvk__map_primaries_to_struct(srcDesc->primaries);
	const NVGVkColorPrimaries* dstPrimaries = nvgvk__map_primaries_to_struct(dstDesc->primaries);

	// Step 3: Compute gamut conversion matrix
	nvgvk_primaries_conversion_matrix(srcPrimaries, dstPrimaries, &path->gamutMatrix);

	// Step 4: Calculate HDR scaling factor
	if (srcDesc->maxLuminance > 0.0f && dstDesc->maxLuminance > 0.0f) {
		path->hdrScale = dstDesc->maxLuminance / srcDesc->maxLuminance;
	} else {
		path->hdrScale = 1.0f;
	}
}

// Helper: Map NVGcolorSpace enum to VkColorSpaceKHR
static VkColorSpaceKHR nvgvk__map_color_space_enum(int nvgColorSpace)
{
	switch (nvgColorSpace) {
		case 0: // NVG_COLOR_SPACE_SRGB
			return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		case 1: // NVG_COLOR_SPACE_LINEAR_SRGB
			return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
		case 2: // NVG_COLOR_SPACE_DISPLAY_P3
			return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
		case 3: // NVG_COLOR_SPACE_REC2020
			return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
		case 4: // NVG_COLOR_SPACE_ADOBE_RGB
			return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
		case 5: // NVG_COLOR_SPACE_SCRGB
			return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
		case 6: // NVG_COLOR_SPACE_HDR10
			return VK_COLOR_SPACE_HDR10_ST2084_EXT;
		default:
			return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}
}

// Helper: Map NVGtransferFunction enum to NVGVkTransferFunctionID
static NVGVkTransferFunctionID nvgvk__map_nvg_transfer(int nvgTransfer)
{
	switch (nvgTransfer) {
		case 0: // NVG_TRANSFER_LINEAR
			return NVGVK_TRANSFER_LINEAR;
		case 1: // NVG_TRANSFER_SRGB
			return NVGVK_TRANSFER_SRGB;
		case 2: // NVG_TRANSFER_GAMMA22
			return NVGVK_TRANSFER_ITU;  // Similar to gamma 2.2
		case 3: // NVG_TRANSFER_GAMMA28
			return NVGVK_TRANSFER_ADOBE_RGB;  // Similar to gamma 2.8
		case 4: // NVG_TRANSFER_REC709
			return NVGVK_TRANSFER_ITU;
		case 5: // NVG_TRANSFER_PQ
			return NVGVK_TRANSFER_PQ;
		case 6: // NVG_TRANSFER_HLG
			return NVGVK_TRANSFER_HLG;
		default:
			return NVGVK_TRANSFER_SRGB;
	}
}

// Helper: Extract primaries from NVGcolorSpace enum
static const NVGVkColorPrimaries* nvgvk__get_primaries_from_nvg_space(int nvgColorSpace)
{
	switch (nvgColorSpace) {
		case 0: // NVG_COLOR_SPACE_SRGB
		case 1: // NVG_COLOR_SPACE_LINEAR_SRGB
		case 5: // NVG_COLOR_SPACE_SCRGB
			return &NVGVK_PRIMARIES_BT709;
		case 2: // NVG_COLOR_SPACE_DISPLAY_P3
			return &NVGVK_PRIMARIES_DISPLAYP3;
		case 3: // NVG_COLOR_SPACE_REC2020
		case 6: // NVG_COLOR_SPACE_HDR10
			return &NVGVK_PRIMARIES_BT2020;
		case 4: // NVG_COLOR_SPACE_ADOBE_RGB
			return &NVGVK_PRIMARIES_ADOBERGB;
		default:
			return &NVGVK_PRIMARIES_BT709;
	}
}

void nvgvk_set_source_color_space(NVGVkColorSpace* cs, int srcSpace)
{
	if (!cs) return;

	VkColorSpaceKHR vkSpace = nvgvk__map_color_space_enum(srcSpace);
	const NVGVkColorSpaceDesc* desc = nvgvk_color_space_get_vk_desc(vkSpace);

	// Update source color space
	// Note: We store this as a modification to the color space object
	// The actual conversion will be computed when updating UBO
	cs->swapchainColorSpace = vkSpace;
	cs->desc = desc;
}

void nvgvk_set_destination_color_space(NVGVkColorSpace* cs, int dstSpace)
{
	if (!cs) return;

	VkColorSpaceKHR vkSpace = nvgvk__map_color_space_enum(dstSpace);
	const NVGVkColorSpaceDesc* desc = nvgvk_color_space_get_vk_desc(vkSpace);

	// Update destination color space
	cs->swapchainColorSpace = vkSpace;
	cs->desc = desc;
}

void nvgvk_set_custom_color_space(NVGVkColorSpace* cs, int srcTransfer, int dstTransfer,
                                   int srcPrimaries, int dstPrimaries)
{
	if (!cs) return;

	// For custom color spaces, we need to store the transfer functions and primaries
	// and compute the conversion matrix
	// This is more complex and requires extending the NVGVkColorSpace structure
	// For now, we'll use the primaries to select a base color space
	(void)srcTransfer;
	(void)dstTransfer;
	(void)srcPrimaries;
	(void)dstPrimaries;

	// TODO: Implement custom color space support
	// This would require storing source/dest transfer and primaries separately
}

void nvgvk_set_hdr_scale(NVGVkColorSpace* cs, float scale)
{
	if (!cs) return;
	cs->hdrScaleOverride = scale;
}

void nvgvk_set_gamut_mapping(NVGVkColorSpace* cs, int enabled)
{
	if (!cs) return;
	cs->useGamutMapping = enabled;
}

void nvgvk_set_tone_mapping(NVGVkColorSpace* cs, int enabled)
{
	if (!cs) return;
	cs->useToneMapping = enabled;
}
