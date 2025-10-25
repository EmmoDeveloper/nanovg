//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
// Copyright (c) 2025 NanoVG Vulkan Backend Contributors
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef NANOVG_VK_PIPELINE_H
#define NANOVG_VK_PIPELINE_H

#include "nanovg_vk_internal.h"

static VkShaderModule vknvg__createShaderModule(VkDevice device, const unsigned char* code, size_t codeSize)
{
	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = codeSize;
	createInfo.pCode = (const uint32_t*)code;

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

#include "../shaders/fill.vert.spv.h"
#include "../shaders/fill.frag.spv.h"
#include "../shaders/text_instanced.vert.spv.h"
#include "../shaders/text_sdf.frag.spv.h"
#include "../shaders/text_subpixel.frag.spv.h"
#include "../shaders/text_msdf.frag.spv.h"
#include "../shaders/text_color.frag.spv.h"
#include "../shaders/text_dual.vert.spv.h"
#include "../shaders/text_dual.frag.spv.h"
#include "../shaders/tessellate.comp.spv.h"

#define vknvg__vertShaderSpv fill_vert_spv
#define vknvg__vertShaderSize sizeof(fill_vert_spv)

#define vknvg__textInstancedVertShaderSpv shaders_text_instanced_vert_spv
#define vknvg__textInstancedVertShaderSize sizeof(shaders_text_instanced_vert_spv)

#define vknvg__computeShaderSpv tessellate_comp_spv
#define vknvg__computeShaderSize sizeof(tessellate_comp_spv)

#define vknvg__fragShaderSpv fill_frag_spv
#define vknvg__fragShaderSize sizeof(fill_frag_spv)

#define vknvg__textSDFFragShaderSpv text_sdf_frag_spv
#define vknvg__textSDFFragShaderSize sizeof(text_sdf_frag_spv)

#define vknvg__textSubpixelFragShaderSpv text_subpixel_frag_spv
#define vknvg__textSubpixelFragShaderSize sizeof(text_subpixel_frag_spv)

#define vknvg__textMSDFFragShaderSpv text_msdf_frag_spv
#define vknvg__textMSDFFragShaderSize sizeof(text_msdf_frag_spv)

#define vknvg__textColorFragShaderSpv text_color_frag_spv
#define vknvg__textColorFragShaderSize sizeof(text_color_frag_spv)

#define vknvg__textDualModeVertShaderSpv shaders_text_dual_vert_spv
#define vknvg__textDualModeVertShaderSize sizeof(shaders_text_dual_vert_spv)

#define vknvg__textDualModeFragShaderSpv shaders_text_dual_frag_spv
#define vknvg__textDualModeFragShaderSize sizeof(shaders_text_dual_frag_spv)

// Common pipeline state initialization helpers
static void vknvg__initCommonPipelineStates(VKNVGcontext* vk,
                                             VkPipelineViewportStateCreateInfo* viewportState,
                                             VkPipelineRasterizationStateCreateInfo* rasterizer,
                                             VkPipelineMultisampleStateCreateInfo* multisampling,
                                             VkPipelineDepthStencilStateCreateInfo* depthStencil,
                                             VkPipelineColorBlendAttachmentState* colorBlendAttachment,
                                             VkPipelineColorBlendStateCreateInfo* colorBlending,
                                             VkPipelineDynamicStateCreateInfo* dynamicState,
                                             VkDynamicState* dynamicStates)
{
	viewportState->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState->viewportCount = 1;
	viewportState->scissorCount = 1;

	rasterizer->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer->depthClampEnable = VK_FALSE;
	rasterizer->rasterizerDiscardEnable = VK_FALSE;
	rasterizer->polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer->lineWidth = 1.0f;
	rasterizer->cullMode = VK_CULL_MODE_NONE;
	rasterizer->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer->depthBiasEnable = VK_FALSE;

	multisampling->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling->sampleShadingEnable = VK_FALSE;
	multisampling->rasterizationSamples = vk->sampleCount;

	depthStencil->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil->depthTestEnable = VK_FALSE;
	depthStencil->depthWriteEnable = VK_FALSE;
	depthStencil->stencilTestEnable = VK_FALSE;

	colorBlendAttachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment->blendEnable = VK_TRUE;
	colorBlendAttachment->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment->colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment->alphaBlendOp = VK_BLEND_OP_ADD;

	colorBlending->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending->logicOpEnable = VK_FALSE;
	colorBlending->attachmentCount = 1;
	colorBlending->pAttachments = colorBlendAttachment;

	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
	dynamicState->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState->dynamicStateCount = 2;
	dynamicState->pDynamicStates = dynamicStates;
}

static void vknvg__initPipelineCreateInfo(VKNVGcontext* vk,
                                           VkGraphicsPipelineCreateInfo* pipelineInfo,
                                           VkPipelineRenderingCreateInfoKHR* renderingCreateInfo)
{
	pipelineInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo->basePipelineHandle = VK_NULL_HANDLE;

	if (vk->hasDynamicRendering) {
		renderingCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		renderingCreateInfo->colorAttachmentCount = 1;
		renderingCreateInfo->pColorAttachmentFormats = &vk->colorFormat;
		renderingCreateInfo->depthAttachmentFormat = vk->depthStencilFormat;
		renderingCreateInfo->stencilAttachmentFormat = vk->depthStencilFormat;
		pipelineInfo->pNext = renderingCreateInfo;
		pipelineInfo->renderPass = VK_NULL_HANDLE;
		pipelineInfo->subpass = 0;
	} else {
		pipelineInfo->renderPass = vk->renderPass;
		pipelineInfo->subpass = vk->subpass;
	}
}

static VkResult vknvg__createDescriptorSetLayout(VKNVGcontext* vk)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	return vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &vk->descriptorSetLayout);
}

