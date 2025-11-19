#include "nvg_vk_shader.h"
#include "vk_shader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Shader file names (relative to base path)
static const char* nvgvk__shader_files[][2] = {
	{"fill_grad.vert.spv", "fill_grad.frag.spv"},       // FILL_STENCIL
	{"fill_grad.vert.spv", "fill_grad.frag.spv"},       // FILL_COVER_GRAD
	{"fill_img.vert.spv", "fill_img.frag.spv"},         // FILL_COVER_IMG
	{"simple.vert.spv", "simple.frag.spv"},             // SIMPLE
	{"img.vert.spv", "img.frag.spv"},                   // IMG
	{"img.vert.spv", "img.frag.spv"},                   // IMG_STENCIL
	{"simple.vert.spv", "simple.frag.spv"},             // FRINGE (uses simple shader with AA)
	{"img.vert.spv", "text_msdf_simple.frag.spv"},      // TEXT_MSDF
	{"img.vert.spv", "text_subpixel.frag.spv"},         // TEXT_SUBPIXEL
	{"img.vert.spv", "text_alpha.frag.spv"},            // TEXT_ALPHA
};

static char* nvgvk__build_shader_path(const char* basePath, const char* filename)
{
	const char* base = basePath ? basePath : "src/shaders";
	size_t baseLen = strlen(base);
	size_t fileLen = strlen(filename);
	size_t totalLen = baseLen + 1 + fileLen + 1;  // base + '/' + filename + '\0'

	char* path = (char*)malloc(totalLen);
	if (!path) {
		return NULL;
	}

	snprintf(path, totalLen, "%s/%s", base, filename);
	return path;
}

int nvgvk_create_shaders(NVGVkContext* vk)
{
	if (!vk) {
		return 0;
	}

	for (int i = 0; i < NVGVK_SHADER_COUNT; i++) {
		NVGVkShaderSet* shader = &vk->shaders[i];

		// Build vertex shader path
		char* vertPath = nvgvk__build_shader_path(vk->shaderBasePath, nvgvk__shader_files[i][0]);
		if (!vertPath) {
			fprintf(stderr, "NanoVG Vulkan: Failed to allocate memory for shader path\n");
			nvgvk_destroy_shaders(vk);
			return 0;
		}

		// Load vertex shader
		shader->vertShader = vk_load_shader_module(vk->device, vertPath);
		if (shader->vertShader == VK_NULL_HANDLE) {
			fprintf(stderr, "NanoVG Vulkan: Failed to load vertex shader: %s\n", vertPath);
			free(vertPath);
			nvgvk_destroy_shaders(vk);
			return 0;
		}
		free(vertPath);

		// Build fragment shader path
		char* fragPath = nvgvk__build_shader_path(vk->shaderBasePath, nvgvk__shader_files[i][1]);
		if (!fragPath) {
			fprintf(stderr, "NanoVG Vulkan: Failed to allocate memory for shader path\n");
			vkDestroyShaderModule(vk->device, shader->vertShader, NULL);
			shader->vertShader = VK_NULL_HANDLE;
			nvgvk_destroy_shaders(vk);
			return 0;
		}

		// Load fragment shader
		shader->fragShader = vk_load_shader_module(vk->device, fragPath);
		if (shader->fragShader == VK_NULL_HANDLE) {
			fprintf(stderr, "NanoVG Vulkan: Failed to load fragment shader: %s\n", fragPath);
			free(fragPath);
			vkDestroyShaderModule(vk->device, shader->vertShader, NULL);
			shader->vertShader = VK_NULL_HANDLE;
			nvgvk_destroy_shaders(vk);
			return 0;
		}
		free(fragPath);
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
