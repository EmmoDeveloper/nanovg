#ifndef NVG_VK_COMPUTE_H
#define NVG_VK_COMPUTE_H

#include <vulkan/vulkan.h>
#include "nvg_vk_types.h"

// Compute pipeline structure
typedef struct NVGVkComputePipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkShaderModule shader;
	int valid;
} NVGVkComputePipeline;

// Create compute pipeline from SPIR-V bytecode
int nvgvk_create_compute_pipeline(NVGVkContext* vk,
                                   const uint32_t* spirv, size_t spirvSize,
                                   const VkDescriptorSetLayoutBinding* bindings,
                                   uint32_t bindingCount,
                                   uint32_t pushConstantSize,
                                   NVGVkComputePipeline* pipeline);

// Destroy compute pipeline and free resources
void nvgvk_destroy_compute_pipeline(NVGVkContext* vk,
                                     NVGVkComputePipeline* pipeline);

// Dispatch compute shader
void nvgvk_dispatch_compute(NVGVkContext* vk,
                            NVGVkComputePipeline* pipeline,
                            VkCommandBuffer cmdBuffer,
                            uint32_t groupCountX,
                            uint32_t groupCountY,
                            uint32_t groupCountZ);

// Update descriptor set bindings
int nvgvk_update_compute_descriptors(NVGVkContext* vk,
                                      NVGVkComputePipeline* pipeline,
                                      uint32_t binding,
                                      VkDescriptorType type,
                                      VkBuffer buffer,
                                      VkDeviceSize offset,
                                      VkDeviceSize range);

// Update image descriptor
int nvgvk_update_compute_image_descriptor(NVGVkContext* vk,
                                           NVGVkComputePipeline* pipeline,
                                           uint32_t binding,
                                           VkImageView imageView,
                                           VkImageLayout layout);

#endif // NVG_VK_COMPUTE_H
