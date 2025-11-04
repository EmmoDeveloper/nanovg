#include "nvg_vk_color_space_ubo.h"
#include "nvg_vk_buffer.h"
#include "nvg_vk_color_space.h"
#include <stdio.h>
#include <string.h>

// Initialize color space descriptor layout (must be called before pipelines)
int nvgvk_init_color_space_layout(NVGVkContext* vk)
{
	// Create descriptor set layout for color space UBO (set = 1, binding = 0)
	VkDescriptorSetLayoutBinding binding = {0};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &vk->colorSpaceDescriptorLayout) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create color space descriptor set layout\n");
		return 0;
	}

	return 1;
}

// Initialize color space UBO buffer and descriptor set (after pipelines)
int nvgvk_init_color_space_ubo(NVGVkContext* vk)
{
	// Descriptor layout should already be created by nvgvk_init_color_space_layout
	if (!vk->colorSpaceDescriptorLayout) {
		fprintf(stderr, "NanoVG Vulkan: Color space descriptor layout not initialized\n");
		return 0;
	}

	// Create uniform buffer for color space parameters (64 bytes)
	VkDeviceSize bufferSize = sizeof(NVGVkColorSpaceUniforms);
	if (!nvgvk_buffer_create(vk, &vk->colorSpaceUBO, bufferSize,
	                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
	                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create color space UBO\n");
		return 0;
	}

	// Map the buffer for persistent updates
	if (vkMapMemory(vk->device, vk->colorSpaceUBO.memory, 0, bufferSize, 0, &vk->colorSpaceUBO.mapped) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to map color space UBO\n");
		nvgvk_buffer_destroy(vk, &vk->colorSpaceUBO);
		return 0;
	}

	// Allocate descriptor set from existing descriptor pool
	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk->pipelines[0].descriptorPool;  // Reuse existing pool
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &vk->colorSpaceDescriptorLayout;

	if (vkAllocateDescriptorSets(vk->device, &allocInfo, &vk->colorSpaceDescriptorSet) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate color space descriptor set\n");
		vkUnmapMemory(vk->device, vk->colorSpaceUBO.memory);
		nvgvk_buffer_destroy(vk, &vk->colorSpaceUBO);
		return 0;
	}

	// Bind UBO to descriptor set
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = vk->colorSpaceUBO.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(NVGVkColorSpaceUniforms);

	VkWriteDescriptorSet descriptorWrite = {0};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = vk->colorSpaceDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);

	// Initialize with identity transform (no conversion)
	nvgvk_update_color_space_ubo(vk);

	return 1;
}

// Destroy color space UBO resources
void nvgvk_destroy_color_space_ubo(NVGVkContext* vk)
{
	if (vk->colorSpaceUBO.buffer != VK_NULL_HANDLE) {
		if (vk->colorSpaceUBO.mapped) {
			vkUnmapMemory(vk->device, vk->colorSpaceUBO.memory);
		}
		nvgvk_buffer_destroy(vk, &vk->colorSpaceUBO);
	}

	if (vk->colorSpaceDescriptorLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(vk->device, vk->colorSpaceDescriptorLayout, NULL);
	}

	// Descriptor set is freed when pool is destroyed
}

// Update color space UBO with current conversion parameters
void nvgvk_update_color_space_ubo(NVGVkContext* vk)
{
	if (!vk->colorSpaceUBO.mapped) {
		return;
	}

	NVGVkColorSpaceUniforms uniforms = {0};

	// If no color space configured, use identity transform
	if (vk->colorSpace) {
		NVGVkColorSpace* cs = (NVGVkColorSpace*)vk->colorSpace;
		uniforms.gamutMatrix = cs->conversionPath.gamutMatrix;
		uniforms.srcTransferID = cs->conversionPath.srcTransferID;
		uniforms.dstTransferID = cs->conversionPath.dstTransferID;
		uniforms.hdrScale = cs->conversionPath.hdrScale;
		uniforms.useGamutMapping = cs->useGamutMapping;
		uniforms.useToneMapping = cs->useToneMapping;
	} else {
		// Identity: sRGB → sRGB, no HDR scaling, identity gamut matrix
		// Matrix is stored as flat array [0..2] = row 0, [3..5] = row 1, [6..8] = row 2
		uniforms.gamutMatrix.m[0] = 1.0f;  // [0,0]
		uniforms.gamutMatrix.m[4] = 1.0f;  // [1,1]
		uniforms.gamutMatrix.m[8] = 1.0f;  // [2,2]
		uniforms.srcTransferID = 1;  // TRANSFER_SRGB
		uniforms.dstTransferID = 1;  // TRANSFER_SRGB
		uniforms.hdrScale = 1.0f;
		uniforms.useGamutMapping = 0;
		uniforms.useToneMapping = 0;
	}

	// Copy to mapped buffer
	memcpy(vk->colorSpaceUBO.mapped, &uniforms, sizeof(NVGVkColorSpaceUniforms));
}
