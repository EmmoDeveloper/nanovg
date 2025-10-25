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

#ifndef NANOVG_VK_RENDER_H
#define NANOVG_VK_RENDER_H

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include "nanovg_vk_internal.h"
#include "nanovg_vk_memory.h"
#include "nanovg_vk_image.h"
#include "nanovg_vk_pipeline.h"
#include "nanovg_vk_threading.h"
#include "nanovg_vk_platform.h"
#include "nanovg_vk_virtual_atlas.h"
#include "nanovg_vk_batch_text.h"
#include "nanovg_vk_text_cache.h"
#include "nanovg_vk_text_cache_integration.h"

__attribute__((unused))
static double vknvg__getTime() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static int vknvg__renderCreate(void* uptr)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VkResult result;

	// Check for stencil buffer availability
	vk->hasStencilBuffer = VK_FALSE;
	if (vk->depthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
	    vk->depthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
	    vk->depthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
	    vk->depthStencilFormat == VK_FORMAT_S8_UINT) {
		vk->hasStencilBuffer = VK_TRUE;
	}

	// Check for extensions
	vk->hasDynamicBlendState = VK_FALSE;
	vk->hasDynamicRendering = VK_FALSE;

	// Query device extensions
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(vk->physicalDevice, NULL, &extensionCount, NULL);
	if (extensionCount > 0) {
		VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extensionCount);
		vkEnumerateDeviceExtensionProperties(vk->physicalDevice, NULL, &extensionCount, extensions);

		for (uint32_t i = 0; i < extensionCount; i++) {
			// Check for VK_EXT_extended_dynamic_state3 (unless forced to use pipeline variants)
			if (!(vk->flags & NVG_BLEND_PIPELINE_VARIANTS) &&
			    strcmp(extensions[i].extensionName, "VK_EXT_extended_dynamic_state3") == 0) {
				vk->vkCmdSetColorBlendEquationEXT = (PFN_vkCmdSetColorBlendEquationEXT_)
					vkGetDeviceProcAddr(vk->device, "vkCmdSetColorBlendEquationEXT");
				vk->vkCmdSetColorBlendEnableEXT = (PFN_vkCmdSetColorBlendEnableEXT_)
					vkGetDeviceProcAddr(vk->device, "vkCmdSetColorBlendEnableEXT");

				if (vk->vkCmdSetColorBlendEquationEXT != NULL && vk->vkCmdSetColorBlendEnableEXT != NULL) {
					vk->hasDynamicBlendState = VK_TRUE;
				}
			}
			// Check for VK_KHR_dynamic_rendering (if requested)
			if ((vk->flags & NVG_DYNAMIC_RENDERING) &&
			    strcmp(extensions[i].extensionName, "VK_KHR_dynamic_rendering") == 0) {
				vk->vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR_)
					vkGetDeviceProcAddr(vk->device, "vkCmdBeginRenderingKHR");
				vk->vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR_)
					vkGetDeviceProcAddr(vk->device, "vkCmdEndRenderingKHR");

				if (vk->vkCmdBeginRenderingKHR != NULL && vk->vkCmdEndRenderingKHR != NULL) {
					vk->hasDynamicRendering = VK_TRUE;
				}
			}
		}
		free(extensions);
	}

	// Detect platform-specific optimizations based on GPU vendor
	vknvg__detectPlatformOptimizations(vk);

	// Setup async compute queue and command pool for high-end GPUs
	if (vk->platformOpt.useAsyncCompute && vk->platformOpt.computeQueueFamilyIndex != UINT32_MAX) {
		// Try to create command pool first to validate queue family exists in device
		VkCommandPoolCreateInfo poolInfo = {0};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = vk->platformOpt.computeQueueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool(vk->device, &poolInfo, NULL, &vk->platformOpt.computeCommandPool);
		if (result == VK_SUCCESS) {
			// Queue family exists, now get the queue
			uint32_t queueIndex = (vk->platformOpt.computeQueueFamilyIndex == vk->queueFamilyIndex) ? 1 : 0;
			vkGetDeviceQueue(vk->device, vk->platformOpt.computeQueueFamilyIndex, queueIndex, &vk->platformOpt.computeQueue);
		} else {
			// Queue family not available in device, disable async compute
			vk->platformOpt.useAsyncCompute = VK_FALSE;
			vk->platformOpt.computeQueue = VK_NULL_HANDLE;
			vk->platformOpt.computeCommandPool = VK_NULL_HANDLE;
		}
	}

	// Create internal render pass if needed
	if (vk->renderPass == VK_NULL_HANDLE && (vk->flags & NVG_INTERNAL_RENDER_PASS)) {
		result = vknvg__createRenderPass(vk);
		if (result != VK_SUCCESS) return 0;
		vk->ownsRenderPass = VK_TRUE;
		vk->subpass = 0;
	}

	// Create shader modules (using embedded SPIR-V)
	vk->vertShaderModule = vknvg__createShaderModule(vk->device, vknvg__vertShaderSpv, vknvg__vertShaderSize);
	if (vk->vertShaderModule == VK_NULL_HANDLE) return 0;

	vk->fragShaderModule = vknvg__createShaderModule(vk->device, vknvg__fragShaderSpv, vknvg__fragShaderSize);
	if (vk->fragShaderModule == VK_NULL_HANDLE) return 0;

	// Create instanced text vertex shader (for text rendering optimization)
	vk->textInstancedVertShaderModule = vknvg__createShaderModule(vk->device, vknvg__textInstancedVertShaderSpv, vknvg__textInstancedVertShaderSize);
	if (vk->textInstancedVertShaderModule == VK_NULL_HANDLE) return 0;

	// Create text shader modules if text rendering features enabled
	if (vk->flags & (NVG_SDF_TEXT | NVG_SUBPIXEL_TEXT | NVG_MSDF_TEXT | NVG_COLOR_TEXT)) {
		if (vk->flags & NVG_SDF_TEXT) {
			vk->textSDFFragShaderModule = vknvg__createShaderModule(vk->device, vknvg__textSDFFragShaderSpv, vknvg__textSDFFragShaderSize);
			if (vk->textSDFFragShaderModule == VK_NULL_HANDLE) return 0;
		}
		if (vk->flags & NVG_SUBPIXEL_TEXT) {
			vk->textSubpixelFragShaderModule = vknvg__createShaderModule(vk->device, vknvg__textSubpixelFragShaderSpv, vknvg__textSubpixelFragShaderSize);
			if (vk->textSubpixelFragShaderModule == VK_NULL_HANDLE) return 0;
		}
		if (vk->flags & NVG_MSDF_TEXT) {
			vk->textMSDFFragShaderModule = vknvg__createShaderModule(vk->device, vknvg__textMSDFFragShaderSpv, vknvg__textMSDFFragShaderSize);
			if (vk->textMSDFFragShaderModule == VK_NULL_HANDLE) return 0;
		}
		if (vk->flags & NVG_COLOR_TEXT) {
			vk->textColorFragShaderModule = vknvg__createShaderModule(vk->device, vknvg__textColorFragShaderSpv, vknvg__textColorFragShaderSize);
			if (vk->textColorFragShaderModule == VK_NULL_HANDLE) return 0;
		}
	}

	// Create descriptor set layout
	result = vknvg__createDescriptorSetLayout(vk);
	if (result != VK_SUCCESS) return 0;

	// Create pipeline layout
	result = vknvg__createPipelineLayout(vk);
	if (result != VK_SUCCESS) return 0;

	// Create pipelines
	// Stencil-only pipeline: color writes disabled, front INCR_WRAP, back DECR_WRAP
	result = vknvg__createGraphicsPipeline(vk, &vk->fillStencilPipeline, VK_TRUE,
	                                        VK_STENCIL_OP_INCREMENT_AND_WRAP,
	                                        VK_STENCIL_OP_DECREMENT_AND_WRAP,
	                                        VK_COMPARE_OP_ALWAYS, VK_FALSE, NULL);
	if (result != VK_SUCCESS) return 0;

	// AA fringe pipeline: color writes enabled, stencil test EQUAL 0
	result = vknvg__createGraphicsPipeline(vk, &vk->fillAAPipeline, VK_TRUE,
	                                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
	                                        VK_COMPARE_OP_EQUAL, VK_TRUE, NULL);
	if (result != VK_SUCCESS) return 0;

	// Fill pipeline: color writes enabled, stencil test NOT_EQUAL 0
	result = vknvg__createGraphicsPipeline(vk, &vk->fillPipeline, VK_TRUE,
	                                        VK_STENCIL_OP_ZERO, VK_STENCIL_OP_ZERO,
	                                        VK_COMPARE_OP_NOT_EQUAL, VK_TRUE, NULL);
	if (result != VK_SUCCESS) return 0;

	// Stroke pipeline: no stencil
	result = vknvg__createGraphicsPipeline(vk, &vk->strokePipeline, VK_FALSE,
	                                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
	                                        VK_COMPARE_OP_ALWAYS, VK_TRUE, NULL);
	if (result != VK_SUCCESS) return 0;

	// Triangles pipeline: no stencil
	result = vknvg__createGraphicsPipeline(vk, &vk->trianglesPipeline, VK_FALSE,
	                                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
	                                        VK_COMPARE_OP_ALWAYS, VK_TRUE, NULL);
	if (result != VK_SUCCESS) return 0;

	// Create text pipelines if text rendering features enabled
	if (vk->flags & NVG_SDF_TEXT) {
		result = vknvg__createTextPipeline(vk, &vk->textSDFPipeline, vk->textSDFFragShaderModule);
		if (result != VK_SUCCESS) return 0;
	}
	if (vk->flags & NVG_SUBPIXEL_TEXT) {
		result = vknvg__createTextPipeline(vk, &vk->textSubpixelPipeline, vk->textSubpixelFragShaderModule);
		if (result != VK_SUCCESS) return 0;
	}
	if (vk->flags & NVG_MSDF_TEXT) {
		result = vknvg__createTextPipeline(vk, &vk->textMSDFPipeline, vk->textMSDFFragShaderModule);
		if (result != VK_SUCCESS) return 0;
	}
	if (vk->flags & NVG_COLOR_TEXT) {
		result = vknvg__createTextPipeline(vk, &vk->textColorPipeline, vk->textColorFragShaderModule);
		if (result != VK_SUCCESS) return 0;
	}

	// Create compute shader resources if compute tessellation is enabled
	if (vk->flags & NVG_COMPUTE_TESSELLATION) {
		vk->computeShaderModule = vknvg__createShaderModule(vk->device, vknvg__computeShaderSpv, vknvg__computeShaderSize);
		if (vk->computeShaderModule == VK_NULL_HANDLE) return 0;

		result = vknvg__createComputeDescriptorSetLayout(vk);
		if (result != VK_SUCCESS) return 0;

		result = vknvg__createComputePipelineLayout(vk);
		if (result != VK_SUCCESS) return 0;

		result = vknvg__createComputePipeline(vk);
		if (result != VK_SUCCESS) return 0;

		// Allocate compute buffers (input and output per frame)
		vk->computeInputBuffers = (VKNVGbuffer*)malloc(sizeof(VKNVGbuffer) * vk->maxFrames);
		vk->computeOutputBuffers = (VKNVGbuffer*)malloc(sizeof(VKNVGbuffer) * vk->maxFrames);
		if (vk->computeInputBuffers == NULL || vk->computeOutputBuffers == NULL) return 0;

		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			memset(&vk->computeInputBuffers[i], 0, sizeof(VKNVGbuffer));
			memset(&vk->computeOutputBuffers[i], 0, sizeof(VKNVGbuffer));

			// Create storage buffers for compute shader (use platform-optimized size)
			VkDeviceSize computeBufferSize = vk->platformOpt.optimalVertexBufferSize;
			result = vknvg__createBuffer(vk, computeBufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk->computeInputBuffers[i]);
			if (result != VK_SUCCESS) return 0;

			result = vknvg__createBuffer(vk, computeBufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk->computeOutputBuffers[i]);
			if (result != VK_SUCCESS) return 0;
		}
	}

	// Allocate command buffers
	vk->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * vk->maxFrames);
	if (vk->commandBuffers == NULL) return 0;

	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = vk->maxFrames;

	result = vkAllocateCommandBuffers(vk->device, &allocInfo, vk->commandBuffers);
	if (result != VK_SUCCESS) return 0;

	// Create synchronization primitives if internal sync is requested
	if (vk->flags & NVG_INTERNAL_SYNC) {
		vk->frameFences = (VkFence*)malloc(sizeof(VkFence) * vk->maxFrames);
		vk->imageAvailableSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * vk->maxFrames);
		vk->renderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * vk->maxFrames);

		if (!vk->frameFences || !vk->imageAvailableSemaphores || !vk->renderFinishedSemaphores) {
			return 0;
		}

		VkFenceCreateInfo fenceInfo = {0};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			result = vkCreateFence(vk->device, &fenceInfo, NULL, &vk->frameFences[i]);
			if (result != VK_SUCCESS) return 0;

			result = vkCreateSemaphore(vk->device, &semaphoreInfo, NULL, &vk->imageAvailableSemaphores[i]);
			if (result != VK_SUCCESS) return 0;

			result = vkCreateSemaphore(vk->device, &semaphoreInfo, NULL, &vk->renderFinishedSemaphores[i]);
			if (result != VK_SUCCESS) return 0;
		}
	} else {
		vk->frameFences = NULL;
		vk->imageAvailableSemaphores = NULL;
		vk->renderFinishedSemaphores = NULL;
	}

	// Create per-frame vertex buffers
	vk->vertexBuffers = (VKNVGbuffer*)calloc(vk->maxFrames, sizeof(VKNVGbuffer));
	if (vk->vertexBuffers == NULL) return 0;

	// Use platform-optimized buffer size and memory properties
	VkMemoryPropertyFlags vertexMemProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	if (vk->platformOpt.useCoherentMemory) {
		vertexMemProps |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	if (vk->platformOpt.useDeviceLocalHostVisible) {
		vertexMemProps |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	for (uint32_t i = 0; i < vk->maxFrames; i++) {
		result = vknvg__createBuffer(vk, vk->platformOpt.optimalVertexBufferSize,
		                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                              vertexMemProps,
		                              &vk->vertexBuffers[i]);
		if (result != VK_SUCCESS) return 0;
	}

	// Create uniform buffer
	vk->uniformBuffer = (VKNVGbuffer*)malloc(sizeof(VKNVGbuffer));
	if (vk->uniformBuffer == NULL) return 0;

	// Use platform-optimized uniform buffer size
	VkDeviceSize uniformBufferSize = vk->platformOpt.optimalUniformBufferSize;
	if (uniformBufferSize < sizeof(VKNVGfragUniforms) * 256) {
		uniformBufferSize = sizeof(VKNVGfragUniforms) * 256;	// Minimum size for 256 draw calls
	}

	result = vknvg__createBuffer(vk, uniformBufferSize,
	                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                              vertexMemProps,
	                              vk->uniformBuffer);
	if (result != VK_SUCCESS) return 0;

	vk->fragSize = sizeof(VKNVGfragUniforms);

	// Initialize glyph instance buffer for instanced text rendering
	vk->useTextInstancing = VK_TRUE;  // Enable by default for performance
	vk->glyphInstanceBuffer = (VKNVGglyphInstanceBuffer*)malloc(sizeof(VKNVGglyphInstanceBuffer));
	if (vk->glyphInstanceBuffer != NULL) {
		result = vknvg__createGlyphInstanceBuffer(vk, vk->glyphInstanceBuffer);
		if (result != VK_SUCCESS) {
			// Non-fatal: fall back to non-instanced rendering
			free(vk->glyphInstanceBuffer);
			vk->glyphInstanceBuffer = NULL;
			vk->useTextInstancing = VK_FALSE;
		}
	} else {
		vk->useTextInstancing = VK_FALSE;
	}

	// Create instanced text pipelines if instancing is enabled
	// Note: Only create for actual text rendering modes (SDF/MSDF/Subpixel/Color)
	if (vk->useTextInstancing) {
		VkBool32 anyPipelineCreated = VK_FALSE;

		if (vk->flags & NVG_SDF_TEXT) {
			result = vknvg__createInstancedTextPipeline(vk, &vk->textInstancedSDFPipeline, vk->textSDFFragShaderModule);
			if (result == VK_SUCCESS) {
				anyPipelineCreated = VK_TRUE;
			}
		}
		if (vk->flags & NVG_SUBPIXEL_TEXT) {
			result = vknvg__createInstancedTextPipeline(vk, &vk->textInstancedSubpixelPipeline, vk->textSubpixelFragShaderModule);
			if (result == VK_SUCCESS) {
				anyPipelineCreated = VK_TRUE;
			}
		}
		if (vk->flags & NVG_MSDF_TEXT) {
			result = vknvg__createInstancedTextPipeline(vk, &vk->textInstancedMSDFPipeline, vk->textMSDFFragShaderModule);
			if (result == VK_SUCCESS) {
				anyPipelineCreated = VK_TRUE;
			}
		}
		if (vk->flags & NVG_COLOR_TEXT) {
			result = vknvg__createInstancedTextPipeline(vk, &vk->textInstancedColorPipeline, vk->textColorFragShaderModule);
			if (result == VK_SUCCESS) {
				anyPipelineCreated = VK_TRUE;
			}
		}

		// If no text rendering flags are set or all pipeline creations failed, disable instancing
		if (!anyPipelineCreated) {
			vk->useTextInstancing = VK_FALSE;
		}
	}

	// Initialize virtual atlas for CJK support (if flag set)
	vk->useVirtualAtlas = VK_FALSE;
	vk->virtualAtlas = NULL;
	vk->fontstashTextureId = 0;
	vk->debugUpdateTextureCount = 0;
	vk->pendingGlyphUploadCount = 0;
	if (vk->flags & NVG_VIRTUAL_ATLAS) {
		vk->virtualAtlas = vknvg__createVirtualAtlas(vk->device,
		                                               vk->physicalDevice,
		                                               NULL,	// Font context set later
		                                               NULL);	// Rasterize callback set later
		if (vk->virtualAtlas != NULL) {
			vk->useVirtualAtlas = VK_TRUE;
		}
	}

	// Initialize color emoji rendering (Phase 6)
	vk->useColorEmoji = VK_FALSE;
	vk->textEmojiState = NULL;
	vk->colorAtlas = NULL;
	vk->textDualModePipeline = VK_NULL_HANDLE;
	vk->textDualModeVertShaderModule = VK_NULL_HANDLE;
	vk->textDualModeFragShaderModule = VK_NULL_HANDLE;
	vk->dualModeDescriptorSetLayout = VK_NULL_HANDLE;
	vk->dualModePipelineLayout = VK_NULL_HANDLE;
	vk->dualModeDescriptorSets = NULL;

	if ((vk->flags & NVG_COLOR_EMOJI) && (vk->emojiFontPath != NULL || vk->emojiFontData != NULL)) {
		// Load dual-mode shader modules
		vk->textDualModeVertShaderModule = vknvg__createShaderModule(vk->device,
		                                                               vknvg__textDualModeVertShaderSpv,
		                                                               vknvg__textDualModeVertShaderSize);
		if (vk->textDualModeVertShaderModule == VK_NULL_HANDLE) goto skip_emoji;

		vk->textDualModeFragShaderModule = vknvg__createShaderModule(vk->device,
		                                                               vknvg__textDualModeFragShaderSpv,
		                                                               vknvg__textDualModeFragShaderSize);
		if (vk->textDualModeFragShaderModule == VK_NULL_HANDLE) goto skip_emoji;

		// Create color atlas (2048x2048 RGBA)
		vk->colorAtlas = vknvg__createColorAtlas(vk->device, vk->physicalDevice, vk->queue, vk->commandPool);
		if (vk->colorAtlas == NULL) goto skip_emoji;

		// Initialize text-emoji state with emoji font
		uint8_t* emojiData = NULL;
		uint32_t emojiDataSize = 0;
		VkBool32 needFreeEmojiData = VK_FALSE;

		if (vk->emojiFontPath != NULL) {
			// Load emoji font from file
			FILE* f = fopen(vk->emojiFontPath, "rb");
			if (f == NULL) goto skip_emoji;
			fseek(f, 0, SEEK_END);
			emojiDataSize = ftell(f);
			fseek(f, 0, SEEK_SET);
			emojiData = (uint8_t*)malloc(emojiDataSize);
			if (emojiData == NULL) {
				fclose(f);
				goto skip_emoji;
			}
			if (fread(emojiData, 1, emojiDataSize, f) != emojiDataSize) {
				free(emojiData);
				fclose(f);
				goto skip_emoji;
			}
			fclose(f);
			needFreeEmojiData = VK_TRUE;
		} else {
			emojiData = (uint8_t*)vk->emojiFontData;
			emojiDataSize = vk->emojiFontDataSize;
		}

		vk->textEmojiState = vknvg__createTextEmojiState(vk->colorAtlas, emojiData, emojiDataSize);
		if (needFreeEmojiData) {
			free(emojiData);
		}
		if (vk->textEmojiState == NULL) goto skip_emoji;

		// Create dual-mode descriptor set layout (3 bindings)
		result = vknvg__createDualModeDescriptorSetLayout(vk);
		if (result != VK_SUCCESS) goto skip_emoji;

		// Create dual-mode pipeline layout
		result = vknvg__createDualModePipelineLayout(vk, &vk->dualModePipelineLayout);
		if (result != VK_SUCCESS) goto skip_emoji;

		// Create dual-mode pipeline
		result = vknvg__createDualModeTextPipeline(vk, &vk->textDualModePipeline, vk->dualModePipelineLayout);
		if (result != VK_SUCCESS) goto skip_emoji;

		// Allocate dual-mode descriptor sets (per frame)
		vk->dualModeDescriptorSets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet) * vk->maxFrames);
		if (vk->dualModeDescriptorSets == NULL) goto skip_emoji;

		VkDescriptorSetLayout* dualModeLayouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * vk->maxFrames);
		if (dualModeLayouts == NULL) {
			free(vk->dualModeDescriptorSets);
			vk->dualModeDescriptorSets = NULL;
			goto skip_emoji;
		}
		for (uint32_t i = 0; i < vk->maxFrames; i++) {
			dualModeLayouts[i] = vk->dualModeDescriptorSetLayout;
		}

		VkDescriptorSetAllocateInfo dualModeAllocInfo = {0};
		dualModeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		dualModeAllocInfo.descriptorPool = vk->descriptorPool;
		dualModeAllocInfo.descriptorSetCount = vk->maxFrames;
		dualModeAllocInfo.pSetLayouts = dualModeLayouts;

		result = vkAllocateDescriptorSets(vk->device, &dualModeAllocInfo, vk->dualModeDescriptorSets);
		free(dualModeLayouts);
		if (result != VK_SUCCESS) {
			free(vk->dualModeDescriptorSets);
			vk->dualModeDescriptorSets = NULL;
			goto skip_emoji;
		}

		// Success - emoji rendering enabled
		vk->useColorEmoji = VK_TRUE;
	}