static VkResult vknvg__createPipelineLayout(VKNVGcontext* vk)
{
	VkPushConstantRange pushConstantRange = {0};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 2;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &vk->descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	return vkCreatePipelineLayout(vk->device, &pipelineLayoutInfo, NULL, &vk->pipelineLayout);
}

static VkResult vknvg__createComputeDescriptorSetLayout(VKNVGcontext* vk)
{
	VkDescriptorSetLayoutBinding bindings[4] = {0};

	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[3].binding = 3;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 4;
	layoutInfo.pBindings = bindings;

	return vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &vk->computeDescriptorSetLayout);
}

static VkResult vknvg__createComputePipelineLayout(VKNVGcontext* vk)
{
	VkPushConstantRange pushConstantRange = {0};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t) * 3 + sizeof(float);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &vk->computeDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	return vkCreatePipelineLayout(vk->device, &pipelineLayoutInfo, NULL, &vk->computePipelineLayout);
}

static VkResult vknvg__createComputePipeline(VKNVGcontext* vk)
{
	VkPipelineShaderStageCreateInfo shaderStage = {0};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = vk->computeShaderModule;
	shaderStage.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStage;
	pipelineInfo.layout = vk->computePipelineLayout;

	return vkCreateComputePipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vk->computePipeline);
}

static VkResult vknvg__createTextPipeline(VKNVGcontext* vk, VkPipeline* pipeline,
                                          VkShaderModule customFragShader)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vk->vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = customFragShader;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkVertexInputBindingDescription bindingDescription = {0};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(NVGvertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[2] = {0};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = 0;
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = sizeof(float) * 2;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {0};
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	VkDynamicState dynamicStates[2];
	VkPipelineDynamicStateCreateInfo dynamicState = {0};

	vknvg__initCommonPipelineStates(vk, &viewportState, &rasterizer, &multisampling, &depthStencil,
	                                 &colorBlendAttachment, &colorBlending, &dynamicState, dynamicStates);

	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {0};
	vknvg__initPipelineCreateInfo(vk, &pipelineInfo, &renderingCreateInfo);

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
	pipelineInfo.layout = vk->pipelineLayout;

	return vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
}

static VkResult vknvg__createInstancedTextPipeline(VKNVGcontext* vk, VkPipeline* pipeline,
                                                    VkShaderModule customFragShader)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vk->textInstancedVertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = customFragShader;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkVertexInputBindingDescription bindingDescription = vknvg__getInstanceBindingDescription();

	VkVertexInputAttributeDescription attributeDescriptions[4];
	vknvg__getInstanceAttributeDescriptions(attributeDescriptions);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 4;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {0};
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	VkDynamicState dynamicStates[2];
	VkPipelineDynamicStateCreateInfo dynamicState = {0};

	vknvg__initCommonPipelineStates(vk, &viewportState, &rasterizer, &multisampling, &depthStencil,
	                                 &colorBlendAttachment, &colorBlending, &dynamicState, dynamicStates);

	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {0};
	vknvg__initPipelineCreateInfo(vk, &pipelineInfo, &renderingCreateInfo);

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
	pipelineInfo.layout = vk->pipelineLayout;

	return vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
}

