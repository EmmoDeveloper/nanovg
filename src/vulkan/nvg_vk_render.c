#include "nvg_vk_render.h"
#include "nvg_vk_pipeline.h"
#include <stdio.h>
#include <string.h>

// Helper: Set stencil test state
static void nvgvk__set_stencil_test(VkCommandBuffer cmd, int enable, VkCompareOp compareOp,
                                     VkStencilOp passOp, uint32_t ref, uint32_t compareMask, uint32_t writeMask)
{
	// Stencil state is set in pipeline, but reference value is dynamic
	if (enable) {
		vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, ref);
		vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, compareMask);
		vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, writeMask);
	}
}

// Helper: Update descriptor set with texture
static void nvgvk__update_descriptors(NVGVkContext* vk, int texId, NVGVkPipeline* pipeline)
{
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = vk->uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(float) * 2; // viewSize

	VkDescriptorImageInfo imageInfo = {0};
	if (texId >= 0 && texId < NVGVK_MAX_TEXTURES && vk->textures[texId].image != VK_NULL_HANDLE) {
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vk->textures[texId].imageView;
		imageInfo.sampler = vk->textures[texId].sampler;
	} else {
		// Use first texture as dummy if no valid texture
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vk->textures[0].imageView;
		imageInfo.sampler = vk->textures[0].sampler;
	}

	VkWriteDescriptorSet writes[2] = {0};

	// Uniform buffer
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = pipeline->descriptorSet;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &bufferInfo;

	// Texture sampler
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = pipeline->descriptorSet;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(vk->device, 2, writes, 0, NULL);
}

void nvgvk_render_fill(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->pathCount == 0) {
		return;
	}

	// Fill rendering uses stencil-then-cover approach:
	// 1. Draw paths to stencil buffer
	// 2. Draw bounding quad with stencil test

	// Get the pipeline based on image vs gradient
	NVGVkPipelineType pipelineType = (call->image >= 0) ? NVGVK_PIPELINE_FILL_IMG : NVGVK_PIPELINE_FILL_GRAD;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];

	// Update descriptors before binding
	nvgvk__update_descriptors(vk, call->image, pipeline);
	nvgvk_bind_pipeline(vk, pipelineType);

	// Get uniforms
	// Skip viewSize (2 floats) to get FragUniforms
	NVGVkFragUniforms* frag = (NVGVkFragUniforms*)(&vk->uniforms[call->uniformOffset].scissorMat);

	// Push constants
	vkCmdPushConstants(vk->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	// Step 1: Draw to stencil
	nvgvk__set_stencil_test(vk->commandBuffer, 1, VK_COMPARE_OP_ALWAYS,
	                         VK_STENCIL_OP_INCREMENT_AND_WRAP, 0, 0xFF, 0xFF);

	for (int i = 0; i < call->pathCount; i++) {
		NVGVkPath* path = &vk->paths[call->pathOffset + i];
		if (path->fillCount > 0) {
			vkCmdDraw(vk->commandBuffer, path->fillCount, 1, path->fillOffset, 0);
		}
	}

	// Step 2: Draw cover with stencil test
	nvgvk__set_stencil_test(vk->commandBuffer, 1, VK_COMPARE_OP_NOT_EQUAL,
	                         VK_STENCIL_OP_ZERO, 0, 0xFF, 0xFF);

	// Draw quad covering affected area
	if (call->triangleCount >= 4) {
		vkCmdDraw(vk->commandBuffer, call->triangleCount, 1, call->triangleOffset, 0);
	}

	// Reset stencil
	nvgvk__set_stencil_test(vk->commandBuffer, 0, VK_COMPARE_OP_ALWAYS,
	                         VK_STENCIL_OP_KEEP, 0, 0xFF, 0xFF);
}

void nvgvk_render_convex_fill(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->pathCount == 0) {
		return;
	}

	// Convex fills can be drawn directly without stencil
	NVGVkPipelineType pipelineType = (call->image >= 0) ? NVGVK_PIPELINE_FILL_IMG : NVGVK_PIPELINE_FILL_GRAD;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];

	nvgvk__update_descriptors(vk, call->image, pipeline);
	nvgvk_bind_pipeline(vk, pipelineType);

	// Skip viewSize (2 floats) to get FragUniforms
	NVGVkFragUniforms* frag = (NVGVkFragUniforms*)(&vk->uniforms[call->uniformOffset].scissorMat);
	vkCmdPushConstants(vk->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	for (int i = 0; i < call->pathCount; i++) {
		NVGVkPath* path = &vk->paths[call->pathOffset + i];
		if (path->fillCount > 0) {
			vkCmdDraw(vk->commandBuffer, path->fillCount, 1, path->fillOffset, 0);
		}
	}
}

