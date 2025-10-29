#include "nvg_vk_render.h"
#include "nvg_vk_pipeline.h"
#include <stdio.h>
#include <string.h>

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

// Setup render (updates all descriptor sets BEFORE command buffer recording)
void nvgvk_setup_render(NVGVkContext* vk)
{
	// Update descriptor sets for all pipelines
	for (int i = 0; i < NVGVK_PIPELINE_COUNT; i++) {
		nvgvk__update_descriptors(vk, -1, &vk->pipelines[i]);
	}
}

// Begin render pass and track state
void nvgvk_begin_render_pass(NVGVkContext* vk, VkRenderPass renderPass, VkFramebuffer framebuffer,
                              VkRect2D renderArea, const VkClearValue* clearValues, uint32_t clearValueCount,
                              VkViewport viewport, VkRect2D scissor)
{
	if (!vk || vk->inRenderPass) {
		return;
	}

	vk->activeRenderPass = renderPass;
	vk->activeFramebuffer = framebuffer;
	vk->renderArea = renderArea;
	vk->clearValueCount = clearValueCount < 2 ? clearValueCount : 2;

	// Copy clear values
	for (uint32_t i = 0; i < vk->clearValueCount; i++) {
		vk->clearValues[i] = clearValues[i];
	}

	// Store viewport and scissor
	vk->viewport = viewport;
	vk->scissor = scissor;

	vk->inRenderPass = 1;
}

// End render pass and clear state
void nvgvk_end_render_pass(NVGVkContext* vk)
{
	if (!vk || !vk->inRenderPass) {
		return;
	}

	vkCmdEndRenderPass(vk->commandBuffer);
	vk->inRenderPass = 0;
	vk->activeRenderPass = VK_NULL_HANDLE;
	vk->activeFramebuffer = VK_NULL_HANDLE;
}

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