skip_emoji:
	// If emoji initialization failed, clean up partial resources
	if ((vk->flags & NVG_COLOR_EMOJI) && !vk->useColorEmoji) {
		if (vk->dualModeDescriptorSets != NULL) {
			free(vk->dualModeDescriptorSets);
			vk->dualModeDescriptorSets = NULL;
		}
		if (vk->dualModePipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(vk->device, vk->dualModePipelineLayout, NULL);
			vk->dualModePipelineLayout = VK_NULL_HANDLE;
		}
		if (vk->dualModeDescriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(vk->device, vk->dualModeDescriptorSetLayout, NULL);
			vk->dualModeDescriptorSetLayout = VK_NULL_HANDLE;
		}
		if (vk->textDualModePipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(vk->device, vk->textDualModePipeline, NULL);
			vk->textDualModePipeline = VK_NULL_HANDLE;
		}
		if (vk->textEmojiState != NULL) {
			vknvg__destroyTextEmojiState(vk->textEmojiState);
			vk->textEmojiState = NULL;
		}
		if (vk->colorAtlas != NULL) {
			vknvg__destroyColorAtlas(vk->colorAtlas);
			vk->colorAtlas = NULL;
		}
		if (vk->textDualModeFragShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(vk->device, vk->textDualModeFragShaderModule, NULL);
			vk->textDualModeFragShaderModule = VK_NULL_HANDLE;
		}
		if (vk->textDualModeVertShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(vk->device, vk->textDualModeVertShaderModule, NULL);
			vk->textDualModeVertShaderModule = VK_NULL_HANDLE;
		}
	}

	// Allocate per-frame descriptor sets
	vk->descriptorSets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet) * vk->maxFrames);
	if (vk->descriptorSets == NULL) return 0;

	VkDescriptorSetLayout* layouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * vk->maxFrames);
	if (layouts == NULL) return 0;
	for (uint32_t i = 0; i < vk->maxFrames; i++) {
		layouts[i] = vk->descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo allocInfo2 = {0};
	allocInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo2.descriptorPool = vk->descriptorPool;
	allocInfo2.descriptorSetCount = vk->maxFrames;
	allocInfo2.pSetLayouts = layouts;

	result = vkAllocateDescriptorSets(vk->device, &allocInfo2, vk->descriptorSets);
	free(layouts);
	if (result != VK_SUCCESS) return 0;

	// Initialize pipeline variant cache if not using dynamic blend state
	if (!vk->hasDynamicBlendState) {
		vk->pipelineVariantCacheSize = 64;
		vk->fillPipelineVariants = (VKNVGpipelineVariant**)calloc(vk->pipelineVariantCacheSize, sizeof(VKNVGpipelineVariant*));
		vk->strokePipelineVariants = (VKNVGpipelineVariant**)calloc(vk->pipelineVariantCacheSize, sizeof(VKNVGpipelineVariant*));
		vk->trianglesPipelineVariants = (VKNVGpipelineVariant**)calloc(vk->pipelineVariantCacheSize, sizeof(VKNVGpipelineVariant*));
		if (!vk->fillPipelineVariants || !vk->strokePipelineVariants || !vk->trianglesPipelineVariants) {
			return 0;
		}
	} else {
		vk->pipelineVariantCacheSize = 0;
		vk->fillPipelineVariants = NULL;
		vk->strokePipelineVariants = NULL;
		vk->trianglesPipelineVariants = NULL;
	}

	// Initialize profiling if enabled
	if (vk->flags & NVG_PROFILING) {
		memset(&vk->profiling, 0, sizeof(VKNVGprofiling));

		// Create timestamp query pool for GPU timing
		VkQueryPoolCreateInfo queryPoolInfo = {0};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolInfo.queryCount = 2 * vk->maxFrames;	// Start and end timestamp per frame

		result = vkCreateQueryPool(vk->device, &queryPoolInfo, NULL, &vk->profiling.timestampQueryPool);
		if (result == VK_SUCCESS) {
			// Get timestamp period for nanosecond to millisecond conversion
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(vk->physicalDevice, &deviceProps);
			vk->profiling.timestampPeriod = deviceProps.limits.timestampPeriod;
			vk->profiling.timestampsAvailable = deviceProps.limits.timestampComputeAndGraphics;
		} else {
			vk->profiling.timestampQueryPool = VK_NULL_HANDLE;
			vk->profiling.timestampsAvailable = VK_FALSE;
		}
	}

	// Initialize multi-threading if requested
	if (vk->flags & NVG_MULTI_THREADED) {
		if (!vknvg__initThreadPool(vk)) {
			return 0;
		}
	}

	// Initialize text run cache if requested
	vk->textCache = NULL;
	vk->useTextCache = VK_FALSE;
	vk->textCacheRenderPass = VK_NULL_HANDLE;

	if (vk->flags & NVG_TEXT_CACHE) {
		// Create render pass for text-to-texture
		#include "nanovg_vk_text_cache_integration.h"
		vk->textCacheRenderPass = vknvg__createTextCacheRenderPass(vk->device);
		if (vk->textCacheRenderPass == VK_NULL_HANDLE) {
			// Non-fatal: text caching disabled but rendering still works
			vk->useTextCache = VK_FALSE;
		} else {
			// Create text cache
			#include "nanovg_vk_text_cache.h"
			vk->textCache = vknvg__createTextRunCache(vk->device,
			                                           vk->physicalDevice,
			                                           vk->textCacheRenderPass);
			if (vk->textCache != NULL) {
				vk->useTextCache = VK_TRUE;
			} else {
				vkDestroyRenderPass(vk->device, vk->textCacheRenderPass, NULL);
				vk->textCacheRenderPass = VK_NULL_HANDLE;
			}
		}
	}

	return 1;
}

