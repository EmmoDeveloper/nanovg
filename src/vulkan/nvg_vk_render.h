#ifndef NVG_VK_RENDER_H
#define NVG_VK_RENDER_H

#include "nvg_vk_types.h"

// Rendering functions
void nvgvk_render_fill(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_convex_fill(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_stroke(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_triangles(NVGVkContext* vk, NVGVkCall* call);

// Helper: Convert NanoVG blend mode to Vulkan blend factors
void nvgvk_get_blend_factors(int blendFunc, VkBlendFactor* srcColor, VkBlendFactor* dstColor,
                              VkBlendFactor* srcAlpha, VkBlendFactor* dstAlpha);

#endif // NVG_VK_RENDER_H
