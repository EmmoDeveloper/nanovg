#include "nvg_vk_shader.h"
#include "vk_shader.h"
#include <stdio.h>
#include <string.h>

// Shader file paths (maps to NVGVkPipelineType)
static const char* nvgvk__shader_paths[][2] = {
	{"src/shaders/fill_grad.vert.spv", "src/shaders/fill_grad.frag.spv"},  // FILL_STENCIL
	{"src/shaders/fill_grad.vert.spv", "src/shaders/fill_grad.frag.spv"},  // FILL_COVER_GRAD
	{"src/shaders/fill_img.vert.spv", "src/shaders/fill_img.frag.spv"},    // FILL_COVER_IMG
	{"src/shaders/simple.vert.spv", "src/shaders/simple.frag.spv"},        // SIMPLE
	{"src/shaders/img.vert.spv", "src/shaders/img.frag.spv"},              // IMG
	{"src/shaders/img.vert.spv", "src/shaders/img.frag.spv"},              // IMG_STENCIL
	{"src/shaders/simple.vert.spv", "src/shaders/simple.frag.spv"},        // FRINGE (uses simple shader with AA)
	{"src/shaders/img.vert.spv", "src/shaders/text_msdf_simple.frag.spv"}, // TEXT_MSDF
	{"src/shaders/img.vert.spv", "src/shaders/text_subpixel.frag.spv"},    // TEXT_SUBPIXEL
};

int nvgvk_create_shaders(NVGVkContext* vk)
{
	if (!vk) {
		return 0;
	}

	for (int i = 0; i < NVGVK_SHADER_COUNT; i++) {
		NVGVkShaderSet* shader = &vk->shaders[i];

		// Load vertex shader
		shader->vertShader = vk_load_shader_module(vk->device, nvgvk__shader_paths[i][0]);
		if (shader->vertShader == VK_NULL_HANDLE) {
			fprintf(stderr, "NanoVG Vulkan: Failed to load vertex shader: %s\n", nvgvk__shader_paths[i][0]);
			nvgvk_destroy_shaders(vk);
			return 0;
		}

		// Load fragment shader
		shader->fragShader = vk_load_shader_module(vk->device, nvgvk__shader_paths[i][1]);
		if (shader->fragShader == VK_NULL_HANDLE) {
			fprintf(stderr, "NanoVG Vulkan: Failed to load fragment shader: %s\n", nvgvk__shader_paths[i][1]);
			vkDestroyShaderModule(vk->device, shader->vertShader, NULL);
			shader->vertShader = VK_NULL_HANDLE;
			nvgvk_destroy_shaders(vk);
			return 0;
		}
	}

	return 1;
}

void nvgvk_destroy_shaders(NVGVkContext* vk)
{
	if (!vk) {
		return;
	}

	for (int i = 0; i < NVGVK_SHADER_COUNT; i++) {
		NVGVkShaderSet* shader = &vk->shaders[i];

		if (shader->vertShader) {
			vkDestroyShaderModule(vk->device, shader->vertShader, NULL);
			shader->vertShader = VK_NULL_HANDLE;
		}
		if (shader->fragShader) {
			vkDestroyShaderModule(vk->device, shader->fragShader, NULL);
			shader->fragShader = VK_NULL_HANDLE;
		}
	}
}
