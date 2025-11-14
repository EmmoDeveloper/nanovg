#ifndef NVG_VK_DISPLAY_COLOR_SPACE_H
#define NVG_VK_DISPLAY_COLOR_SPACE_H

#include "nvg_vk_color_space_math.h"
#include "nvg_vk_color_space.h"
#include <vulkan/vulkan.h>

// Display characteristics and color space capabilities
typedef struct NVGVkDisplay {
	char name[256];					// Display name
	int isHDR;					// HDR capable
	float maxLuminance;				// Peak luminance (cd/m²)
	float minLuminance;				// Black level (cd/m²)
	float whitePoint[2];				// CIE xy chromaticity
	NVGVkColorPrimaries nativePrimaries;		// Native display primaries

	// Supported color spaces
	VkColorSpaceKHR supportedSpaces[16];
	int supportedSpaceCount;

	// Currently active color space
	VkColorSpaceKHR activeColorSpace;
} NVGVkDisplay;

// Color space transformation in quaternion space
// Represents the complete transformation: input -> display
typedef struct NVGVkColorSpaceTransform {
	// Decomposed matrix representation (quaternion + scale)
	NVGVkMat3Decomposed decomposed;			// Rotation (quat) + scale

	// Full matrix (cached for performance)
	NVGVkMat3 matrix;				// Full 3x3 gamut matrix

	// Transfer functions
	NVGVkTransferFunctionID srcTransfer;
	NVGVkTransferFunctionID dstTransfer;

	// HDR parameters
	float hdrScale;					// Luminance scaling factor
	float hdrMaxNits;				// Target max luminance

	// Source and destination
	const NVGVkColorPrimaries* srcPrimaries;
	const NVGVkColorPrimaries* dstPrimaries;
	VkColorSpaceKHR srcColorSpace;
	VkColorSpaceKHR dstColorSpace;

	// Validation
	float determinant;				// Matrix determinant (should be ~1.0)
	float orthogonalityError;			// Deviation from orthogonal
	int isValid;					// 1 if transform is valid
} NVGVkColorSpaceTransform;

// Color space graph node (for transform chains)
typedef struct NVGVkColorSpaceNode {
	VkColorSpaceKHR colorSpace;
	const NVGVkColorPrimaries* primaries;
	NVGVkTransferFunctionID transfer;

	// Transform to XYZ (canonical space)
	NVGVkMat3Decomposed toXYZ;
	NVGVkMat3Decomposed fromXYZ;
} NVGVkColorSpaceNode;

// Display color space manager
typedef struct NVGVkDisplayColorManager {
	// Display information
	NVGVkDisplay display;

	// Color space nodes
	NVGVkColorSpaceNode nodes[16];
	int nodeCount;

	// Active transforms
	NVGVkColorSpaceTransform* activeTransforms[8];
	int transformCount;

	// Animation state for smooth transitions
	NVGVkColorSpaceTransform currentTransform;
	NVGVkColorSpaceTransform targetTransform;
	float transitionProgress;			// 0.0 to 1.0
	int isTransitioning;
} NVGVkDisplayColorManager;

// Create display color manager from Vulkan physical device
NVGVkDisplayColorManager* nvgvk_display_color_manager_create(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface
);

// Destroy manager
void nvgvk_display_color_manager_destroy(NVGVkDisplayColorManager* manager);

// Build color space transform from source to destination
void nvgvk_build_color_space_transform(
	const NVGVkColorPrimaries* srcPrimaries,
	const NVGVkColorPrimaries* dstPrimaries,
	NVGVkTransferFunctionID srcTransfer,
	NVGVkTransferFunctionID dstTransfer,
	float hdrScale,
	NVGVkColorSpaceTransform* transform
);

// Validate transform (check orthogonality, determinant)
void nvgvk_validate_transform(NVGVkColorSpaceTransform* transform);

// Interpolate between two transforms using quaternion SLERP
void nvgvk_interpolate_transforms(
	const NVGVkColorSpaceTransform* a,
	const NVGVkColorSpaceTransform* b,
	float t,
	NVGVkColorSpaceTransform* result
);

// Set target color space (starts smooth transition)
void nvgvk_display_set_target_color_space(
	NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR targetSpace,
	VkColorSpaceKHR sourceSpace
);

// Update transition animation
void nvgvk_display_update_transition(
	NVGVkDisplayColorManager* manager,
	float deltaTime
);

// Get current active transform
const NVGVkColorSpaceTransform* nvgvk_display_get_current_transform(
	const NVGVkDisplayColorManager* manager
);

// Register a color space node in the graph
void nvgvk_display_register_color_space(
	NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR colorSpace,
	const NVGVkColorPrimaries* primaries,
	NVGVkTransferFunctionID transfer
);

// Find shortest path between two color spaces
int nvgvk_display_find_transform_path(
	const NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR srcSpace,
	VkColorSpaceKHR dstSpace,
	VkColorSpaceKHR* pathOut,
	int maxPathLength
);

// Print display capabilities
void nvgvk_display_print_capabilities(const NVGVkDisplay* display);

// Print transform information
void nvgvk_transform_print_info(const NVGVkColorSpaceTransform* transform);

// Compose two transforms: result = b(a(x))
void nvgvk_compose_transforms(
	const NVGVkColorSpaceTransform* a,
	const NVGVkColorSpaceTransform* b,
	NVGVkColorSpaceTransform* result
);

// Get identity transform (no-op)
void nvgvk_identity_transform(NVGVkColorSpaceTransform* transform);

#endif // NVG_VK_DISPLAY_COLOR_SPACE_H
