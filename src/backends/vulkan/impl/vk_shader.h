#ifndef VK_SHADER_H
#define VK_SHADER_H

#include <vulkan/vulkan.h>

VkShaderModule vk_load_shader_module(VkDevice device, const char* filename);

#endif
