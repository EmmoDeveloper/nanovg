// Compute Shader Support for NanoVG Vulkan Implementation
// GPU-accelerated operations: atlas defragmentation, etc.

#include "nanovg_vk_compute.h"
#include <stdio.h>
#include <string.h>

// Include compiled shader SPIR-V
#include "vulkan/atlas_defrag_comp.h"

// Create shader module from SPIR-V
VkShaderModule vknvg__createComputeShaderModule(VkDevice device,
                                                 const uint32_t* code,
                                                 size_t codeSize)
{
	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = codeSize;
	createInfo.pCode = code;

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

// Create atlas defragmentation compute pipeline
int vknvg__createDefragPipeline(VKNVGcomputeContext* ctx)
{
	VKNVGcomputePipeline* pipeline = &ctx->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG];

	// Create shader module
	pipeline->shaderModule = vknvg__createComputeShaderModule(
		ctx->device,
		(const uint32_t*)shaders_atlas_defrag_spv,
		sizeof(shaders_atlas_defrag_spv)
	);
	if (pipeline->shaderModule == VK_NULL_HANDLE) {
		fprintf(stderr, "Failed to create defrag compute shader module\n");
		return 0;
	}

	// Descriptor set layout (2 storage images)
	VkDescriptorSetLayoutBinding bindings[2] = {0};

	// Binding 0: Source atlas (readonly)
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	// Binding 1: Destination atlas (writeonly)
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL,
	                                  &pipeline->descriptorSetLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create compute descriptor set layout\n");
		vkDestroyShaderModule(ctx->device, pipeline->shaderModule, NULL);
		return 0;
	}

	// Push constant range
	VkPushConstantRange pushConstantRange = {0};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(VKNVGdefragPushConstants);

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &pipeline->descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL,
	                             &pipeline->layout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create compute pipeline layout\n");
		vkDestroyDescriptorSetLayout(ctx->device, pipeline->descriptorSetLayout, NULL);
		vkDestroyShaderModule(ctx->device, pipeline->shaderModule, NULL);
		return 0;
	}

	// Compute shader stage
	VkPipelineShaderStageCreateInfo shaderStageInfo = {0};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = pipeline->shaderModule;
	shaderStageInfo.pName = "main";

	// Compute pipeline
	VkComputePipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageInfo;
	pipelineInfo.layout = pipeline->layout;

	if (vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo,
	                               NULL, &pipeline->pipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create compute pipeline\n");
		vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
		vkDestroyDescriptorSetLayout(ctx->device, pipeline->descriptorSetLayout, NULL);
		vkDestroyShaderModule(ctx->device, pipeline->shaderModule, NULL);
		return 0;
	}

	// Create descriptor pool
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize.descriptorCount = 2;  // 2 storage images

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(ctx->device, &poolInfo, NULL,
	                             &pipeline->descriptorPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create compute descriptor pool\n");
		vkDestroyPipeline(ctx->device, pipeline->pipeline, NULL);
		vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
		vkDestroyDescriptorSetLayout(ctx->device, pipeline->descriptorSetLayout, NULL);
		vkDestroyShaderModule(ctx->device, pipeline->shaderModule, NULL);
		return 0;
	}

	return 1;
}

// Initialize compute context
int vknvg__initComputeContext(VKNVGcomputeContext* ctx,
                               VkDevice device,
                               VkQueue queue,
                               uint32_t queueFamilyIndex)
{
	memset(ctx, 0, sizeof(*ctx));

	ctx->device = device;
	ctx->computeQueue = queue;
	ctx->queueFamilyIndex = queueFamilyIndex;

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create compute command pool\n");
		return 0;
	}

	// Allocate command buffer
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = ctx->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &ctx->commandBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate compute command buffer\n");
		vkDestroyCommandPool(device, ctx->commandPool, NULL);
		return 0;
	}

	// Create pipelines
	if (!vknvg__createDefragPipeline(ctx)) {
		vkFreeCommandBuffers(device, ctx->commandPool, 1, &ctx->commandBuffer);
		vkDestroyCommandPool(device, ctx->commandPool, NULL);
		return 0;
	}

	ctx->initialized = 1;
	return 1;
}

// Destroy compute context
void vknvg__destroyComputeContext(VKNVGcomputeContext* ctx)
{
	if (!ctx->initialized) {
		return;
	}

	// Destroy pipelines
	for (int i = 0; i < VKNVG_COMPUTE_PIPELINE_COUNT; i++) {
		VKNVGcomputePipeline* pipeline = &ctx->pipelines[i];
		if (pipeline->descriptorPool) {
			vkDestroyDescriptorPool(ctx->device, pipeline->descriptorPool, NULL);
		}
		if (pipeline->pipeline) {
			vkDestroyPipeline(ctx->device, pipeline->pipeline, NULL);
		}
		if (pipeline->layout) {
			vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
		}
		if (pipeline->descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(ctx->device, pipeline->descriptorSetLayout, NULL);
		}
		if (pipeline->shaderModule) {
			vkDestroyShaderModule(ctx->device, pipeline->shaderModule, NULL);
		}
	}

	if (ctx->commandBuffer) {
		vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &ctx->commandBuffer);
	}
	if (ctx->commandPool) {
		vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
	}

	ctx->initialized = 0;
}

// Dispatch atlas defragmentation compute shader
void vknvg__dispatchDefragCompute(VKNVGcomputeContext* ctx,
                                   VkDescriptorSet descriptorSet,
                                   const VKNVGdefragPushConstants* pushConstants)
{
	VKNVGcomputePipeline* pipeline = &ctx->pipelines[VKNVG_COMPUTE_ATLAS_DEFRAG];

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(ctx->commandBuffer, &beginInfo);

	// Bind pipeline
	vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

	// Bind descriptor set
	vkCmdBindDescriptorSets(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
	                        pipeline->layout, 0, 1, &descriptorSet, 0, NULL);

	// Push constants
	vkCmdPushConstants(ctx->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_COMPUTE_BIT, 0,
	                   sizeof(VKNVGdefragPushConstants), pushConstants);

	// Dispatch compute (8x8 local size, so divide by 8 and round up)
	uint32_t groupsX = (pushConstants->extentWidth + 7) / 8;
	uint32_t groupsY = (pushConstants->extentHeight + 7) / 8;
	vkCmdDispatch(ctx->commandBuffer, groupsX, groupsY, 1);

	// End command buffer
	vkEndCommandBuffer(ctx->commandBuffer);

	// Submit
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &ctx->commandBuffer;

	vkQueueSubmit(ctx->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx->computeQueue);  // Simple synchronization for now
}
