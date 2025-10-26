#ifndef NVG_VK_PIPELINE_H
#define NVG_VK_PIPELINE_H

#include "nvg_vk_types.h"

// Pipeline types for NanoVG rendering (matches NVGVK_PIPELINE_COUNT from types.h)
typedef enum NVGVkPipelineType {
	NVGVK_PIPELINE_FILL_GRAD = 0,
	NVGVK_PIPELINE_FILL_IMG = 1,
	NVGVK_PIPELINE_SIMPLE = 2,
	NVGVK_PIPELINE_IMG = 3
} NVGVkPipelineType;

// Pipeline management
int nvgvk_create_pipelines(NVGVkContext* vk, VkRenderPass renderPass);
void nvgvk_destroy_pipelines(NVGVkContext* vk);
void nvgvk_bind_pipeline(NVGVkContext* vk, NVGVkPipelineType type);

#endif // NVG_VK_PIPELINE_H
