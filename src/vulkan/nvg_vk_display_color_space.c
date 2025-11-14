#include "nvg_vk_display_color_space.h"
#include "nvg_vk_color_space.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Helper: Compute matrix determinant
static float nvgvk__mat3_determinant(const NVGVkMat3* m)
{
	const float* a = m->m;
	return a[0] * (a[4] * a[8] - a[7] * a[5]) -
	       a[1] * (a[3] * a[8] - a[5] * a[6]) +
	       a[2] * (a[3] * a[7] - a[4] * a[6]);
}

// Helper: Check orthogonality (R^T × R should be identity)
static float nvgvk__mat3_orthogonality_error(const NVGVkMat3* m)
{
	NVGVkMat3 mT, product;

	// Transpose
	mT.m[0] = m->m[0]; mT.m[1] = m->m[3]; mT.m[2] = m->m[6];
	mT.m[3] = m->m[1]; mT.m[4] = m->m[4]; mT.m[5] = m->m[7];
	mT.m[6] = m->m[2]; mT.m[7] = m->m[5]; mT.m[8] = m->m[8];

	// Multiply
	nvgvk_mat3_multiply(&mT, m, &product);

	// Compute deviation from identity
	float error = 0.0f;
	error += fabsf(product.m[0] - 1.0f);
	error += fabsf(product.m[1]);
	error += fabsf(product.m[2]);
	error += fabsf(product.m[3]);
	error += fabsf(product.m[4] - 1.0f);
	error += fabsf(product.m[5]);
	error += fabsf(product.m[6]);
	error += fabsf(product.m[7]);
	error += fabsf(product.m[8] - 1.0f);

	return error;
}

void nvgvk_build_color_space_transform(
	const NVGVkColorPrimaries* srcPrimaries,
	const NVGVkColorPrimaries* dstPrimaries,
	NVGVkTransferFunctionID srcTransfer,
	NVGVkTransferFunctionID dstTransfer,
	float hdrScale,
	NVGVkColorSpaceTransform* transform)
{
	if (!transform) return;

	memset(transform, 0, sizeof(NVGVkColorSpaceTransform));

	transform->srcPrimaries = srcPrimaries;
	transform->dstPrimaries = dstPrimaries;
	transform->srcTransfer = srcTransfer;
	transform->dstTransfer = dstTransfer;
	transform->hdrScale = hdrScale;

	// Build gamut conversion matrix
	nvgvk_primaries_conversion_matrix(srcPrimaries, dstPrimaries, &transform->matrix);

	// Decompose into quaternion + scale
	nvgvk_mat3_decompose(&transform->matrix, &transform->decomposed);

	// Validate
	nvgvk_validate_transform(transform);
}

void nvgvk_validate_transform(NVGVkColorSpaceTransform* transform)
{
	if (!transform) return;

	// Recompose to get clean matrix
	NVGVkMat3 cleanMatrix;
	nvgvk_mat3_compose(&transform->decomposed, &cleanMatrix);

	// Check determinant (should be positive, close to 1.0 for volume-preserving)
	transform->determinant = nvgvk__mat3_determinant(&cleanMatrix);

	// Check orthogonality of rotation component
	NVGVkMat3 rotationMatrix;
	nvgvk_quat_to_mat3(&transform->decomposed.rotation, &rotationMatrix);
	transform->orthogonalityError = nvgvk__mat3_orthogonality_error(&rotationMatrix);

	// Valid if determinant is positive and orthogonality error is small
	transform->isValid = (transform->determinant > 0.0f) &&
	                     (transform->orthogonalityError < 0.01f);

	// Update matrix with cleaned version
	transform->matrix = cleanMatrix;
}

void nvgvk_interpolate_transforms(
	const NVGVkColorSpaceTransform* a,
	const NVGVkColorSpaceTransform* b,
	float t,
	NVGVkColorSpaceTransform* result)
{
	if (!a || !b || !result) return;

	// SLERP rotation quaternion
	nvgvk_quat_slerp(&a->decomposed.rotation, &b->decomposed.rotation, t, &result->decomposed.rotation);

	// Linear interpolate scale
	for (int i = 0; i < 3; i++) {
		result->decomposed.scale[i] = a->decomposed.scale[i] + t * (b->decomposed.scale[i] - a->decomposed.scale[i]);
	}

	// Interpolate transfer functions (discrete, use threshold)
	result->srcTransfer = (t < 0.5f) ? a->srcTransfer : b->srcTransfer;
	result->dstTransfer = (t < 0.5f) ? a->dstTransfer : b->dstTransfer;

	// Interpolate HDR scale
	result->hdrScale = a->hdrScale + t * (b->hdrScale - a->hdrScale);

	// Copy primaries pointers (discrete transition at t=0.5)
	result->srcPrimaries = (t < 0.5f) ? a->srcPrimaries : b->srcPrimaries;
	result->dstPrimaries = (t < 0.5f) ? a->dstPrimaries : b->dstPrimaries;

	// Recompose matrix
	nvgvk_mat3_compose(&result->decomposed, &result->matrix);

	// Validate
	nvgvk_validate_transform(result);
}

