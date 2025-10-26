#ifndef NVG_VK_SHADER_H
#define NVG_VK_SHADER_H

#include "nvg_vk_types.h"
#include "vk_shader.h"  // Reuse existing shader loading

// Shader types for NanoVG
typedef enum NVGVkShaderType {
	NVGVK_SHADER_FILLGRAD,
	NVGVK_SHADER_FILLIMG,
	NVGVK_SHADER_SIMPLE,
	NVGVK_SHADER_IMG,
	NVGVK_SHADER_COUNT
} NVGVkShaderType;

// Shader management
int nvgvk_create_shaders(NVGVkContext* vk);
void nvgvk_destroy_shaders(NVGVkContext* vk);

#endif // NVG_VK_SHADER_H