static int vknvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGtexture* tex;
	VkFormat format;
	int id = vknvg__allocTexture(vk);
	if (id == 0) return 0;

	tex = vknvg__findTexture(vk, id);
	if (tex == NULL) return 0;

	tex->width = w;
	tex->height = h;
	tex->type = type;
	tex->flags = imageFlags;

	// Detect fontstash texture creation for virtual atlas redirection
	// Fontstash creates an ALPHA texture that starts small and grows
	// This is the first ALPHA texture created, so we track it
	if (vk->useVirtualAtlas && type == 1 && vk->fontstashTextureId == 0) {
		vk->fontstashTextureId = id;
		// Mark texture for virtual atlas redirection
		tex->flags |= 0x20000;  // Custom flag for virtual atlas
	}

	// Determine format based on type
	// NanoVG texture types: RGBA=0, ALPHA=1
	format = (type == 1) ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

	// Create image
	VkResult result = vknvg__createImage(vk, w, h, format,
	                                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                                     tex);
	if (result != VK_SUCCESS) {
		tex->id = 0;
		return 0;
	}

	// Create image view
	result = vknvg__createImageView(vk, tex->image, format, &tex->imageView);
	if (result != VK_SUCCESS) {
		vkDestroyImage(vk->device, tex->image, NULL);
		vkFreeMemory(vk->device, tex->memory, NULL);
		tex->id = 0;
		return 0;
	}

	// Create sampler
	result = vknvg__createSampler(vk, imageFlags, &tex->sampler);
	if (result != VK_SUCCESS) {
		vkDestroyImageView(vk->device, tex->imageView, NULL);
		vkDestroyImage(vk->device, tex->image, NULL);
		vkFreeMemory(vk->device, tex->memory, NULL);
		tex->id = 0;
		return 0;
	}

	// Upload data if provided
	if (data != NULL) {
		int bytesPerPixel = (type == 1) ? 1 : 4;
		VkDeviceSize imageSize = w * h * bytesPerPixel;

		// Create staging buffer
		VKNVGbuffer stagingBuffer = {0};
		result = vknvg__createBuffer(vk, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                              &stagingBuffer);
		if (result != VK_SUCCESS) {
			vkDestroySampler(vk->device, tex->sampler, NULL);
			vkDestroyImageView(vk->device, tex->imageView, NULL);
			vkDestroyImage(vk->device, tex->image, NULL);
			vkFreeMemory(vk->device, tex->memory, NULL);
			tex->id = 0;
			return 0;
		}

		// Copy data to staging buffer (flip if requested)
		if (imageFlags & 0x01) { // NVG_IMAGE_FLIPY
			int rowSize = w * bytesPerPixel;
			for (int y = 0; y < h; y++) {
				const unsigned char* srcRow = data + y * rowSize;
				unsigned char* dstRow = (unsigned char*)stagingBuffer.mapped + (h - 1 - y) * rowSize;
				memcpy(dstRow, srcRow, rowSize);
			}
		} else {
			memcpy(stagingBuffer.mapped, data, imageSize);
		}

		// Transition image layout and copy
		vknvg__transitionImageLayout(vk, tex->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vknvg__copyBufferToImage(vk, stagingBuffer.buffer, tex->image, 0, 0, w, h);
		vknvg__transitionImageLayout(vk, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Cleanup staging buffer
		vknvg__destroyBuffer(vk, &stagingBuffer);
	} else {
		// Just transition to shader read layout
		vknvg__transitionImageLayout(vk, tex->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	return id;
}

static int vknvg__renderDeleteTexture(void* uptr, int image)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGtexture* tex = vknvg__findTexture(vk, image);
	if (tex == NULL) return 0;

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

	memset(tex, 0, sizeof(VKNVGtexture));
	return 1;
}

// Forward declaration of renderUpdateTexture for callback
static int vknvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);

// FONSglyph struct definition (from fontstash.h) for callback access
// This allows the callback to work without requiring FONTSTASH_IMPLEMENTATION
#ifndef FONTSTASH_IMPLEMENTATION
struct FONSglyph {
	unsigned int codepoint;
	int index;
	int next;
	short size, blur;
	short x0,y0,x1,y1;
	short xadv,xoff,yoff;
};
typedef struct FONSglyph FONSglyph;
#endif

// Handle glyph added callback from fontstash (Option 2 implementation)
// Fontstash uses 4096x4096 coordinate space but only rasterizes into small buffer
// This callback receives the glyph pixel data and uploads to virtual atlas or fontstash texture
void vknvg__onGlyphAdded(void* uptr, FONSglyph* glyph, int fontIndex, const unsigned char* data, int width, int height)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	if (!vk || !glyph || !data || width <= 0 || height <= 0) {
		return;
	}

	// Phase D: Check if this glyph should be rendered as a color emoji
	if (vk->useColorEmoji && vk->textEmojiState != NULL) {
		if (vknvg__shouldRenderAsEmoji(vk->textEmojiState, glyph->codepoint)) {
			// Try to render as color emoji
			VKNVGglyphRenderInfo renderInfo;
			float size = (float)(glyph->size) / 10.0f;  // Convert from 1/10th pixel

			if (vknvg__tryRenderAsEmoji(vk->textEmojiState, (uint32_t)fontIndex,
			                             glyph->codepoint, glyph->index, size, &renderInfo)) {
				// Successfully rendered as emoji - update glyph coordinates to point to color atlas
				// Note: The dual-mode shader will use binding 2 (color atlas) for these coords
				glyph->x0 = renderInfo.colorAtlasX;
				glyph->y0 = renderInfo.colorAtlasY;
				glyph->x1 = renderInfo.colorAtlasX + renderInfo.colorWidth;
				glyph->y1 = renderInfo.colorAtlasY + renderInfo.colorHeight;
				glyph->xoff = renderInfo.bearingX;
				glyph->yoff = renderInfo.bearingY;
				glyph->xadv = renderInfo.advance;
				return;  // Early return - glyph is in color atlas
			}
			// Fall through to SDF rendering if emoji rendering failed
		}
	}

	// If virtual atlas is enabled, use it for SDF rendering
	if (vk->useVirtualAtlas && vk->virtualAtlas) {
		// Check if data has non-zero pixels
		int nonZero = 0;
		for (int i = 0; i < width * height && i < 100; i++) {
			if (data[i] != 0) nonZero++;
		}
		printf("DEBUG: onGlyphAdded codepoint=0x%X size=%dx%d first100NonZero=%d\n", glyph->codepoint, width, height, nonZero);
		// Validate size (glyphs should be reasonable size)
		if (width > 256 || height > 256 || width * height > 65536) {
			// Glyph too large, skip
			return;
		}

		// Create glyph key
		VKNVGglyphKey key = {
			.fontID = (uint32_t)fontIndex,
			.codepoint = glyph->codepoint,
			.size = (uint32_t)(glyph->size << 16),  // Convert 1/10th pixel to fixed point 16.16
			.padding = 0
		};

		// Allocate and copy pixel data (virtual atlas takes ownership)
		size_t dataSize = (size_t)width * (size_t)height;
		uint8_t* pixelData = (uint8_t*)malloc(dataSize);
		if (pixelData) {
			memcpy(pixelData, data, dataSize);

			// Add to virtual atlas
			VKNVGglyphCacheEntry* entry = vknvg__addGlyphDirect(vk->virtualAtlas, key, pixelData,
			                      (uint16_t)width, (uint16_t)height,
			                      glyph->xoff, glyph->yoff,
			                      (uint16_t)glyph->xadv);

			// Update fontstash glyph coordinates to match virtual atlas location
			// This is critical: fontstash uses these coords to calculate UVs
			if (entry) {
				glyph->x0 = entry->atlasX;
				glyph->y0 = entry->atlasY;
				glyph->x1 = entry->atlasX + entry->width;
				glyph->y1 = entry->atlasY + entry->height;
			}
		}
	} else {
		// Fallback: upload to fontstash texture directly
		// With FONS_EXTERNAL_TEXTURE mode, fontstash doesn't call renderUpdate
		// We need to manually upload the glyph data to the font texture
		if (vk->fontstashTextureId > 0) {
			// Upload glyph pixel data at the coordinates fontstash assigned (x0, y0)
			// Fontstash has already calculated correct coordinates in 4096x4096 space
			vknvg__renderUpdateTexture(vk, vk->fontstashTextureId,
			                            glyph->x0, glyph->y0,
			                            width, height,
			                            data);
		}
	}
}

