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

#ifndef NANOVG_VK_TYPES_H
#define NANOVG_VK_TYPES_H

#include <vulkan/vulkan.h>

enum NVGcreateFlags {
	NVG_ANTIALIAS 		= 1<<0,
	NVG_STENCIL_STROKES	= 1<<1,
	NVG_DEBUG 			= 1<<2,
	NVG_BLEND_PIPELINE_VARIANTS = 1<<3,
	NVG_DYNAMIC_RENDERING = 1<<4,
	NVG_INTERNAL_RENDER_PASS = 1<<5,
	NVG_INTERNAL_SYNC = 1<<6,
	NVG_SDF_TEXT = 1<<7,
	NVG_SUBPIXEL_TEXT = 1<<8,
	NVG_MSDF_TEXT = 1<<13,
	NVG_COLOR_TEXT = 1<<14,
	NVG_VIRTUAL_ATLAS = 1<<15,	// Enable virtual atlas for CJK support
	NVG_TEXT_CACHE = 1<<16,		// Enable text run caching
	NVG_MSAA = 1<<9,
	NVG_PROFILING = 1<<10,
	NVG_MULTI_THREADED = 1<<11,
	NVG_COMPUTE_TESSELLATION = 1<<12,
};

struct NVGVkCreateInfo {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	uint32_t queueFamilyIndex;
	VkRenderPass renderPass;
	uint32_t subpass;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	uint32_t maxFrames;
	VkFormat colorFormat;
	VkFormat depthStencilFormat;
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;
	VkSampleCountFlagBits sampleCount;
	uint32_t threadCount;
};
typedef struct NVGVkCreateInfo NVGVkCreateInfo;

struct VKNVGprofiling {
	double cpuFrameTime;
	double cpuGeometryTime;
	double cpuRenderTime;
	double gpuFrameTime;
	uint32_t drawCalls;
	uint32_t fillCount;
	uint32_t strokeCount;
	uint32_t triangleCount;
	uint32_t vertexCount;
	uint32_t textureBinds;
	uint32_t pipelineSwitches;
	uint64_t vertexBufferSize;
	uint64_t uniformBufferSize;
	uint64_t textureMemoryUsed;
	VkQueryPool timestampQueryPool;
	uint64_t timestampPeriod;
	VkBool32 timestampsAvailable;
};
typedef struct VKNVGprofiling VKNVGprofiling;

#endif // NANOVG_VK_TYPES_H
