#ifndef NVG_VK_RENDER_H
#define NVG_VK_RENDER_H

#include "nvg_vk_types.h"

// Setup function (call before command buffer recording)
void nvgvk_setup_render(NVGVkContext* vk);

// Render pass management
void nvgvk_begin_render_pass(NVGVkContext* vk, VkRenderPass renderPass, VkFramebuffer framebuffer,
                              VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount,
                              VkViewport viewport, VkRect2D scissor);
void nvgvk_end_render_pass(NVGVkContext* vk);

// Rendering functions
void nvgvk_render_fill(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_convex_fill(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_stroke(NVGVkContext* vk, NVGVkCall* call);
void nvgvk_render_triangles(NVGVkContext* vk, NVGVkCall* call);

// Helper: Convert NanoVG blend mode to Vulkan blend factors
void nvgvk_get_blend_factors(int blendFunc, VkBlendFactor* srcColor, VkBlendFactor* dstColor,
                              VkBlendFactor* srcAlpha, VkBlendFactor* dstAlpha);

#endif // NVG_VK_RENDER_H