// Redirect fontstash glyph upload to virtual atlas
#ifdef FONTSTASH_IMPLEMENTATION
static void vknvg__redirectGlyphToVirtualAtlas(VKNVGcontext* vk, int x, int y, int w, int h, const unsigned char* data)
{
	FONScontext* fs = (FONScontext*)vk->fontStashContext;
	if (!fs || !vk->virtualAtlas) return;

	// Get current font from fontstash state
	int currentFontIdx = -1;
	if (fs->nstates > 0) {
		currentFontIdx = fs->states[fs->nstates - 1].font;
	}

	// Find the FONSglyph that matches this atlas position
	// Search in reverse order (newest glyphs first) as fontstash just added this glyph
	FONSglyph* targetGlyph = NULL;
	int fontIdx = -1;

	// First try current font (most likely)
	if (currentFontIdx >= 0 && currentFontIdx < fs->nfonts) {
		FONSfont* font = fs->fonts[currentFontIdx];
		if (font) {
			for (int j = font->nglyphs - 1; j >= 0; j--) {
				FONSglyph* g = &font->glyphs[j];
				if (g->x0 == x && g->y0 == y && g->x1 == (x + w) && g->y1 == (y + h)) {
					targetGlyph = g;
					fontIdx = currentFontIdx;
					break;
				}
			}
		}
	}

	// If not found, search other fonts (fallback fonts)
	if (!targetGlyph) {
		for (int i = fs->nfonts - 1; i >= 0 && !targetGlyph; i--) {
			if (i == currentFontIdx) continue;  // Already searched

			FONSfont* font = fs->fonts[i];
			if (!font) continue;

			for (int j = font->nglyphs - 1; j >= 0; j--) {
				FONSglyph* g = &font->glyphs[j];
				if (g->x0 == x && g->y0 == y && g->x1 == (x + w) && g->y1 == (y + h)) {
					targetGlyph = g;
					fontIdx = i;
					break;
				}
			}
		}
	}

	if (!targetGlyph) return;

	// Request this glyph in virtual atlas
	VKNVGglyphKey key = {
		.fontID = fontIdx + 1,
		.codepoint = targetGlyph->codepoint,
		.size = (targetGlyph->size << 16) / 10,  // Convert from fontstash's 10x format
		.padding = targetGlyph->blur
	};

	VKNVGglyphCacheEntry* entry = vknvg__requestGlyph(vk->virtualAtlas, key);
	if (!entry) return;

	// Upload pixel data to virtual atlas staging
	pthread_mutex_lock(&vk->virtualAtlas->uploadQueueMutex);
	if (vk->virtualAtlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
		size_t dataSize = w * h;
		uint8_t* pixelData = (uint8_t*)malloc(dataSize);
		if (pixelData) {
			memcpy(pixelData, data, dataSize);

			VKNVGglyphUploadRequest* upload = &vk->virtualAtlas->uploadQueue[vk->virtualAtlas->uploadQueueCount++];
			upload->atlasX = entry->atlasX;
			upload->atlasY = entry->atlasY;
			upload->width = w;
			upload->height = h;
			upload->pixelData = pixelData;
			upload->entry = entry;

			entry->state = VKNVG_GLYPH_READY;
		}
	}
	pthread_mutex_unlock(&vk->virtualAtlas->uploadQueueMutex);

	// Modify the FONSglyph to point to virtual atlas position
	// Virtual atlas uses normalized coordinates, fontstash uses pixel coordinates
	// We need to map from virtual atlas pixel position to fontstash-style coordinates
	targetGlyph->x0 = entry->atlasX;
	targetGlyph->y0 = entry->atlasY;
	targetGlyph->x1 = entry->atlasX + w;
	targetGlyph->y1 = entry->atlasY + h;
}
#endif

static int vknvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGtexture* tex = vknvg__findTexture(vk, image);
	if (tex == NULL) return 0;

	vk->debugUpdateTextureCount++;  // DEBUG: Track calls

