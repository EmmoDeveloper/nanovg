#ifndef NVG_VK_PIPELINE_H
#define NVG_VK_PIPELINE_H

#include "nvg_vk_types.h"

// Pipeline types for NanoVG rendering (matches NVGVK_PIPELINE_COUNT from types.h)
typedef enum NVGVkPipelineType {
	NVGVK_PIPELINE_FILL_STENCIL = 0,     // Stencil write pass (color disabled)
	NVGVK_PIPELINE_FILL_COVER_GRAD = 1,  // Cover pass with gradient (stencil test)
	NVGVK_PIPELINE_FILL_COVER_IMG = 2,   // Cover pass with image (stencil test)
	NVGVK_PIPELINE_SIMPLE = 3,           // Simple fills (convex, stroke)
	NVGVK_PIPELINE_IMG = 4,              // Image rendering
	NVGVK_PIPELINE_IMG_STENCIL = 5,      // Image rendering with stencil
	NVGVK_PIPELINE_FRINGE = 6,           // AA fringe rendering (triangle strip)
	NVGVK_PIPELINE_TEXT_MSDF = 7,        // MSDF text rendering
	NVGVK_PIPELINE_TEXT_SUBPIXEL = 8     // LCD subpixel text rendering
} NVGVkPipelineType;

// Pipeline management
int nvgvk_create_pipelines(NVGVkContext* vk, VkRenderPass renderPass);
void nvgvk_destroy_pipelines(NVGVkContext* vk);
void nvgvk_bind_pipeline(NVGVkContext* vk, NVGVkPipelineType type);

#endif // NVG_VK_PIPELINE_H
