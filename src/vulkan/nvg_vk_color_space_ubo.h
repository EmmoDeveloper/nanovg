#ifndef NVG_VK_COLOR_SPACE_UBO_H
#define NVG_VK_COLOR_SPACE_UBO_H

#include "nvg_vk_types.h"

// Initialize color space UBO descriptor and buffer
// Called during context creation
int nvgvk_init_color_space_ubo(NVGVkContext* vk);

// Destroy color space UBO resources
// Called during context destruction
void nvgvk_destroy_color_space_ubo(NVGVkContext* vk);

// Update color space UBO with current conversion parameters
// Called when color space changes
void nvgvk_update_color_space_ubo(NVGVkContext* vk);

#endif // NVG_VK_COLOR_SPACE_UBO_H
