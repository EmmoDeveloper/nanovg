#include "nvg_vk_compute.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int nvgvk_create_compute_pipeline(NVGVkContext* vk,
                                   const uint32_t* spirv, size_t spirvSize,
                                   const VkDescriptorSetLayoutBinding* bindings,
                                   uint32_t bindingCount,
                                   uint32_t pushConstantSize,
                                   NVGVkComputePipeline* pipeline) {
	if (!vk || !spirv || !pipeline) return 0;

	memset(pipeline, 0, sizeof(NVGVkComputePipeline));

	// Create shader module
	VkShaderModuleCreateInfo shaderInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirvSize,
		.pCode = spirv
	};

	if (vkCreateShaderModule(vk->device, &shaderInfo, NULL, &pipeline->shader) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_vk_compute] Failed to create shader module\n");
		return 0;
	}

	// Create descriptor set layout
	if (bindingCount > 0) {
		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings = bindings
		};

		if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL,
		                                 &pipeline->descriptorSetLayout) != VK_SUCCESS) {
			fprintf(stderr, "[nvg_vk_compute] Failed to create descriptor set layout\n");
			vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
			return 0;
		}

		// Create descriptor pool
		VkDescriptorPoolSize poolSizes[4];
		uint32_t poolSizeCount = 0;

		// Count descriptor types
		uint32_t storageBufferCount = 0;
		uint32_t uniformBufferCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t sampledImageCount = 0;

		for (uint32_t i = 0; i < bindingCount; i++) {
			switch (bindings[i].descriptorType) {
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					storageBufferCount += bindings[i].descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					uniformBufferCount += bindings[i].descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					storageImageCount += bindings[i].descriptorCount;
					break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					sampledImageCount += bindings[i].descriptorCount;
					break;
				default:
					break;
			}
		}

		if (storageBufferCount > 0) {
			poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = storageBufferCount
			};
		}
		if (uniformBufferCount > 0) {
			poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = uniformBufferCount
			};
		}
		if (storageImageCount > 0) {
			poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = storageImageCount
			};
		}
		if (sampledImageCount > 0) {
			poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = sampledImageCount
			};
		}

		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = poolSizeCount,
			.pPoolSizes = poolSizes
		};

		if (vkCreateDescriptorPool(vk->device, &poolInfo, NULL,
		                            &pipeline->descriptorPool) != VK_SUCCESS) {
			fprintf(stderr, "[nvg_vk_compute] Failed to create descriptor pool\n");
			vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
			vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
			return 0;
		}

		// Allocate descriptor set
		VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = pipeline->descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &pipeline->descriptorSetLayout
		};

		if (vkAllocateDescriptorSets(vk->device, &allocInfo,
		                              &pipeline->descriptorSet) != VK_SUCCESS) {
			fprintf(stderr, "[nvg_vk_compute] Failed to allocate descriptor set\n");
			vkDestroyDescriptorPool(vk->device, pipeline->descriptorPool, NULL);
			vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
			vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
			return 0;
		}
	}

	// Create pipeline layout
	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = pushConstantSize
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = (bindingCount > 0) ? 1 : 0,
		.pSetLayouts = (bindingCount > 0) ? &pipeline->descriptorSetLayout : NULL,
		.pushConstantRangeCount = (pushConstantSize > 0) ? 1 : 0,
		.pPushConstantRanges = (pushConstantSize > 0) ? &pushConstantRange : NULL
	};

	if (vkCreatePipelineLayout(vk->device, &pipelineLayoutInfo, NULL,
	                            &pipeline->layout) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_vk_compute] Failed to create pipeline layout\n");
		if (bindingCount > 0) {
			vkDestroyDescriptorPool(vk->device, pipeline->descriptorPool, NULL);
			vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
		}
		vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
		return 0;
	}

	// Create compute pipeline
	VkComputePipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = pipeline->shader,
			.pName = "main"
		},
		.layout = pipeline->layout
	};

	if (vkCreateComputePipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
	                              &pipeline->pipeline) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_vk_compute] Failed to create compute pipeline\n");
		vkDestroyPipelineLayout(vk->device, pipeline->layout, NULL);
		if (bindingCount > 0) {
			vkDestroyDescriptorPool(vk->device, pipeline->descriptorPool, NULL);
			vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
		}
		vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
		return 0;
	}

	pipeline->valid = 1;
	return 1;
}

void nvgvk_destroy_compute_pipeline(NVGVkContext* vk, NVGVkComputePipeline* pipeline) {
	if (!vk || !pipeline || !pipeline->valid) return;

	if (pipeline->pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, pipeline->pipeline, NULL);
	}
	if (pipeline->layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(vk->device, pipeline->layout, NULL);
	}
	if (pipeline->descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(vk->device, pipeline->descriptorPool, NULL);
	}
	if (pipeline->descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
	}
	if (pipeline->shader != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, pipeline->shader, NULL);
	}

	memset(pipeline, 0, sizeof(NVGVkComputePipeline));
}

void nvgvk_dispatch_compute(NVGVkContext* vk,
                            NVGVkComputePipeline* pipeline,
                            VkCommandBuffer cmdBuffer,
                            uint32_t groupCountX,
                            uint32_t groupCountY,
                            uint32_t groupCountZ) {
	if (!vk || !pipeline || !pipeline->valid || !cmdBuffer) return;

	// Bind pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

	// Bind descriptor set if present
	if (pipeline->descriptorSet != VK_NULL_HANDLE) {
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		                        pipeline->layout, 0, 1, &pipeline->descriptorSet, 0, NULL);
	}

	// Dispatch
	vkCmdDispatch(cmdBuffer, groupCountX, groupCountY, groupCountZ);
}

int nvgvk_update_compute_descriptors(NVGVkContext* vk,
                                      NVGVkComputePipeline* pipeline,
                                      uint32_t binding,
                                      VkDescriptorType type,
                                      VkBuffer buffer,
                                      VkDeviceSize offset,
                                      VkDeviceSize range) {
	if (!vk || !pipeline || !pipeline->valid) return 0;

	VkDescriptorBufferInfo bufferInfo = {
		.buffer = buffer,
		.offset = offset,
		.range = range
	};

	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = pipeline->descriptorSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = &bufferInfo
	};

	vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
	return 1;
}

int nvgvk_update_compute_image_descriptor(NVGVkContext* vk,
                                           NVGVkComputePipeline* pipeline,
                                           uint32_t binding,
                                           VkImageView imageView,
                                           VkImageLayout layout) {
	if (!vk || !pipeline || !pipeline->valid) return 0;

	VkDescriptorImageInfo imageInfo = {
		.imageView = imageView,
		.imageLayout = layout
	};

	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = pipeline->descriptorSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imageInfo
	};

	vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
	return 1;
}
