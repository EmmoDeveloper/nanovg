#include "vk_shader.h"
#include <stdio.h>
#include <stdlib.h>

VkShaderModule vk_load_shader_module(VkDevice device, const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open shader file: %s\n", filename);
		return VK_NULL_HANDLE;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* code = malloc(fileSize);
	if (!code) {
		fclose(file);
		return VK_NULL_HANDLE;
	}

	fread(code, 1, fileSize, file);
	fclose(file);

	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = (const uint32_t*)code;

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module\n");
		free(code);
		return VK_NULL_HANDLE;
	}

	free(code);
	return shaderModule;
}