static VkResult vknvg__createGraphicsPipeline(VKNVGcontext* vk, VkPipeline* pipeline,
                                              VkBool32 enableStencil, VkStencilOp frontStencilOp,
                                              VkStencilOp backStencilOp, VkCompareOp stencilCompareOp,
                                              VkBool32 colorWriteEnable, VKNVGblend* blend)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vk->vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = vk->fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkVertexInputBindingDescription bindingDescription = {0};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(NVGvertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[2] = {0};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = 0;
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = sizeof(float) * 2;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {0};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk->sampleCount;

	VkStencilOpState frontStencilState = {0};
	frontStencilState.failOp = VK_STENCIL_OP_KEEP;
	frontStencilState.passOp = frontStencilOp;
	frontStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
	frontStencilState.compareOp = stencilCompareOp;
	frontStencilState.compareMask = 0xFF;
	frontStencilState.writeMask = 0xFF;
	frontStencilState.reference = 0;

	VkStencilOpState backStencilState = {0};
	backStencilState.failOp = VK_STENCIL_OP_KEEP;
	backStencilState.passOp = backStencilOp;
	backStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
	backStencilState.compareOp = stencilCompareOp;
	backStencilState.compareMask = 0xFF;
	backStencilState.writeMask = 0xFF;
	backStencilState.reference = 0;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.stencilTestEnable = enableStencil;
	depthStencil.front = frontStencilState;
	depthStencil.back = backStencilState;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	colorBlendAttachment.colorWriteMask = colorWriteEnable ?
		(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT) : 0;
	colorBlendAttachment.blendEnable = VK_TRUE;

	if (blend != NULL) {
		colorBlendAttachment.srcColorBlendFactor = blend->srcRGB;
		colorBlendAttachment.dstColorBlendFactor = blend->dstRGB;
		colorBlendAttachment.srcAlphaBlendFactor = blend->srcAlpha;
		colorBlendAttachment.dstAlphaBlendFactor = blend->dstAlpha;
	} else {
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkDynamicState dynamicStates[4];
	uint32_t dynamicStateCount = 0;
	dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

	if (vk->hasDynamicBlendState && colorWriteEnable) {
		dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT;
		dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT;
	}

	VkPipelineDynamicStateCreateInfo dynamicState = {0};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStateCount;
	dynamicState.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {0};
	if (vk->hasDynamicRendering) {
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		renderingCreateInfo.colorAttachmentCount = 1;
		renderingCreateInfo.pColorAttachmentFormats = &vk->colorFormat;
		renderingCreateInfo.depthAttachmentFormat = vk->depthStencilFormat;
		renderingCreateInfo.stencilAttachmentFormat = vk->depthStencilFormat;
		pipelineInfo.pNext = &renderingCreateInfo;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;
	} else {
		pipelineInfo.renderPass = vk->renderPass;
		pipelineInfo.subpass = vk->subpass;
	}

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
	pipelineInfo.layout = vk->pipelineLayout;

	return vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
}

// Dual-mode descriptor set layout (3 bindings: UBO, SDF sampler, color atlas sampler)
static VkResult vknvg__createDualModeDescriptorSetLayout(VKNVGcontext* vk)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding sdfSamplerBinding = {0};
	sdfSamplerBinding.binding = 1;
	sdfSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sdfSamplerBinding.descriptorCount = 1;
	sdfSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding colorAtlasSamplerBinding = {0};
	colorAtlasSamplerBinding.binding = 2;
	colorAtlasSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	colorAtlasSamplerBinding.descriptorCount = 1;
	colorAtlasSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, sdfSamplerBinding, colorAtlasSamplerBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 3;
	layoutInfo.pBindings = bindings;

	return vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &vk->dualModeDescriptorSetLayout);
}

// Dual-mode pipeline layout (uses dual-mode descriptor set layout)
static VkResult vknvg__createDualModePipelineLayout(VKNVGcontext* vk, VkPipelineLayout* pipelineLayout)
{
	VkPushConstantRange pushConstantRange = {0};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 2;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &vk->dualModeDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	return vkCreatePipelineLayout(vk->device, &pipelineLayoutInfo, NULL, pipelineLayout);
}

// Dual-mode text pipeline (instanced, with dual-mode shaders)
static VkResult vknvg__createDualModeTextPipeline(VKNVGcontext* vk, VkPipeline* pipeline,
                                                   VkPipelineLayout dualModePipelineLayout)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vk->textDualModeVertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = vk->textDualModeFragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// Vertex input: position, size, uvOffset, uvSize (from instance buffer)
	VkVertexInputBindingDescription bindingDescription = {0};
	bindingDescription.binding = 0;
	bindingDescription.stride = 32;  // vec2 pos + vec2 size + vec2 uvOff + vec2 uvSize = 8 floats
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	VkVertexInputAttributeDescription attributeDescriptions[4] = {0};
	// Position (location 0)
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = 0;
	// Size (location 1)
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = sizeof(float) * 2;
	// UV Offset (location 2)
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = sizeof(float) * 4;
	// UV Size (location 3)
	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[3].offset = sizeof(float) * 6;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 4;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {0};
	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	VkDynamicState dynamicStates[2];
	VkPipelineDynamicStateCreateInfo dynamicState = {0};

	vknvg__initCommonPipelineStates(vk, &viewportState, &rasterizer, &multisampling, &depthStencil,
	                                 &colorBlendAttachment, &colorBlending, &dynamicState, dynamicStates);

	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {0};
	vknvg__initPipelineCreateInfo(vk, &pipelineInfo, &renderingCreateInfo);

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
	pipelineInfo.layout = dualModePipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	return vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline);
}

#endif // NANOVG_VK_PIPELINE_H
