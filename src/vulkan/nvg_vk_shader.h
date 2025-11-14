#ifndef NVG_VK_SHADER_H
#define NVG_VK_SHADER_H

#include "nvg_vk_types.h"
#include "vk_shader.h"  // Reuse existing shader loading

// Shader types for NanoVG (maps to NVGVkPipelineType, each pipeline gets its own shader set)
typedef enum NVGVkShaderType {
	NVGVK_SHADER_FILL_STENCIL,
	NVGVK_SHADER_FILL_COVER_GRAD,
	NVGVK_SHADER_FILL_COVER_IMG,
	NVGVK_SHADER_SIMPLE,
	NVGVK_SHADER_IMG,
	NVGVK_SHADER_IMG_STENCIL,
	NVGVK_SHADER_FRINGE,
	NVGVK_SHADER_TEXT_MSDF,
	NVGVK_SHADER_TEXT_SUBPIXEL,
	NVGVK_SHADER_COUNT
} NVGVkShaderType;

// Shader management
int nvgvk_create_shaders(NVGVkContext* vk);
void nvgvk_destroy_shaders(NVGVkContext* vk);

#endif // NVG_VK_SHADER_H
