#ifndef NVG_VK_COLOR_SPACE_H
#define NVG_VK_COLOR_SPACE_H

#include <vulkan/vulkan.h>
#include "../nanovg.h"
#include "khr_df_enums.h"
#include "nvg_vk_color_space_math.h"

// Transfer function ID for shader-based conversion
typedef enum {
	NVGVK_TRANSFER_LINEAR = 0,
	NVGVK_TRANSFER_SRGB = 1,
	NVGVK_TRANSFER_ITU = 2,		// BT.709/601
	NVGVK_TRANSFER_PQ = 3,		// ST.2084
	NVGVK_TRANSFER_HLG = 4,		// Hybrid Log-Gamma
	NVGVK_TRANSFER_DCI_P3 = 5,	// Gamma 2.6
	NVGVK_TRANSFER_ADOBE_RGB = 6	// Gamma 2.2
} NVGVkTransferFunctionID;

// Color space descriptor based on Khronos Data Format Specification
typedef struct NVGVkColorSpaceDesc {
	VkColorSpaceKHR vkColorSpace;		// Vulkan color space enum
	khr_df_primaries_e primaries;		// Color primaries
	khr_df_transfer_e transfer;		// Transfer function (OETF/EOTF)
	khr_df_model_e model;			// Color model
	const char* name;			// Human-readable name
	const char* standard;			// Standard reference (e.g., "ITU-R BT.709")
	float maxLuminance;			// Max luminance in cd/m² (nits)
	int isHDR;				// 1 if HDR, 0 if SDR
	int isLinear;				// 1 if linear, 0 if gamma-corrected

	// Transformation data
	NVGVkTransferFunctionID transferID;	// Transfer function for shader
	const NVGVkColorPrimaries* colorPrimaries;  // Primaries definition
	NVGVkMat3 toXYZ;			// RGB → CIE XYZ
	NVGVkMat3 fromXYZ;			// CIE XYZ → RGB
} NVGVkColorSpaceDesc;

// Color space conversion path (runtime configuration)
typedef struct NVGVkColorConversionPath {
	NVGVkMat3 gamutMatrix;			// Primaries conversion matrix
	NVGVkTransferFunctionID srcTransferID;	// Source transfer function
	NVGVkTransferFunctionID dstTransferID;	// Destination transfer function
	float hdrScale;				// HDR luminance scaling
} NVGVkColorConversionPath;

// Color space configuration
typedef struct NVGVkColorSpace {
	// Swapchain configuration
	VkFormat swapchainFormat;
	VkColorSpaceKHR swapchainColorSpace;
	const NVGVkColorSpaceDesc* desc;

	// Texture format mapping
	VkFormat alphaFormat;			// NVG_TEXTURE_ALPHA
	VkFormat rgbaFormat;			// NVG_TEXTURE_RGBA
	VkFormat msdfFormat;			// NVG_TEXTURE_MSDF

	// Depth/stencil
	VkFormat depthStencilFormat;

	// Runtime color space conversion parameters
	NVGVkColorConversionPath conversionPath;
	float hdrScaleOverride;			// User-specified HDR scale (0 = use default)
	int useGamutMapping;			// 0=clip, 1=soft map
	int useToneMapping;			// 0=linear scale, 1=apply tone mapping

	// Source color space (for input colors)
	VkColorSpaceKHR sourceColorSpace;
	const NVGVkColorSpaceDesc* sourceDesc;
} NVGVkColorSpace;

// Create color space with sRGB defaults
NVGVkColorSpace* nvgvk_color_space_create(void);

// Create from Vulkan color space enum
NVGVkColorSpace* nvgvk_color_space_create_from_vk(VkColorSpaceKHR vkColorSpace);

// Create from physical device and surface (auto-detect)
NVGVkColorSpace* nvgvk_color_space_create_from_surface(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface
);

// Create from surface with preferred color space
NVGVkColorSpace* nvgvk_color_space_create_from_surface_with_preference(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkColorSpaceKHR preferredColorSpace
);

// Destroy color space
void nvgvk_color_space_destroy(NVGVkColorSpace* cs);

// Get Vulkan format for NanoVG texture type
VkFormat nvgvk_color_space_get_format(const NVGVkColorSpace* cs, int nvgTextureType);

// Get bytes per pixel
int nvgvk_color_space_get_bytes_per_pixel(const NVGVkColorSpace* cs, int nvgTextureType);

// Get color space descriptor
const NVGVkColorSpaceDesc* nvgvk_color_space_get_desc(const NVGVkColorSpace* cs);

// Query: Is HDR?
int nvgvk_color_space_is_hdr(const NVGVkColorSpace* cs);

// Query: Is linear?
int nvgvk_color_space_is_linear(const NVGVkColorSpace* cs);

// Query: Is sRGB format?
int nvgvk_color_space_is_srgb_format(VkFormat format);

// Get descriptor for Vulkan color space
const NVGVkColorSpaceDesc* nvgvk_color_space_get_vk_desc(VkColorSpaceKHR vkColorSpace);

// Choose best surface format
VkSurfaceFormatKHR nvgvk_color_space_choose_surface_format(
	const VkSurfaceFormatKHR* availableFormats,
	uint32_t formatCount,
	VkColorSpaceKHR preferredColorSpace
);

// Query available color spaces
int nvgvk_color_space_query_available(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkColorSpaceKHR* outColorSpaces,
	uint32_t* outCount
);

// Get human-readable name
const char* nvgvk_color_space_get_name(VkColorSpaceKHR vkColorSpace);

// Get standard reference
const char* nvgvk_color_space_get_standard(VkColorSpaceKHR vkColorSpace);

// Get maximum luminance in cd/m²
float nvgvk_color_space_get_max_luminance(const NVGVkColorSpace* cs);

// Print color space information (for debugging)
void nvgvk_color_space_print_info(const NVGVkColorSpace* cs);

// List all available color spaces for a surface
int nvgvk_color_space_print_available(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

// Build conversion path from source to destination color space
// This can be used to set up shader uniforms for color space conversion
// Build conversion path between source and destination color spaces
void nvgvk_color_space_build_conversion(const NVGVkColorSpace* src,
                                          const NVGVkColorSpace* dst,
                                          NVGVkColorConversionPath* path);

// Set source color space (input colors interpretation)
void nvgvk_set_source_color_space(NVGVkColorSpace* cs, int srcSpace);

// Set destination color space (output framebuffer)
void nvgvk_set_destination_color_space(NVGVkColorSpace* cs, int dstSpace);

// Set custom color space using transfer functions and primaries
void nvgvk_set_custom_color_space(NVGVkColorSpace* cs, int srcTransfer, int dstTransfer,
                                   int srcPrimaries, int dstPrimaries);

// Set HDR scaling factor
void nvgvk_set_hdr_scale(NVGVkColorSpace* cs, float scale);

// Enable/disable soft gamut mapping
void nvgvk_set_gamut_mapping(NVGVkColorSpace* cs, int enabled);

// Enable/disable tone mapping
void nvgvk_set_tone_mapping(NVGVkColorSpace* cs, int enabled);

#endif // NVG_VK_COLOR_SPACE_H