NVGVkDisplayColorManager* nvgvk_display_color_manager_create(
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface)
{
	NVGVkDisplayColorManager* manager = (NVGVkDisplayColorManager*)calloc(1, sizeof(NVGVkDisplayColorManager));
	if (!manager) return NULL;

	// Query surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);

	if (formatCount > 0) {
		VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

		// Extract unique color spaces
		manager->display.supportedSpaceCount = 0;
		for (uint32_t i = 0; i < formatCount && manager->display.supportedSpaceCount < 16; i++) {
			VkColorSpaceKHR cs = formats[i].colorSpace;
			int isDuplicate = 0;
			for (int j = 0; j < manager->display.supportedSpaceCount; j++) {
				if (manager->display.supportedSpaces[j] == cs) {
					isDuplicate = 1;
					break;
				}
			}
			if (!isDuplicate) {
				manager->display.supportedSpaces[manager->display.supportedSpaceCount++] = cs;
			}
		}

		free(formats);
	}

	// Set display name
	snprintf(manager->display.name, sizeof(manager->display.name), "Primary Display");

	// Default to sRGB
	manager->display.activeColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	manager->display.maxLuminance = 80.0f;
	manager->display.minLuminance = 0.1f;
	manager->display.nativePrimaries = NVGVK_PRIMARIES_BT709;

	// Register standard color space nodes
	nvgvk_display_register_color_space(manager, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	                                    &NVGVK_PRIMARIES_BT709, NVGVK_TRANSFER_SRGB);
	nvgvk_display_register_color_space(manager, VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
	                                    &NVGVK_PRIMARIES_DISPLAYP3, NVGVK_TRANSFER_SRGB);
	nvgvk_display_register_color_space(manager, VK_COLOR_SPACE_BT2020_LINEAR_EXT,
	                                    &NVGVK_PRIMARIES_BT2020, NVGVK_TRANSFER_LINEAR);

	// Initialize identity transform
	nvgvk_identity_transform(&manager->currentTransform);
	nvgvk_identity_transform(&manager->targetTransform);

	return manager;
}

void nvgvk_display_color_manager_destroy(NVGVkDisplayColorManager* manager)
{
	if (manager) {
		free(manager);
	}
}

void nvgvk_display_register_color_space(
	NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR colorSpace,
	const NVGVkColorPrimaries* primaries,
	NVGVkTransferFunctionID transfer)
{
	if (!manager || manager->nodeCount >= 16) return;

	NVGVkColorSpaceNode* node = &manager->nodes[manager->nodeCount++];
	node->colorSpace = colorSpace;
	node->primaries = primaries;
	node->transfer = transfer;

	// Precompute XYZ transforms for graph traversal
	NVGVkMat3 toXYZMatrix, fromXYZMatrix;
	nvgvk_primaries_to_xyz_matrix(primaries, &toXYZMatrix);
	nvgvk_xyz_to_primaries_matrix(primaries, &fromXYZMatrix);

	nvgvk_mat3_decompose(&toXYZMatrix, &node->toXYZ);
	nvgvk_mat3_decompose(&fromXYZMatrix, &node->fromXYZ);
}

void nvgvk_display_set_target_color_space(
	NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR targetSpace,
	VkColorSpaceKHR sourceSpace)
{
	if (!manager) return;

	// Find nodes for source and target
	const NVGVkColorSpaceNode* srcNode = NULL;
	const NVGVkColorSpaceNode* dstNode = NULL;

	for (int i = 0; i < manager->nodeCount; i++) {
		if (manager->nodes[i].colorSpace == sourceSpace) {
			srcNode = &manager->nodes[i];
		}
		if (manager->nodes[i].colorSpace == targetSpace) {
			dstNode = &manager->nodes[i];
		}
	}

	if (!srcNode || !dstNode) {
		printf("Warning: Color space not registered\n");
		return;
	}

	// Build target transform
	nvgvk_build_color_space_transform(
		srcNode->primaries,
		dstNode->primaries,
		srcNode->transfer,
		dstNode->transfer,
		1.0f,
		&manager->targetTransform
	);

	// Start transition
	manager->isTransitioning = 1;
	manager->transitionProgress = 0.0f;
}

void nvgvk_display_update_transition(
	NVGVkDisplayColorManager* manager,
	float deltaTime)
{
	if (!manager || !manager->isTransitioning) return;

	// Update progress (smooth over 0.5 seconds)
	manager->transitionProgress += deltaTime * 2.0f;

	if (manager->transitionProgress >= 1.0f) {
		manager->transitionProgress = 1.0f;
		manager->isTransitioning = 0;
		manager->currentTransform = manager->targetTransform;
	} else {
		// Interpolate using quaternion SLERP
		nvgvk_interpolate_transforms(
			&manager->currentTransform,
			&manager->targetTransform,
			manager->transitionProgress,
			&manager->currentTransform
		);
	}
}

