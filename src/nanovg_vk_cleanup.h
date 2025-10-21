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

#ifndef NANOVG_VK_CLEANUP_H
#define NANOVG_VK_CLEANUP_H

#include <stdlib.h>
#include "nanovg_vk_internal.h"
#include "nanovg_vk_memory.h"
#include "nanovg_vk_threading.h"

static void vknvg__renderDelete(void* uptr)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	if (vk == NULL) return;

	// Wait for device to be idle before cleanup
	vkDeviceWaitIdle(vk->device);

	// Cleanup multi-threading resources
	vknvg__destroyThreadPool(vk);

	// Cleanup textures
	if (vk->textures != NULL) {
		for (int i = 0; i < vk->ntextures; i++) {
			VKNVGtexture* tex = &vk->textures[i];
			if (tex->sampler != VK_NULL_HANDLE) {
				vkDestroySampler(vk->device, tex->sampler, NULL);
			}
			if (tex->imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(vk->device, tex->imageView, NULL);
			}
			if (tex->image != VK_NULL_HANDLE && !(tex->flags & 0x10000)) { // NVG_IMAGE_NODELETE
				vkDestroyImage(vk->device, tex->image, NULL);
			}
			if (tex->memory != VK_NULL_HANDLE) {
				vkFreeMemory(vk->device, tex->memory, NULL);
			}
		}
		free(vk->textures);
	}

	// Cleanup buffers
	if (vk->vertexBuffers != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			vknvg__destroyBuffer(vk, &vk->vertexBuffers[i]);
		}
		free(vk->vertexBuffers);
	}

	if (vk->uniformBuffer != NULL) {
		vknvg__destroyBuffer(vk, vk->uniformBuffer);
		free(vk->uniformBuffer);
	}

	// Cleanup glyph instance buffer
	if (vk->glyphInstanceBuffer != NULL) {
		vknvg__destroyGlyphInstanceBuffer(vk, vk->glyphInstanceBuffer);
		free(vk->glyphInstanceBuffer);
	}

	// Cleanup virtual atlas
	if (vk->virtualAtlas != NULL) {
		vknvg__destroyVirtualAtlas(vk->virtualAtlas);
		vk->virtualAtlas = NULL;
	}

	// Cleanup text run cache
	if (vk->textCache != NULL) {
		vknvg__destroyTextRunCache(vk->textCache);
		vk->textCache = NULL;
	}
	if (vk->textCacheRenderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(vk->device, vk->textCacheRenderPass, NULL);
		vk->textCacheRenderPass = VK_NULL_HANDLE;
	}

	// Cleanup pipelines
	if (vk->fillPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->fillPipeline, NULL);
	}
	if (vk->fillStencilPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->fillStencilPipeline, NULL);
	}
	if (vk->fillAAPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->fillAAPipeline, NULL);
	}
	if (vk->strokePipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->strokePipeline, NULL);
	}
	if (vk->trianglesPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->trianglesPipeline, NULL);
	}
	if (vk->textPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textPipeline, NULL);
	}
	if (vk->textSDFPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textSDFPipeline, NULL);
	}
	if (vk->textMSDFPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textMSDFPipeline, NULL);
	}
	if (vk->textSubpixelPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textSubpixelPipeline, NULL);
	}
	if (vk->textColorPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textColorPipeline, NULL);
	}
	if (vk->textInstancedPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textInstancedPipeline, NULL);
	}
	if (vk->textInstancedSDFPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textInstancedSDFPipeline, NULL);
	}
	if (vk->textInstancedSubpixelPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textInstancedSubpixelPipeline, NULL);
	}
	if (vk->textInstancedMSDFPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textInstancedMSDFPipeline, NULL);
	}
	if (vk->textInstancedColorPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->textInstancedColorPipeline, NULL);
	}

	// Cleanup pipeline variants
	if (vk->fillPipelineVariants != NULL) {
		for (int i = 0; i < vk->pipelineVariantCacheSize; i++) {
			VKNVGpipelineVariant* variant = vk->fillPipelineVariants[i];
			while (variant != NULL) {
				VKNVGpipelineVariant* next = variant->next;
				if (variant->pipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(vk->device, variant->pipeline, NULL);
				}
				free(variant);
				variant = next;
			}
		}
		free(vk->fillPipelineVariants);
	}
	if (vk->strokePipelineVariants != NULL) {
		for (int i = 0; i < vk->pipelineVariantCacheSize; i++) {
			VKNVGpipelineVariant* variant = vk->strokePipelineVariants[i];
			while (variant != NULL) {
				VKNVGpipelineVariant* next = variant->next;
				if (variant->pipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(vk->device, variant->pipeline, NULL);
				}
				free(variant);
				variant = next;
			}
		}
		free(vk->strokePipelineVariants);
	}
	if (vk->trianglesPipelineVariants != NULL) {
		for (int i = 0; i < vk->pipelineVariantCacheSize; i++) {
			VKNVGpipelineVariant* variant = vk->trianglesPipelineVariants[i];
			while (variant != NULL) {
				VKNVGpipelineVariant* next = variant->next;
				if (variant->pipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(vk->device, variant->pipeline, NULL);
				}
				free(variant);
				variant = next;
			}
		}
		free(vk->trianglesPipelineVariants);
	}

	// Cleanup pipeline layout and descriptor set layout
	if (vk->pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(vk->device, vk->pipelineLayout, NULL);
	}
	if (vk->descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(vk->device, vk->descriptorSetLayout, NULL);
	}

	// Cleanup shaders
	if (vk->vertShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->vertShaderModule, NULL);
	}
	if (vk->fragShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->fragShaderModule, NULL);
	}
	if (vk->textInstancedVertShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->textInstancedVertShaderModule, NULL);
	}
	if (vk->textSDFFragShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->textSDFFragShaderModule, NULL);
	}
	if (vk->textMSDFFragShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->textMSDFFragShaderModule, NULL);
	}
	if (vk->textSubpixelFragShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->textSubpixelFragShaderModule, NULL);
	}
	if (vk->textColorFragShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->textColorFragShaderModule, NULL);
	}
	if (vk->computeShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(vk->device, vk->computeShaderModule, NULL);
	}

	// Cleanup compute pipeline resources
	if (vk->computePipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vk->device, vk->computePipeline, NULL);
	}
	if (vk->computePipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(vk->device, vk->computePipelineLayout, NULL);
	}
	if (vk->computeDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(vk->device, vk->computeDescriptorSetLayout, NULL);
	}
	if (vk->computeInputBuffers != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			vknvg__destroyBuffer(vk, &vk->computeInputBuffers[i]);
		}
		free(vk->computeInputBuffers);
	}
	if (vk->computeOutputBuffers != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			vknvg__destroyBuffer(vk, &vk->computeOutputBuffers[i]);
		}
		free(vk->computeOutputBuffers);
	}
	if (vk->computeDescriptorSets != NULL) {
		free(vk->computeDescriptorSets);
	}

	// Cleanup internal render pass if we own it
	if (vk->ownsRenderPass && vk->renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(vk->device, vk->renderPass, NULL);
	}

	// Cleanup descriptor sets (returned to pool automatically)
	if (vk->descriptorSets != NULL) {
		free(vk->descriptorSets);
	}

	// Cleanup command buffers (returned to pool automatically)
	if (vk->commandBuffers != NULL) {
		free(vk->commandBuffers);
	}

	// Cleanup synchronization primitives
	if (vk->frameFences != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			if (vk->frameFences[i] != VK_NULL_HANDLE) {
				vkDestroyFence(vk->device, vk->frameFences[i], NULL);
			}
		}
		free(vk->frameFences);
	}
	if (vk->imageAvailableSemaphores != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			if (vk->imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
				vkDestroySemaphore(vk->device, vk->imageAvailableSemaphores[i], NULL);
			}
		}
		free(vk->imageAvailableSemaphores);
	}
	if (vk->renderFinishedSemaphores != NULL) {
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			if (vk->renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
				vkDestroySemaphore(vk->device, vk->renderFinishedSemaphores[i], NULL);
			}
		}
		free(vk->renderFinishedSemaphores);
	}

	// Cleanup dynamic arrays
	if (vk->calls != NULL) free(vk->calls);
	if (vk->paths != NULL) free(vk->paths);
	if (vk->verts != NULL) free(vk->verts);
	if (vk->uniforms != NULL) free(vk->uniforms);

	// Cleanup profiling resources
	if (vk->profiling.timestampQueryPool != VK_NULL_HANDLE) {
		vkDestroyQueryPool(vk->device, vk->profiling.timestampQueryPool, NULL);
	}

	// Cleanup async compute resources
	if (vk->platformOpt.computeCommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(vk->device, vk->platformOpt.computeCommandPool, NULL);
	}

	free(vk);
}

#endif // NANOVG_VK_CLEANUP_H
