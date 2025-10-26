#include "nvg_vk_shader.h"
#include "vk_shader.h"
#include <stdio.h>
#include <string.h>

// Shader file paths
static const char* nvgvk__shader_paths[][2] = {
	{"shaders/fill_grad.vert.spv", "shaders/fill_grad.frag.spv"},  // FILLGRAD
	{"shaders/fill_img.vert.spv", "shaders/fill_img.frag.spv"},    // FILLIMG
	{"shaders/simple.vert.spv", "shaders/simple.frag.spv"},        // SIMPLE
	{"shaders/img.vert.spv", "shaders/img.frag.spv"},              // IMG
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

	printf("NanoVG Vulkan: Loaded %d shader sets\n", NVGVK_SHADER_COUNT);
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