void nvgvk_render_stroke(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->pathCount == 0) {
		return;
	}

	// Strokes are rendered as triangle strips
	NVGVkPipelineType pipelineType = (call->image >= 0) ? NVGVK_PIPELINE_FILL_IMG : NVGVK_PIPELINE_FILL_GRAD;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];

	nvgvk__update_descriptors(vk, call->image, pipeline);
	nvgvk_bind_pipeline(vk, pipelineType);

	// Skip viewSize (2 floats) to get FragUniforms
	NVGVkFragUniforms* frag = (NVGVkFragUniforms*)(&vk->uniforms[call->uniformOffset].scissorMat);
	vkCmdPushConstants(vk->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	for (int i = 0; i < call->pathCount; i++) {
		NVGVkPath* path = &vk->paths[call->pathOffset + i];
		if (path->strokeCount > 0) {
			vkCmdDraw(vk->commandBuffer, path->strokeCount, 1, path->strokeOffset, 0);
		}
	}
}

void nvgvk_render_triangles(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->triangleCount == 0) {
		return;
	}

	// Simple triangle rendering
	NVGVkPipelineType pipelineType = (call->image >= 0) ? NVGVK_PIPELINE_IMG : NVGVK_PIPELINE_SIMPLE;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];

	// Update descriptors before binding
	nvgvk__update_descriptors(vk, call->image, pipeline);
	nvgvk_bind_pipeline(vk, pipelineType);

	// Skip viewSize (2 floats) to get FragUniforms
	NVGVkFragUniforms* frag = (NVGVkFragUniforms*)(&vk->uniforms[call->uniformOffset].scissorMat);

	vkCmdPushConstants(vk->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	vkCmdDraw(vk->commandBuffer, call->triangleCount, 1, call->triangleOffset, 0);
}

void nvgvk_get_blend_factors(int blendFunc, VkBlendFactor* srcColor, VkBlendFactor* dstColor,
                              VkBlendFactor* srcAlpha, VkBlendFactor* dstAlpha)
{
	// NanoVG blend modes
	switch (blendFunc) {
		case 0: // Source over
			*srcColor = VK_BLEND_FACTOR_ONE;
			*dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ONE;
			*dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case 1: // Source in
			*srcColor = VK_BLEND_FACTOR_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_ZERO;
			*srcAlpha = VK_BLEND_FACTOR_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_ZERO;
			break;
		case 2: // Source out
			*srcColor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_ZERO;
			*srcAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_ZERO;
			break;
		case 3: // Atop
			*srcColor = VK_BLEND_FACTOR_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case 4: // Destination over
			*srcColor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_ONE;
			*srcAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_ONE;
			break;
		case 5: // Destination in
			*srcColor = VK_BLEND_FACTOR_ZERO;
			*dstColor = VK_BLEND_FACTOR_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ZERO;
			*dstAlpha = VK_BLEND_FACTOR_SRC_ALPHA;
			break;
		case 6: // Destination out
			*srcColor = VK_BLEND_FACTOR_ZERO;
			*dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ZERO;
			*dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case 7: // Destination atop
			*srcColor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_SRC_ALPHA;
			break;
		case 8: // Lighter
			*srcColor = VK_BLEND_FACTOR_ONE;
			*dstColor = VK_BLEND_FACTOR_ONE;
			*srcAlpha = VK_BLEND_FACTOR_ONE;
			*dstAlpha = VK_BLEND_FACTOR_ONE;
			break;
		case 9: // Copy
			*srcColor = VK_BLEND_FACTOR_ONE;
			*dstColor = VK_BLEND_FACTOR_ZERO;
			*srcAlpha = VK_BLEND_FACTOR_ONE;
			*dstAlpha = VK_BLEND_FACTOR_ZERO;
			break;
		case 10: // XOR
			*srcColor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			*dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		default:
			*srcColor = VK_BLEND_FACTOR_ONE;
			*dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			*srcAlpha = VK_BLEND_FACTOR_ONE;
			*dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
	}
}