const NVGVkColorSpaceTransform* nvgvk_display_get_current_transform(
	const NVGVkDisplayColorManager* manager)
{
	return manager ? &manager->currentTransform : NULL;
}

void nvgvk_display_print_capabilities(const NVGVkDisplay* display)
{
	if (!display) return;

	printf("Display: %s\n", display->name);
	printf("  HDR Capable: %s\n", display->isHDR ? "Yes" : "No");
	printf("  Peak Luminance: %.1f cd/m²\n", display->maxLuminance);
	printf("  Black Level: %.3f cd/m²\n", display->minLuminance);
	printf("  Supported Color Spaces: %d\n", display->supportedSpaceCount);

	for (int i = 0; i < display->supportedSpaceCount; i++) {
		const char* name = nvgvk_color_space_get_name(display->supportedSpaces[i]);
		printf("    %d. %s\n", i + 1, name);
	}
}

void nvgvk_transform_print_info(const NVGVkColorSpaceTransform* transform)
{
	if (!transform) return;

	printf("Color Space Transform:\n");
	printf("  Valid: %s\n", transform->isValid ? "Yes" : "No");
	printf("  Determinant: %.6f\n", transform->determinant);
	printf("  Orthogonality Error: %.8f\n", transform->orthogonalityError);
	printf("  HDR Scale: %.2f\n", transform->hdrScale);

	printf("  Quaternion: [%.4f, %.4f, %.4f, %.4f]\n",
	       transform->decomposed.rotation.w,
	       transform->decomposed.rotation.x,
	       transform->decomposed.rotation.y,
	       transform->decomposed.rotation.z);

	printf("  Scale: [%.4f, %.4f, %.4f]\n",
	       transform->decomposed.scale[0],
	       transform->decomposed.scale[1],
	       transform->decomposed.scale[2]);

	printf("  Matrix:\n");
	printf("    [%.4f %.4f %.4f]\n", transform->matrix.m[0], transform->matrix.m[1], transform->matrix.m[2]);
	printf("    [%.4f %.4f %.4f]\n", transform->matrix.m[3], transform->matrix.m[4], transform->matrix.m[5]);
	printf("    [%.4f %.4f %.4f]\n", transform->matrix.m[6], transform->matrix.m[7], transform->matrix.m[8]);
}

void nvgvk_compose_transforms(
	const NVGVkColorSpaceTransform* a,
	const NVGVkColorSpaceTransform* b,
	NVGVkColorSpaceTransform* result)
{
	if (!a || !b || !result) return;

	// Compose quaternions: q_result = q_b * q_a
	nvgvk_quat_multiply(&b->decomposed.rotation, &a->decomposed.rotation, &result->decomposed.rotation);

	// Compose scales: s_result = s_b * R_b * s_a * R_b^T
	// Simplified: multiply scales elementwise (assumes aligned axes)
	for (int i = 0; i < 3; i++) {
		result->decomposed.scale[i] = a->decomposed.scale[i] * b->decomposed.scale[i];
	}

	// Transfer functions: use outermost
	result->srcTransfer = a->srcTransfer;
	result->dstTransfer = b->dstTransfer;

	// HDR scale: multiply
	result->hdrScale = a->hdrScale * b->hdrScale;

	// Primaries: use endpoints
	result->srcPrimaries = a->srcPrimaries;
	result->dstPrimaries = b->dstPrimaries;

	// Recompose matrix
	nvgvk_mat3_compose(&result->decomposed, &result->matrix);

	// Validate
	nvgvk_validate_transform(result);
}

void nvgvk_identity_transform(NVGVkColorSpaceTransform* transform)
{
	if (!transform) return;

	memset(transform, 0, sizeof(NVGVkColorSpaceTransform));

	// Identity quaternion
	nvgvk_quat_identity(&transform->decomposed.rotation);

	// Unit scale
	transform->decomposed.scale[0] = 1.0f;
	transform->decomposed.scale[1] = 1.0f;
	transform->decomposed.scale[2] = 1.0f;

	// Identity matrix
	nvgvk_mat3_identity(&transform->matrix);

	// Default transfers
	transform->srcTransfer = NVGVK_TRANSFER_LINEAR;
	transform->dstTransfer = NVGVK_TRANSFER_LINEAR;
	transform->hdrScale = 1.0f;

	transform->determinant = 1.0f;
	transform->orthogonalityError = 0.0f;
	transform->isValid = 1;
}

int nvgvk_display_find_transform_path(
	const NVGVkDisplayColorManager* manager,
	VkColorSpaceKHR srcSpace,
	VkColorSpaceKHR dstSpace,
	VkColorSpaceKHR* pathOut,
	int maxPathLength)
{
	if (!manager || !pathOut || maxPathLength < 2) return 0;

	// Simple path: src -> XYZ -> dst (always valid)
	pathOut[0] = srcSpace;
	pathOut[1] = dstSpace;
	return 2;
}