#ifdef FONTSTASH_IMPLEMENTATION
	// Process pending glyph uploads to virtual atlas
	if ((tex->flags & 0x20000) && vk->useVirtualAtlas && vk->virtualAtlas && vk->pendingGlyphUploadCount > 0) {
		// Extract and upload each pending glyph from the dirty rect
		// NOTE: 'data' parameter contains only the dirty rectangle [x, y, w, h], not the entire atlas
		int kept = 0;  // Number of unprocessed glyphs to keep for next update
		for (int i = 0; i < vk->pendingGlyphUploadCount; i++) {
			int gx = vk->pendingGlyphUploads[i].origX;
			int gy = vk->pendingGlyphUploads[i].origY;
			int gw = vk->pendingGlyphUploads[i].w;
			int gh = vk->pendingGlyphUploads[i].h;

			// Check if glyph is fully within the dirty rectangle
			if (gx >= x && gy >= y && gx + gw <= x + w && gy + gh <= y + h) {
				// Extract glyph pixels from dirty rectangle data
				size_t dataSize = gw * gh;
				uint8_t* pixelData = (uint8_t*)malloc(dataSize);
				if (pixelData) {
					// Calculate position relative to dirty rectangle
					int relX = gx - x;
					int relY = gy - y;

					// Copy pixels from dirty rectangle buffer (indexed by dirty rect dimensions)
					for (int row = 0; row < gh; row++) {
						const uint8_t* src = data + (relY + row) * w + relX;
						uint8_t* dst = pixelData + row * gw;
						memcpy(dst, src, gw);
					}

					// Upload to virtual atlas
					pthread_mutex_lock(&vk->virtualAtlas->uploadQueueMutex);
					if (vk->virtualAtlas->uploadQueueCount < VKNVG_UPLOAD_QUEUE_SIZE) {
						VKNVGglyphUploadRequest* upload = &vk->virtualAtlas->uploadQueue[vk->virtualAtlas->uploadQueueCount++];
						upload->atlasX = vk->pendingGlyphUploads[i].atlasX;
						upload->atlasY = vk->pendingGlyphUploads[i].atlasY;
						upload->width = gw;
						upload->height = gh;
						upload->pixelData = pixelData;
						upload->entry = NULL;  // Entry already updated
					} else {
						free(pixelData);
					}
					pthread_mutex_unlock(&vk->virtualAtlas->uploadQueueMutex);
				}
				// Glyph processed successfully, don't keep it
			} else {
				// Glyph not in this dirty rect - keep it for next update
				// Compact unprocessed glyphs to the front of the array
				vk->pendingGlyphUploads[kept++] = vk->pendingGlyphUploads[i];
			}
		}

		// Update count to only include unprocessed glyphs
		vk->pendingGlyphUploadCount = kept;
		// Continue to also upload to fontstash texture (don't skip)
		// This ensures fontstash texture stays valid even if virtual atlas redirection fails
	}
#endif

	int bytesPerPixel = (tex->type == 1) ? 1 : 4;
	VkDeviceSize imageSize = w * h * bytesPerPixel;

	// Create staging buffer
	VKNVGbuffer stagingBuffer = {0};
	VkResult result = vknvg__createBuffer(vk, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                      &stagingBuffer);
	if (result != VK_SUCCESS) return 0;

	// Copy data to staging buffer (flip if texture has FLIPY flag)
	if (tex->flags & 0x01) { // NVG_IMAGE_FLIPY
		int rowSize = w * bytesPerPixel;
		for (int row = 0; row < h; row++) {
			const unsigned char* srcRow = data + row * rowSize;
			unsigned char* dstRow = (unsigned char*)stagingBuffer.mapped + (h - 1 - row) * rowSize;
			memcpy(dstRow, srcRow, rowSize);
		}
	} else {
		memcpy(stagingBuffer.mapped, data, imageSize);
	}

	// Transition, copy region, transition back
	vknvg__transitionImageLayout(vk, tex->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vknvg__copyBufferToImage(vk, stagingBuffer.buffer, tex->image, x, y, w, h);
	vknvg__transitionImageLayout(vk, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Cleanup staging buffer
	vknvg__destroyBuffer(vk, &stagingBuffer);

	return 1;
}

static int vknvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGtexture* tex = vknvg__findTexture(vk, image);
	if (tex == NULL) return 0;

	// Return virtual atlas size if this is the fontstash texture
	if ((tex->flags & 0x20000) && vk->useVirtualAtlas && vk->virtualAtlas) {
		*w = VKNVG_ATLAS_PHYSICAL_SIZE;
		*h = VKNVG_ATLAS_PHYSICAL_SIZE;
	} else {
		*w = tex->width;
		*h = tex->height;
	}
	return 1;
}

static void vknvg__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	(void)devicePixelRatio; // Not currently used
	vk->view[0] = width;
	vk->view[1] = height;
}

static void vknvg__renderCancel(void* uptr)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	vk->nverts = 0;
	vk->npaths = 0;
	vk->ncalls = 0;
	vk->nuniforms = 0;

	// Reset instance buffer if using instanced rendering
	if (vk->useTextInstancing && vk->glyphInstanceBuffer != NULL) {
		vknvg__resetGlyphInstances(vk->glyphInstanceBuffer);
	}
}

// Helper functions for rendering

static VKNVGcall* vknvg__allocCall(VKNVGcontext* vk)
{
	VKNVGcall* ret = NULL;
	if (vk->ncalls + 1 > vk->ccalls) {
		VKNVGcall* calls;
		int ccalls = vk->ccalls == 0 ? 32 : vk->ccalls * 2;
		calls = (VKNVGcall*)realloc(vk->calls, sizeof(VKNVGcall) * ccalls);
		if (calls == NULL) return NULL;
		vk->calls = calls;
		vk->ccalls = ccalls;
	}
	ret = &vk->calls[vk->ncalls++];
	memset(ret, 0, sizeof(VKNVGcall));
	return ret;
}

static int vknvg__allocPaths(VKNVGcontext* vk, int n)
{
	int ret = 0;
	if (vk->npaths + n > vk->cpaths) {
		VKNVGpath* paths;
		int cpaths = vk->cpaths == 0 ? 32 : vk->cpaths * 2;
		if (vk->npaths + n > cpaths) cpaths = vk->npaths + n;
		paths = (VKNVGpath*)realloc(vk->paths, sizeof(VKNVGpath) * cpaths);
		if (paths == NULL) return -1;
		vk->paths = paths;
		vk->cpaths = cpaths;
	}
	ret = vk->npaths;
	vk->npaths += n;
	return ret;
}

static int vknvg__allocVerts(VKNVGcontext* vk, int n)
{
	int ret = 0;
	if (vk->nverts + n > vk->cverts) {
		NVGvertex* verts;
		int cverts = vk->cverts == 0 ? 4096 : vk->cverts * 2;
		if (vk->nverts + n > cverts) cverts = vk->nverts + n;
		verts = (NVGvertex*)realloc(vk->verts, sizeof(NVGvertex) * cverts);
		if (verts == NULL) return -1;
		vk->verts = verts;
		vk->cverts = cverts;
	}
	ret = vk->nverts;
	vk->nverts += n;
	return ret;
}

static int vknvg__allocUniforms(VKNVGcontext* vk, int n)
{
	int ret = 0;
	if (vk->nuniforms + n > vk->cuniforms) {
		unsigned char* uniforms;
		int cuniforms = vk->cuniforms == 0 ? 128 : vk->cuniforms * 2;
		if (vk->nuniforms + n > cuniforms) cuniforms = vk->nuniforms + n;
		uniforms = (unsigned char*)realloc(vk->uniforms, vk->fragSize * cuniforms);
		if (uniforms == NULL) return -1;
		vk->uniforms = uniforms;
		vk->cuniforms = cuniforms;
	}
	ret = vk->nuniforms * vk->fragSize;
	vk->nuniforms += n;
	return ret;
}

static VKNVGfragUniforms* vknvg__fragUniformPtr(VKNVGcontext* vk, int offset)
{
	return (VKNVGfragUniforms*)&vk->uniforms[offset];
}

static VkBlendFactor vknvg__convertBlendFactor(int factor)
{
	if (factor & (1<<0)) return VK_BLEND_FACTOR_ZERO;					// NVG_ZERO
	if (factor & (1<<1)) return VK_BLEND_FACTOR_ONE;					// NVG_ONE
	if (factor & (1<<2)) return VK_BLEND_FACTOR_SRC_COLOR;				// NVG_SRC_COLOR
	if (factor & (1<<3)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;	// NVG_ONE_MINUS_SRC_COLOR
	if (factor & (1<<4)) return VK_BLEND_FACTOR_DST_COLOR;				// NVG_DST_COLOR
	if (factor & (1<<5)) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;	// NVG_ONE_MINUS_DST_COLOR
	if (factor & (1<<6)) return VK_BLEND_FACTOR_SRC_ALPHA;				// NVG_SRC_ALPHA
	if (factor & (1<<7)) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	// NVG_ONE_MINUS_SRC_ALPHA
	if (factor & (1<<8)) return VK_BLEND_FACTOR_DST_ALPHA;				// NVG_DST_ALPHA
	if (factor & (1<<9)) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;	// NVG_ONE_MINUS_DST_ALPHA
	if (factor & (1<<10)) return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;	// NVG_SRC_ALPHA_SATURATE
	return VK_BLEND_FACTOR_ONE;
}

static VKNVGblend vknvg__blendCompositeOperation(NVGcompositeOperationState op)
{
	VKNVGblend blend;
	blend.srcRGB = vknvg__convertBlendFactor(op.srcRGB);
	blend.dstRGB = vknvg__convertBlendFactor(op.dstRGB);
	blend.srcAlpha = vknvg__convertBlendFactor(op.srcAlpha);
	blend.dstAlpha = vknvg__convertBlendFactor(op.dstAlpha);
	return blend;
}

static void vknvg__convertPaint(VKNVGcontext* vk, VKNVGfragUniforms* frag, NVGpaint* paint,
                                 NVGscissor* scissor, float width, float fringe, float strokeThr)
{
	VKNVGtexture* tex = NULL;
	float invxform[6];

	memset(frag, 0, sizeof(*frag));

	frag->innerCol = paint->innerColor;
	frag->outerCol = paint->outerColor;

	if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
		memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
		frag->scissorExt[0] = 1.0f;
		frag->scissorExt[1] = 1.0f;
		frag->scissorScale[0] = 1.0f;
		frag->scissorScale[1] = 1.0f;
	} else {
		nvgTransformInverse(invxform, scissor->xform);
		// Convert to 3x4 matrix
		frag->scissorMat[0] = invxform[0];
		frag->scissorMat[1] = invxform[1];
		frag->scissorMat[2] = 0.0f;
		frag->scissorMat[3] = invxform[2];
		frag->scissorMat[4] = invxform[3];
		frag->scissorMat[5] = 0.0f;
		frag->scissorMat[6] = invxform[4];
		frag->scissorMat[7] = invxform[5];
		frag->scissorMat[8] = 1.0f;
		frag->scissorMat[9] = 0.0f;
		frag->scissorMat[10] = 0.0f;
		frag->scissorMat[11] = 0.0f;

		frag->scissorExt[0] = scissor->extent[0];
		frag->scissorExt[1] = scissor->extent[1];
		frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]) / fringe;
		frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]) / fringe;
	}

	memcpy(frag->extent, paint->extent, sizeof(frag->extent));
	frag->strokeMult = (width*0.5f + fringe*0.5f) / fringe;
	frag->strokeThr = strokeThr;

	if (paint->image != 0) {
		tex = vknvg__findTexture(vk, paint->image);
		if (tex != NULL) {
			nvgTransformInverse(invxform, paint->xform);
			frag->type = 1; // SHADER_FILLIMG
			// Set texType based on texture type and flags
			if (tex->type == 1) {
				// Alpha only (fonts) - check if MSDF or SDF
				if (vk->flags & NVG_MSDF_TEXT) {
					frag->texType = 2; // MSDF mode
				} else {
					frag->texType = 1; // SDF mode (default for fonts)
				}
			} else if (tex->flags & 0x02) { // NVG_IMAGE_PREMULTIPLIED
				frag->texType = 1; // Premultiplied alpha RGBA
			} else {
				frag->texType = 0; // Standard RGBA
			}
		} else {
			frag->type = 0; // SHADER_FILLGRAD as fallback
		}
	} else {
		frag->type = 0; // SHADER_FILLGRAD
	}

	// Convert paint transform to 3x4 matrix
	nvgTransformInverse(invxform, paint->xform);
	frag->paintMat[0] = invxform[0];
	frag->paintMat[1] = invxform[1];
	frag->paintMat[2] = 0.0f;
	frag->paintMat[3] = invxform[2];
	frag->paintMat[4] = invxform[3];
	frag->paintMat[5] = 0.0f;
	frag->paintMat[6] = invxform[4];
	frag->paintMat[7] = invxform[5];
	frag->paintMat[8] = 1.0f;
	frag->paintMat[9] = 0.0f;
	frag->paintMat[10] = 0.0f;
	frag->paintMat[11] = 0.0f;

	frag->radius = paint->radius;
	frag->feather = paint->feather;
}