void nvgvk_render_fill(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->pathCount == 0) {
		return;
	}

	// Fill rendering uses stencil-then-cover approach with two pipelines:
	// Pass 1: Write to stencil buffer (color writes disabled)
	// Pass 2: Draw cover quad where stencil != 0 (color writes enabled)

	// Get uniforms
	NVGVkFragUniforms* frag = (NVGVkFragUniforms*)(&vk->uniforms[call->uniformOffset].scissorMat);

	// === PASS 1: Stencil Write ===
	// Bind stencil write pipeline (color writes disabled)
	nvgvk_bind_pipeline(vk, NVGVK_PIPELINE_FILL_STENCIL);
	NVGVkPipeline* stencilPipeline = &vk->pipelines[NVGVK_PIPELINE_FILL_STENCIL];

	// Push constants
	vkCmdPushConstants(vk->commandBuffer, stencilPipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	// Set stencil reference/masks (pipeline has stencil ops configured)
	vkCmdSetStencilReference(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
	vkCmdSetStencilCompareMask(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);
	vkCmdSetStencilWriteMask(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);

	// Draw all path fill geometry to stencil
	for (int i = 0; i < call->pathCount; i++) {
		NVGVkPath* path = &vk->paths[call->pathOffset + i];
		if (path->fillCount > 0) {
			vkCmdDraw(vk->commandBuffer, path->fillCount, 1, path->fillOffset, 0);
		}
	}

	// === PASS 2: Cover with Color ===
	// Bind cover pipeline based on image vs gradient (color writes enabled, stencil test)
	// Note: image > 0 means texture, image == 0 means solid color or gradient
	NVGVkPipelineType coverPipeline = (call->image > 0) ?
		NVGVK_PIPELINE_FILL_COVER_IMG : NVGVK_PIPELINE_FILL_COVER_GRAD;
	nvgvk_bind_pipeline(vk, coverPipeline);
	NVGVkPipeline* pipeline = &vk->pipelines[coverPipeline];

	// Bind texture descriptor set if using image
	if (call->image > 0) {
		int texId = call->image - 1;
		if (texId >= 0 && texId < NVGVK_MAX_TEXTURES && vk->textures[texId].image != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			                        pipeline->layout, 0, 1, &vk->textures[texId].descriptorSet, 0, NULL);
		}
	} else {
		// Bind pipeline's default descriptor set for gradients
		vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline->layout, 0, 1, &pipeline->descriptorSet, 0, NULL);
	}

	// Push constants (same uniforms for cover pass)
	vkCmdPushConstants(vk->commandBuffer, pipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	// Set stencil test parameters (pipeline configured for NOT_EQUAL test)
	vkCmdSetStencilReference(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
	vkCmdSetStencilCompareMask(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);
	vkCmdSetStencilWriteMask(vk->commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFF);

	// Draw bounding quad (only pixels where stencil != 0 will be drawn)
	if (call->triangleCount >= 3) {
		vkCmdDraw(vk->commandBuffer, call->triangleCount, 1, call->triangleOffset, 0);
	}

	// === PASS 3: AA Fringe ===
	// Draw anti-aliasing fringe (triangle strip around edges)
	nvgvk_bind_pipeline(vk, NVGVK_PIPELINE_FRINGE);
	NVGVkPipeline* fringePipeline = &vk->pipelines[NVGVK_PIPELINE_FRINGE];

	// Push constants (same uniforms)
	vkCmdPushConstants(vk->commandBuffer, fringePipeline->layout,
	                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                   0, sizeof(NVGVkFragUniforms), frag);

	// Draw fringe for all paths
	for (int i = 0; i < call->pathCount; i++) {
		NVGVkPath* path = &vk->paths[call->pathOffset + i];
		if (path->strokeCount > 0) {
			vkCmdDraw(vk->commandBuffer, path->strokeCount, 1, path->strokeOffset, 0);
		}
	}
}

void nvgvk_render_convex_fill(NVGVkContext* vk, NVGVkCall* call)
{
	if (!vk || !call || call->pathCount == 0) {
		return;
	}

	// Convex fills can be drawn directly without stencil - use SIMPLE pipeline
	NVGVkPipelineType pipelineType = (call->image >= 0) ? NVGVK_PIPELINE_IMG : NVGVK_PIPELINE_SIMPLE;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];
	nvgvk_bind_pipeline(vk, pipelineType);

	// Bind texture descriptor set if using image
	if (call->image > 0) {
		int texId = call->image - 1;
		if (texId >= 0 && texId < NVGVK_MAX_TEXTURES && vk->textures[texId].image != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			                        pipeline->layout, 0, 1, &vk->textures[texId].descriptorSet, 0, NULL);
		}
	} else {
		// Bind pipeline's default descriptor set
		vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline->layout, 0, 1, &pipeline->descriptorSet, 0, NULL);
	}

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

	// Strokes use FRINGE pipeline (triangle strip with AA support)
	NVGVkPipelineType pipelineType = NVGVK_PIPELINE_FRINGE;
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];
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

	// Triangle rendering - choose pipeline based on texture type
	NVGVkPipelineType pipelineType = NVGVK_PIPELINE_SIMPLE;
	if (call->image > 0) {
		int texId = call->image - 1;
		// Check if texture is MSDF type (type 3 = NVG_TEXTURE_MSDF)
		if (texId >= 0 && texId < NVGVK_MAX_TEXTURES && vk->textures[texId].type == 3) {
			pipelineType = NVGVK_PIPELINE_TEXT_MSDF;
		} else {
			pipelineType = NVGVK_PIPELINE_IMG;
		}
	}
	NVGVkPipeline* pipeline = &vk->pipelines[pipelineType];

	nvgvk_bind_pipeline(vk, pipelineType);

	// Bind texture descriptor set if using image
	if (call->image > 0) {
		int texId = call->image - 1;
		if (texId >= 0 && texId < NVGVK_MAX_TEXTURES && vk->textures[texId].image != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			                        pipeline->layout, 0, 1, &vk->textures[texId].descriptorSet, 0, NULL);
		}
	} else {
		// Bind pipeline's default descriptor set
		vkCmdBindDescriptorSets(vk->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        pipeline->layout, 0, 1, &pipeline->descriptorSet, 0, NULL);
	}

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
