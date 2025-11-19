#ifndef NVG_VK_COLOR_SPACE_UBO_H
#define NVG_VK_COLOR_SPACE_UBO_H

#include "nvg_vk_types.h"

// Initialize color space descriptor layout (must be called before pipelines)
// Called during context creation, before pipeline creation
int nvgvk_init_color_space_layout(NVGVkContext* vk);

// Initialize color space UBO buffer and descriptor set (after pipelines exist)
// Called after pipeline creation to reuse descriptor pool
int nvgvk_init_color_space_ubo(NVGVkContext* vk);

// Destroy color space UBO resources
// Called during context destruction
void nvgvk_destroy_color_space_ubo(NVGVkContext* vk);

// Update color space UBO with current conversion parameters
// Called when color space changes
void nvgvk_update_color_space_ubo(NVGVkContext* vk);

#endif // NVG_VK_COLOR_SPACE_UBO_H