static void vknvg__renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                               NVGscissor* scissor, float fringe, const float* bounds,
                               const NVGpath* paths, int npaths)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGcall* call = vknvg__allocCall(vk);
	NVGvertex* quad;
	VKNVGfragUniforms* frag;
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = 0; // FILL type
	call->triangleCount = 4;
	call->pathOffset = vknvg__allocPaths(vk, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;
	call->blend = vknvg__blendCompositeOperation(compositeOperation);

	// Convex fill optimization: skip stencil if single convex path
	if (npaths == 1 && paths[0].convex) {
		call->type = 3; // CONVEXFILL type
		call->triangleCount = 0; // No bounding box quad needed
	}

	// Calculate max vertices needed
	maxverts = 0;
	for (i = 0; i < npaths; i++) {
		maxverts += paths[i].nfill;
		maxverts += paths[i].nstroke;
	}
	maxverts += call->triangleCount;

	offset = vknvg__allocVerts(vk, maxverts);
	if (offset == -1) goto error;

	// Copy path vertex data
	for (i = 0; i < npaths; i++) {
		VKNVGpath* copy = &vk->paths[call->pathOffset + i];
		const NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(VKNVGpath));
		if (path->nfill > 0) {
			copy->fillOffset = offset;
			copy->fillCount = path->nfill;
			memcpy(&vk->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
			offset += path->nfill;
		}
		if (path->nstroke > 0) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&vk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Quad for bounding box fill (only for non-convex)
	if (call->type != 3) {
		call->triangleOffset = offset;
		quad = &vk->verts[call->triangleOffset];
		quad[0].x = bounds[2]; quad[0].y = bounds[3]; quad[0].u = 0.5f; quad[0].v = 1.0f;
		quad[1].x = bounds[2]; quad[1].y = bounds[1]; quad[1].u = 0.5f; quad[1].v = 1.0f;
		quad[2].x = bounds[0]; quad[2].y = bounds[3]; quad[2].u = 0.5f; quad[2].v = 1.0f;
		quad[3].x = bounds[0]; quad[3].y = bounds[1]; quad[3].u = 0.5f; quad[3].v = 1.0f;
	}

	// Allocate uniforms
	if (call->type == 3) {
		// Convex fill: only one uniform needed
		call->uniformOffset = vknvg__allocUniforms(vk, 1);
		if (call->uniformOffset == -1) goto error;
		vknvg__convertPaint(vk, vknvg__fragUniformPtr(vk, call->uniformOffset),
		                     paint, scissor, fringe, fringe, -1.0f);
	} else {
		// Non-convex fill: two uniforms (stencil pass and fill pass)
		call->uniformOffset = vknvg__allocUniforms(vk, 2);
		if (call->uniformOffset == -1) goto error;

		// Simple shader for stencil
		frag = vknvg__fragUniformPtr(vk, call->uniformOffset);
		memset(frag, 0, sizeof(*frag));
		frag->strokeThr = -1.0f;
		frag->type = 2; // SIMPLE type

		// Fill shader
		vknvg__convertPaint(vk, vknvg__fragUniformPtr(vk, call->uniformOffset + vk->fragSize),
		                     paint, scissor, fringe, fringe, -1.0f);
	}

	return;

error:
	if (vk->ncalls > 0) vk->ncalls--;
}

static void vknvg__renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                                NVGscissor* scissor, float fringe, float strokeWidth,
                                const NVGpath* paths, int npaths)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGcall* call = vknvg__allocCall(vk);
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = 1; // STROKE type
	call->pathOffset = vknvg__allocPaths(vk, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;
	call->blend = vknvg__blendCompositeOperation(compositeOperation);

	// Calculate max vertices
	maxverts = 0;
	for (i = 0; i < npaths; i++) {
		maxverts += paths[i].nstroke;
	}

	offset = vknvg__allocVerts(vk, maxverts);
	if (offset == -1) goto error;

	// Copy stroke vertex data
	for (i = 0; i < npaths; i++) {
		VKNVGpath* copy = &vk->paths[call->pathOffset + i];
		const NVGpath* path = &paths[i];
		memset(copy, 0, sizeof(VKNVGpath));
		if (path->nstroke > 0) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			memcpy(&vk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Allocate uniforms
	if (vk->flags & NVG_STENCIL_STROKES) {
		call->uniformOffset = vknvg__allocUniforms(vk, 2);
		if (call->uniformOffset == -1) goto error;
		vknvg__convertPaint(vk, vknvg__fragUniformPtr(vk, call->uniformOffset),
		                     paint, scissor, strokeWidth, fringe, -1.0f);
		vknvg__convertPaint(vk, vknvg__fragUniformPtr(vk, call->uniformOffset + vk->fragSize),
		                     paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);
	} else {
		call->uniformOffset = vknvg__allocUniforms(vk, 1);
		if (call->uniformOffset == -1) goto error;
		vknvg__convertPaint(vk, vknvg__fragUniformPtr(vk, call->uniformOffset),
		                     paint, scissor, strokeWidth, fringe, -1.0f);
	}

	return;

error:
	if (vk->ncalls > 0) vk->ncalls--;
}

static void vknvg__renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                                   NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGcall* call = vknvg__allocCall(vk);
	VKNVGfragUniforms* frag;

	if (call == NULL) return;

	call->type = 2; // TRIANGLES type
	call->image = paint->image;
	call->blend = vknvg__blendCompositeOperation(compositeOperation);
	call->useInstancing = VK_FALSE;
	call->useColorEmoji = VK_FALSE;	// Default to SDF rendering

	// Check if we should use instanced rendering for text
	// Text rendering uses font atlas (image >= 0) and triangles
	VkBool32 isTextRendering = (paint->image > 0 && nverts % 6 == 0);

	// Phase E: Enable dual-mode emoji rendering for text
	// The dual-mode shader handles both SDF and color emoji automatically
	if (isTextRendering && vk->useColorEmoji && vk->textDualModePipeline != VK_NULL_HANDLE) {
		call->useColorEmoji = VK_TRUE;
	}

	if (vk->useTextInstancing && isTextRendering && vk->glyphInstanceBuffer != NULL) {
		// Use instanced rendering: convert vertex quads to instances
		int nglyphs = nverts / 6;

		if (vk->glyphInstanceBuffer->count + nglyphs <= vk->glyphInstanceBuffer->capacity) {
			call->useInstancing = VK_TRUE;
			call->instanceOffset = vk->glyphInstanceBuffer->count;
			call->instanceCount = nglyphs;

			// Convert vertices to instances (6 vertices per glyph -> 1 instance per glyph)
			for (int i = 0; i < nglyphs; i++) {
				const NVGvertex* quad = &verts[i * 6];

				// Extract min/max positions and UVs from the 6 vertices
				float minX = quad[0].x, maxX = quad[0].x;
				float minY = quad[0].y, maxY = quad[0].y;
				float minU = quad[0].u, maxU = quad[0].u;
				float minV = quad[0].v, maxV = quad[0].v;

				for (int j = 1; j < 6; j++) {
					if (quad[j].x < minX) minX = quad[j].x;
					if (quad[j].x > maxX) maxX = quad[j].x;
					if (quad[j].y < minY) minY = quad[j].y;
					if (quad[j].y > maxY) maxY = quad[j].y;
					if (quad[j].u < minU) minU = quad[j].u;
					if (quad[j].u > maxU) maxU = quad[j].u;
					if (quad[j].v < minV) minV = quad[j].v;
					if (quad[j].v > maxV) maxV = quad[j].v;
				}

				vknvg__addGlyphInstance(vk->glyphInstanceBuffer,
				                        minX, minY,
				                        maxX - minX, maxY - minY,
				                        minU, minV,
				                        maxU - minU, maxV - minV);
			}
		} else {
			// Instance buffer full, fall back to regular rendering
			call->useInstancing = VK_FALSE;
		}
	}

	if (!call->useInstancing) {
		// Regular vertex-based rendering
		call->triangleOffset = vknvg__allocVerts(vk, nverts);
		if (call->triangleOffset == -1) goto error;
		call->triangleCount = nverts;
		memcpy(&vk->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);
	}

	// Allocate uniform
	call->uniformOffset = vknvg__allocUniforms(vk, 1);
	if (call->uniformOffset == -1) goto error;
	frag = vknvg__fragUniformPtr(vk, call->uniformOffset);
	vknvg__convertPaint(vk, frag, paint, scissor, 1.0f, fringe, -1.0f);
	frag->type = 1; // IMG type for triangles

	return;

error:
	if (vk->ncalls > 0) vk->ncalls--;
}

static VkPipeline vknvg__getOrCreatePipelineVariant(VKNVGcontext* vk, VKNVGpipelineVariant** cacheTable,
                                                    VKNVGblend blend, VkBool32 enableStencil,
                                                    VkStencilOp frontStencilOp, VkStencilOp backStencilOp,
                                                    VkCompareOp stencilCompareOp, VkBool32 colorWriteEnable)
{
	uint32_t hash = vknvg__hashBlendState(blend, vk->pipelineVariantCacheSize);
	VKNVGpipelineVariant* variant = cacheTable[hash];

	// Search linked list for matching blend state
	while (variant != NULL) {
		if (vknvg__compareBlendState(variant->blend, blend)) {
			return variant->pipeline;
		}
		variant = variant->next;
	}

	// Not found - create new pipeline
	VkPipeline newPipeline = VK_NULL_HANDLE;
	VkResult result = vknvg__createGraphicsPipeline(vk, &newPipeline, enableStencil,
	                                                 frontStencilOp, backStencilOp,
	                                                 stencilCompareOp, colorWriteEnable, &blend);
	if (result != VK_SUCCESS || newPipeline == VK_NULL_HANDLE) {
		return VK_NULL_HANDLE;
	}

	// Add to cache
	VKNVGpipelineVariant* newVariant = (VKNVGpipelineVariant*)malloc(sizeof(VKNVGpipelineVariant));
	if (newVariant == NULL) {
		vkDestroyPipeline(vk->device, newPipeline, NULL);
		return VK_NULL_HANDLE;
	}

	newVariant->blend = blend;
	newVariant->pipeline = newPipeline;
	newVariant->next = cacheTable[hash];
	cacheTable[hash] = newVariant;

	return newPipeline;
}

static void vknvg__renderFlush(void* uptr)
{
	VKNVGcontext* vk = (VKNVGcontext*)uptr;
	VKNVGbuffer* vertexBuffer;
	VkCommandBuffer cmd;
	VkDescriptorSet descriptorSet;
	int i;

	if (vk->ncalls == 0) return;

	vertexBuffer = &vk->vertexBuffers[vk->currentFrame];
	cmd = vk->commandBuffers[vk->currentFrame];
	descriptorSet = vk->descriptorSets[vk->currentFrame];

	// Begin command buffer recording (must be before any vkCmd* calls)
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	// Process virtual atlas uploads (records commands into cmd)
	if (vk->useVirtualAtlas && vk->virtualAtlas) {
		vknvg__processUploads(vk->virtualAtlas, cmd);
		vknvg__atlasNextFrame(vk->virtualAtlas);
	}

	// Upload vertex data
	if (vk->nverts > 0) {
		VkDeviceSize vertexSize = vk->nverts * sizeof(NVGvertex);
		if (vertexSize > vertexBuffer->size) {
			vknvg__destroyBuffer(vk, vertexBuffer);
			VkDeviceSize newSize = vertexSize * 2;
			vknvg__createBuffer(vk, newSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			                     vertexBuffer);
		}
		memcpy(vertexBuffer->mapped, vk->verts, vertexSize);
	}

	// Upload instance data if using instanced rendering
	if (vk->useTextInstancing && vk->glyphInstanceBuffer != NULL && vk->glyphInstanceBuffer->count > 0) {
		vknvg__uploadGlyphInstances(vk->glyphInstanceBuffer);
	}

	// Upload uniform data
	if (vk->nuniforms > 0) {
		VkDeviceSize uniformSize = vk->nuniforms * vk->fragSize;
		if (uniformSize <= vk->uniformBuffer->size) {
			memcpy(vk->uniformBuffer->mapped, vk->uniforms, uniformSize);
		}
	}

	// Update descriptor set with uniform buffer
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = vk->uniformBuffer->buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = vk->fragSize;

	VkWriteDescriptorSet descriptorWrite = {0};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);

	// Begin rendering (dynamic rendering mode or application-managed render pass)
	// Note: command buffer was already begun earlier for virtual atlas uploads
	if (vk->hasDynamicRendering) {
		// Use VK_KHR_dynamic_rendering
		VkRenderingAttachmentInfoKHR colorAttachment = {0};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = vk->colorImageView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingAttachmentInfoKHR depthStencilAttachment = {0};
		depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthStencilAttachment.imageView = vk->depthStencilImageView;
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfoKHR renderingInfo = {0};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea.offset.x = 0;
		renderingInfo.renderArea.offset.y = 0;
		renderingInfo.renderArea.extent.width = (uint32_t)vk->view[0];
		renderingInfo.renderArea.extent.height = (uint32_t)vk->view[1];
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = (vk->depthStencilImageView != VK_NULL_HANDLE) ? &depthStencilAttachment : NULL;
		renderingInfo.pStencilAttachment = (vk->depthStencilImageView != VK_NULL_HANDLE) ? &depthStencilAttachment : NULL;

		vk->vkCmdBeginRenderingKHR(cmd, &renderingInfo);
	}
	// Note: If not using dynamic rendering, application must have already begun render pass

	// Bind vertex buffer
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer->buffer, offsets);

	// Set viewport and push constants
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = vk->view[0];
	viewport.height = vk->view[1];
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = (uint32_t)vk->view[0];
	scissor.extent.height = (uint32_t)vk->view[1];
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdPushConstants(cmd, vk->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, vk->view);

	// Batch consecutive text rendering calls to reduce draw call overhead
	vk->ncalls = vknvg__batchTextCalls(vk);

	// Execute draw calls
	for (i = 0; i < vk->ncalls; i++) {
		VKNVGcall* call = &vk->calls[i];
		VKNVGpath* paths = &vk->paths[call->pathOffset];
		int j;

		// Set blend state for this call (if dynamic blending available)
		if (vk->hasDynamicBlendState) {
			VkBool32 blendEnable = VK_TRUE;
			vk->vkCmdSetColorBlendEnableEXT(cmd, 0, 1, &blendEnable);

			// VkColorBlendEquationEXT structure
			struct {
				VkBlendFactor srcColorBlendFactor;
				VkBlendFactor dstColorBlendFactor;
				VkBlendOp colorBlendOp;
				VkBlendFactor srcAlphaBlendFactor;
				VkBlendFactor dstAlphaBlendFactor;
				VkBlendOp alphaBlendOp;
			} blendEquation;

			blendEquation.srcColorBlendFactor = call->blend.srcRGB;
			blendEquation.dstColorBlendFactor = call->blend.dstRGB;
			blendEquation.colorBlendOp = VK_BLEND_OP_ADD;
			blendEquation.srcAlphaBlendFactor = call->blend.srcAlpha;
			blendEquation.dstAlphaBlendFactor = call->blend.dstAlpha;
			blendEquation.alphaBlendOp = VK_BLEND_OP_ADD;

			vk->vkCmdSetColorBlendEquationEXT(cmd, 0, 1, &blendEquation);
		}

		if (call->type == 0) { // FILL
			if (vk->hasStencilBuffer) {
				// Stencil-based fill (3-pass algorithm)
				// Pass 1: Stencil write (color writes disabled)
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->fillStencilPipeline);
				bufferInfo.offset = call->uniformOffset;
				vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);

				// Draw triangle fans into stencil buffer
				for (j = 0; j < call->pathCount; j++) {
					if (paths[j].fillCount > 0) {
						vkCmdDraw(cmd, paths[j].fillCount, 1, paths[j].fillOffset, 0);
					}
				}

				// Pass 2: AA fringe (draw where stencil == 0)
				if (vk->flags & NVG_ANTIALIAS) {
					VkPipeline aaPipeline = vk->fillAAPipeline;
					if (!vk->hasDynamicBlendState) {
						aaPipeline = vknvg__getOrCreatePipelineVariant(vk, vk->fillPipelineVariants,
						                                                call->blend, VK_TRUE,
						                                                VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
						                                                VK_COMPARE_OP_EQUAL, VK_TRUE);
					}
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aaPipeline);
					bufferInfo.offset = call->uniformOffset + vk->fragSize;
					vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
					                         0, 1, &descriptorSet, 0, NULL);

					for (j = 0; j < call->pathCount; j++) {
						if (paths[j].strokeCount > 0) {
							vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
						}
					}
				}

				// Pass 3: Fill quad (draw where stencil != 0, and clear stencil)
				VkPipeline fillPipeline = vk->fillPipeline;
				if (!vk->hasDynamicBlendState) {
					fillPipeline = vknvg__getOrCreatePipelineVariant(vk, vk->fillPipelineVariants,
					                                                  call->blend, VK_TRUE,
					                                                  VK_STENCIL_OP_ZERO, VK_STENCIL_OP_ZERO,
					                                                  VK_COMPARE_OP_NOT_EQUAL, VK_TRUE);
				}
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fillPipeline);
				// Descriptor set already bound to correct uniform
				if (call->triangleCount > 0) {
					vkCmdDraw(cmd, call->triangleCount, 1, call->triangleOffset, 0);
				}
			} else {
				// Fallback: Simple non-stencil fill (may not handle self-intersecting paths correctly)
				VkPipeline simpleFillPipeline = vk->trianglesPipeline;
				if (!vk->hasDynamicBlendState) {
					simpleFillPipeline = vknvg__getOrCreatePipelineVariant(vk, vk->trianglesPipelineVariants,
					                                                         call->blend, VK_FALSE,
					                                                         VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
					                                                         VK_COMPARE_OP_ALWAYS, VK_TRUE);
				}
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, simpleFillPipeline);
				bufferInfo.offset = call->uniformOffset;
				vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);

				// Draw fill geometry directly
				for (j = 0; j < call->pathCount; j++) {
					if (paths[j].fillCount > 0) {
						vkCmdDraw(cmd, paths[j].fillCount, 1, paths[j].fillOffset, 0);
					}
					// Draw fringe for antialiasing
					if ((vk->flags & NVG_ANTIALIAS) && paths[j].strokeCount > 0) {
						vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
					}
				}
			}

		} else if (call->type == 3) { // CONVEXFILL (no stencil)
			// Simple fill without stencil passes
			VkPipeline convexPipeline = vk->trianglesPipeline;
			if (!vk->hasDynamicBlendState) {
				convexPipeline = vknvg__getOrCreatePipelineVariant(vk, vk->trianglesPipelineVariants,
				                                                     call->blend, VK_FALSE,
				                                                     VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
				                                                     VK_COMPARE_OP_ALWAYS, VK_TRUE);
			}
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, convexPipeline);
			bufferInfo.offset = call->uniformOffset;
			vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
			                         0, 1, &descriptorSet, 0, NULL);

			// Draw fill geometry and fringe directly
			for (j = 0; j < call->pathCount; j++) {
				if (paths[j].fillCount > 0) {
					vkCmdDraw(cmd, paths[j].fillCount, 1, paths[j].fillOffset, 0);
				}
				if (paths[j].strokeCount > 0) {
					vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
				}
			}

		} else if (call->type == 1) { // STROKE
			// Bind stroke pipeline
			VkPipeline strokePipeline = vk->strokePipeline;
			if (!vk->hasDynamicBlendState) {
				strokePipeline = vknvg__getOrCreatePipelineVariant(vk, vk->strokePipelineVariants,
				                                                     call->blend, VK_FALSE,
				                                                     VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
				                                                     VK_COMPARE_OP_ALWAYS, VK_TRUE);
			}
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, strokePipeline);

			if (vk->flags & NVG_STENCIL_STROKES) {
				// Stencil stroke rendering (multi-pass)
				// Pass 1: Fill stroke base
				bufferInfo.offset = call->uniformOffset + vk->fragSize;
				vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);
				for (j = 0; j < call->pathCount; j++) {
					if (paths[j].strokeCount > 0) {
						vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
					}
				}

				// Pass 2: AA pass
				bufferInfo.offset = call->uniformOffset;
				vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);
				for (j = 0; j < call->pathCount; j++) {
					if (paths[j].strokeCount > 0) {
						vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
					}
				}
			} else {
				// Simple stroke rendering (single pass)
				bufferInfo.offset = call->uniformOffset;
				vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);
				for (j = 0; j < call->pathCount; j++) {
					if (paths[j].strokeCount > 0) {
						vkCmdDraw(cmd, paths[j].strokeCount, 1, paths[j].strokeOffset, 0);
					}
				}
			}

		} else if (call->type == 2) { // TRIANGLES
			// Determine pipeline based on texture type, text flags, and instancing
			VkPipeline trianglesPipeline = vk->trianglesPipeline;
			VkBool32 isTextRendering = VK_FALSE;
			VkBool32 useInstancedPipeline = call->useInstancing;
			VkBool32 useEmojiPipeline = VK_FALSE;

			// Check if we're using dual-mode emoji rendering (Phase 6)
			if (call->useColorEmoji && vk->useColorEmoji && vk->textDualModePipeline != VK_NULL_HANDLE) {
				trianglesPipeline = vk->textDualModePipeline;
				isTextRendering = VK_TRUE;
				useEmojiPipeline = VK_TRUE;
			}
			// Check if we're rendering text
			else if (call->image > 0) {
				VKNVGtexture* tex = vknvg__findTexture(vk, call->image);
				if (tex != NULL) {
					if (tex->type == 3 && (vk->flags & NVG_COLOR_TEXT)) {
						// RGBA texture (color emoji/fonts)
						trianglesPipeline = useInstancedPipeline ? vk->textInstancedColorPipeline : vk->textColorPipeline;
						isTextRendering = VK_TRUE;
					} else if (tex->type == 2) { // Alpha-only (fonts)
						// Use specialized text pipelines if available
						if ((vk->flags & NVG_MSDF_TEXT) && vk->textMSDFPipeline != VK_NULL_HANDLE) {
							trianglesPipeline = useInstancedPipeline ? vk->textInstancedMSDFPipeline : vk->textMSDFPipeline;
							isTextRendering = VK_TRUE;
						} else if ((vk->flags & NVG_SDF_TEXT) && vk->textSDFPipeline != VK_NULL_HANDLE) {
							trianglesPipeline = useInstancedPipeline ? vk->textInstancedSDFPipeline : vk->textSDFPipeline;
							isTextRendering = VK_TRUE;
						} else if ((vk->flags & NVG_SUBPIXEL_TEXT) && vk->textSubpixelPipeline != VK_NULL_HANDLE) {
							trianglesPipeline = useInstancedPipeline ? vk->textInstancedSubpixelPipeline : vk->textSubpixelPipeline;
							isTextRendering = VK_TRUE;
						} else if (useInstancedPipeline) {
							trianglesPipeline = vk->textInstancedPipeline;
							isTextRendering = VK_TRUE;
						}
					}
				}
			}

			// Use variant if not using text pipeline and no dynamic blend state
			if (!isTextRendering && !vk->hasDynamicBlendState) {
				trianglesPipeline = vknvg__getOrCreatePipelineVariant(vk, vk->trianglesPipelineVariants,
				                                                        call->blend, VK_FALSE,
				                                                        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
				                                                        VK_COMPARE_OP_ALWAYS, VK_TRUE);
			}
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglesPipeline);

			// Bind instance buffer if using instanced rendering
			if (useInstancedPipeline && vk->glyphInstanceBuffer != NULL) {
				VkBuffer instanceBuffer = vk->glyphInstanceBuffer->buffer->buffer;
				VkDeviceSize instanceOffset = 0;
				vkCmdBindVertexBuffers(cmd, 1, 1, &instanceBuffer, &instanceOffset);
			}

			// Update descriptor set and bind textures
			bufferInfo.offset = call->uniformOffset;
			vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);

			if (useEmojiPipeline) {
				// Dual-mode emoji rendering: bind both SDF and color atlases
				VkDescriptorSet emojiDescriptorSet = vk->dualModeDescriptorSets[vk->currentFrame];

				// Binding 0: Uniform buffer (already updated above)
				// Binding 1: SDF atlas (fontstash texture)
				// Binding 2: Color atlas (emoji texture)

				VkDescriptorImageInfo imageInfos[2] = {0};
				VkWriteDescriptorSet imageWrites[2] = {0};

				// SDF atlas at binding 1
				if (call->image > 0 && vk->useVirtualAtlas && vk->virtualAtlas) {
					imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos[0].imageView = vk->virtualAtlas->atlasImageView;
					imageInfos[0].sampler = vk->virtualAtlas->atlasSampler;

					imageWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					imageWrites[0].dstSet = emojiDescriptorSet;
					imageWrites[0].dstBinding = 1;
					imageWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					imageWrites[0].descriptorCount = 1;
					imageWrites[0].pImageInfo = &imageInfos[0];
				}

				// Color atlas at binding 2
				if (vk->colorAtlas != NULL && vk->colorAtlas->atlasManager != NULL &&
				    vk->colorAtlas->atlasManager->atlasCount > 0) {
					imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos[1].imageView = vk->colorAtlas->atlasManager->atlases[0].imageView;
					imageInfos[1].sampler = vk->colorAtlas->atlasSampler;

					imageWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					imageWrites[1].dstSet = emojiDescriptorSet;
					imageWrites[1].dstBinding = 2;
					imageWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					imageWrites[1].descriptorCount = 1;
					imageWrites[1].pImageInfo = &imageInfos[1];

					vkUpdateDescriptorSets(vk->device, 2, imageWrites, 0, NULL);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->dualModePipelineLayout,
				                         0, 1, &emojiDescriptorSet, 0, NULL);
			} else {
				// Regular rendering: bind single texture
				if (call->image > 0) {
					VKNVGtexture* tex = vknvg__findTexture(vk, call->image);
					if (tex != NULL) {
						VkDescriptorImageInfo imageInfo = {0};
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

						// Redirect to virtual atlas if this is the fontstash texture
						if ((tex->flags & 0x20000) && vk->useVirtualAtlas && vk->virtualAtlas) {
							imageInfo.imageView = vk->virtualAtlas->atlasImageView;
							imageInfo.sampler = vk->virtualAtlas->atlasSampler;
						} else {
							imageInfo.imageView = tex->imageView;
							imageInfo.sampler = tex->sampler;
						}

						VkWriteDescriptorSet imageWrite = {0};
						imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						imageWrite.dstSet = descriptorSet;
						imageWrite.dstBinding = 1;
						imageWrite.dstArrayElement = 0;
						imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						imageWrite.descriptorCount = 1;
						imageWrite.pImageInfo = &imageInfo;

						vkUpdateDescriptorSets(vk->device, 1, &imageWrite, 0, NULL);
					}
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout,
				                         0, 1, &descriptorSet, 0, NULL);
			}

			if (useInstancedPipeline) {
				// Instanced draw: 4 vertices per instance (triangle strip)
				if (call->instanceCount > 0) {
					vkCmdDraw(cmd, 4, call->instanceCount, 0, call->instanceOffset);
				}
			} else {
				// Regular draw
				if (call->triangleCount > 0) {
					vkCmdDraw(cmd, call->triangleCount, 1, call->triangleOffset, 0);
				}
			}
		}
	}

	// End rendering (dynamic rendering mode only)
	if (vk->hasDynamicRendering) {
		vk->vkCmdEndRenderingKHR(cmd);
	}

	vkEndCommandBuffer(cmd);

	// Submit command buffer if internal sync is enabled
	if (vk->flags & NVG_INTERNAL_SYNC) {
		// Wait for previous frame
		vkWaitForFences(vk->device, 1, &vk->frameFences[vk->currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(vk->device, 1, &vk->frameFences[vk->currentFrame]);

		// Submit command buffer
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &vk->imageAvailableSemaphores[vk->currentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &vk->renderFinishedSemaphores[vk->currentFrame];

		vkQueueSubmit(vk->queue, 1, &submitInfo, vk->frameFences[vk->currentFrame]);
	}

	// Advance text cache frame for LRU tracking
	if (vk->useTextCache && vk->textCache) {
		vknvg__advanceTextCacheFrame(vk);
	}

	// Reset counters
	vk->nverts = 0;
	vk->npaths = 0;
	vk->ncalls = 0;
	vk->nuniforms = 0;

	// Reset instance buffer if using instanced rendering
	if (vk->useTextInstancing && vk->glyphInstanceBuffer != NULL) {
		vknvg__resetGlyphInstances(vk->glyphInstanceBuffer);
	}
}

#endif // NANOVG_VK_RENDER_H
