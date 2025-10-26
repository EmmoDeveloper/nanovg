#include "nvg_vk_pipeline.h"
#include "nvg_vk_shader.h"
#include <stdio.h>
#include <string.h>

// Helper: Create descriptor set layout for textures and uniforms
static int nvgvk__create_descriptor_layout(NVGVkContext* vk, NVGVkPipeline* pipeline)
{
	VkDescriptorSetLayoutBinding bindings[2] = {0};

	// Binding 0: Uniform buffer (viewSize)
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Binding 1: Combined image sampler (texture)
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &pipeline->descriptorSetLayout) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create descriptor set layout\n");
		return 0;
	}

	return 1;
}

// Helper: Create pipeline layout with push constants
static int nvgvk__create_pipeline_layout(NVGVkContext* vk, NVGVkPipeline* pipeline)
{
	// Push constant range for fragment uniforms (accessible from both stages)
	VkPushConstantRange pushConstant = {0};
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(NVGVkFragUniforms);

	VkPipelineLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &pipeline->descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushConstant;

	if (vkCreatePipelineLayout(vk->device, &layoutInfo, NULL, &pipeline->layout) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create pipeline layout\n");
		return 0;
	}

	return 1;
}

// Helper: Create graphics pipeline
static int nvgvk__create_graphics_pipeline(NVGVkContext* vk, NVGVkPipeline* pipeline,
                                            VkRenderPass renderPass, NVGVkShaderSet* shaders,
                                            int useStencil)
{
	// Vertex input state
	VkVertexInputBindingDescription binding = {0};
	binding.binding = 0;
	binding.stride = sizeof(NVGvertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrs[2] = {0};
	// Position
	attrs[0].location = 0;
	attrs[0].binding = 0;
	attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrs[0].offset = 0;
	// TexCoord
	attrs[1].location = 1;
	attrs[1].binding = 0;
	attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrs[1].offset = sizeof(float) * 2;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attrs;

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport state (dynamic)
	VkPipelineViewportStateCreateInfo viewportState = {0};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	// Multisample
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth/Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	if (useStencil) {
		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
		depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
		depthStencil.front.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
		depthStencil.front.compareMask = 0xFF;
		depthStencil.front.writeMask = 0xFF;
		depthStencil.back = depthStencil.front;
	}

	// Color blend
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	// Dynamic state
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = {0};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Shader stages
	VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = shaders->vertShader;
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = shaders->fragShader;
	shaderStages[1].pName = "main";

	// Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipeline->layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline->pipeline) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create graphics pipeline\n");
		return 0;
	}

	return 1;
}

int nvgvk_create_pipelines(NVGVkContext* vk, VkRenderPass renderPass)
{
	if (!vk || !renderPass) {
		return 0;
	}

	// Create shaders first
	if (!nvgvk_create_shaders(vk)) {
		return 0;
	}

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[2] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = NVGVK_PIPELINE_COUNT;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = NVGVK_PIPELINE_COUNT;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = NVGVK_PIPELINE_COUNT;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(vk->device, &poolInfo, NULL, &vk->pipelines[0].descriptorPool) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create descriptor pool\n");
		nvgvk_destroy_shaders(vk);
		return 0;
	}

	// Create pipelines
	for (int i = 0; i < NVGVK_PIPELINE_COUNT; i++) {
		NVGVkPipeline* pipeline = &vk->pipelines[i];

		// Descriptor set layout
		if (!nvgvk__create_descriptor_layout(vk, pipeline)) {
			nvgvk_destroy_pipelines(vk);
			return 0;
		}

		// Pipeline layout
		if (!nvgvk__create_pipeline_layout(vk, pipeline)) {
			nvgvk_destroy_pipelines(vk);
			return 0;
		}

		// Graphics pipeline
		int useStencil = (i == NVGVK_PIPELINE_FILL_GRAD || i == NVGVK_PIPELINE_FILL_IMG);
		if (!nvgvk__create_graphics_pipeline(vk, pipeline, renderPass, &vk->shaders[i], useStencil)) {
			nvgvk_destroy_pipelines(vk);
			return 0;
		}

		// Allocate descriptor set
		VkDescriptorSetAllocateInfo allocInfo = {0};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = vk->pipelines[0].descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &pipeline->descriptorSetLayout;

		if (vkAllocateDescriptorSets(vk->device, &allocInfo, &pipeline->descriptorSet) != VK_SUCCESS) {
			fprintf(stderr, "NanoVG Vulkan: Failed to allocate descriptor set\n");
			nvgvk_destroy_pipelines(vk);
			return 0;
		}
	}

	printf("NanoVG Vulkan: Created %d pipelines\n", NVGVK_PIPELINE_COUNT);
	return 1;
}

void nvgvk_destroy_pipelines(NVGVkContext* vk)
{
	if (!vk) {
		return;
	}

	vkDeviceWaitIdle(vk->device);

	for (int i = 0; i < NVGVK_PIPELINE_COUNT; i++) {
		NVGVkPipeline* pipeline = &vk->pipelines[i];

		if (pipeline->pipeline) {
			vkDestroyPipeline(vk->device, pipeline->pipeline, NULL);
			pipeline->pipeline = VK_NULL_HANDLE;
		}
		if (pipeline->layout) {
			vkDestroyPipelineLayout(vk->device, pipeline->layout, NULL);
			pipeline->layout = VK_NULL_HANDLE;
		}
		if (pipeline->descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptorSetLayout, NULL);
			pipeline->descriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	if (vk->pipelines[0].descriptorPool) {
		vkDestroyDescriptorPool(vk->device, vk->pipelines[0].descriptorPool, NULL);
		vk->pipelines[0].descriptorPool = VK_NULL_HANDLE;
	}

	nvgvk_destroy_shaders(vk);
}

void nvgvk_bind_pipeline(NVGVkContext* vk, NVGVkPipelineType type)
{
	if (!vk || type >= NVGVK_PIPELINE_COUNT) {
		return;
	}

	vk->currentPipeline = type;
	vkCmdBindPipeline(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelines[type].pipeline);
	vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        vk->pipelines[type].layout, 0, 1, &vk->pipelines[type].descriptorSet, 0, NULL);
}
