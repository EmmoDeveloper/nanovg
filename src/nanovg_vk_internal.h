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

#ifndef NANOVG_VK_INTERNAL_H
#define NANOVG_VK_INTERNAL_H

#include <vulkan/vulkan.h>
#include <pthread.h>
#include "nanovg_vk_types.h"

// Forward declarations
typedef struct VKNVGglyphInstanceBuffer VKNVGglyphInstanceBuffer;
typedef struct VKNVGtextRunCache VKNVGtextRunCache;

enum VKNVGshaderType {
	NSVG_SHADER_FILLGRAD,
	NSVG_SHADER_FILLIMG,
	NSVG_SHADER_SIMPLE,
	NSVG_SHADER_IMAGE,
};

struct VKNVGblend {
	VkBlendFactor srcRGB;
	VkBlendFactor dstRGB;
	VkBlendFactor srcAlpha;
	VkBlendFactor dstAlpha;
};
typedef struct VKNVGblend VKNVGblend;

struct VKNVGpipelineVariant {
	VKNVGblend blend;
	VkPipeline pipeline;
	struct VKNVGpipelineVariant* next;
};
typedef struct VKNVGpipelineVariant VKNVGpipelineVariant;

struct VKNVGtexture {
	int id;
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
	VkSampler sampler;
	int width, height;
	int type;
	int flags;
};
typedef struct VKNVGtexture VKNVGtexture;

struct VKNVGbuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;
	VkDeviceSize size;
	VkDeviceSize used;
};
typedef struct VKNVGbuffer VKNVGbuffer;

struct VKNVGcall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
	VKNVGblend blend;
	VkBool32 useInstancing;
	int instanceOffset;
	int instanceCount;
};
typedef struct VKNVGcall VKNVGcall;

struct VKNVGpath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};
typedef struct VKNVGpath VKNVGpath;

struct VKNVGfragUniforms {
	float scissorMat[12];
	float paintMat[12];
	struct NVGcolor innerCol;
	struct NVGcolor outerCol;
	float scissorExt[2];
	float scissorScale[2];
	float extent[2];
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
};
typedef struct VKNVGfragUniforms VKNVGfragUniforms;

typedef void (VKAPI_PTR *PFN_vkCmdSetColorBlendEquationEXT_)(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const void* pColorBlendEquations);
typedef void (VKAPI_PTR *PFN_vkCmdSetColorBlendEnableEXT_)(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkBool32* pColorBlendEnables);
typedef void (VKAPI_PTR *PFN_vkCmdBeginRenderingKHR_)(VkCommandBuffer commandBuffer, const void* pRenderingInfo);
typedef void (VKAPI_PTR *PFN_vkCmdEndRenderingKHR_)(VkCommandBuffer commandBuffer);

enum VKNVGvendor {
	VKNVG_VENDOR_UNKNOWN = 0,
	VKNVG_VENDOR_NVIDIA = 0x10DE,
	VKNVG_VENDOR_AMD = 0x1002,
	VKNVG_VENDOR_INTEL = 0x8086,
	VKNVG_VENDOR_ARM = 0x13B5,
	VKNVG_VENDOR_QUALCOMM = 0x5143,
	VKNVG_VENDOR_IMGTECH = 0x1010,
};
typedef enum VKNVGvendor VKNVGvendor;

enum VKNVGgpuTier {
	VKNVG_GPU_TIER_UNKNOWN = 0,
	VKNVG_GPU_TIER_INTEGRATED,
	VKNVG_GPU_TIER_ENTRY,
	VKNVG_GPU_TIER_MIDRANGE,
	VKNVG_GPU_TIER_HIGHEND,
	VKNVG_GPU_TIER_ENTHUSIAST
};
typedef enum VKNVGgpuTier VKNVGgpuTier;

struct VKNVGplatformOptimizations {
	VKNVGvendor vendor;
	VKNVGgpuTier tier;
	uint32_t vendorID;
	uint32_t deviceID;
	char deviceName[256];
	VkDeviceSize optimalVertexBufferSize;
	VkDeviceSize optimalUniformBufferSize;
	VkDeviceSize optimalStagingBufferSize;
	uint32_t preferredDeviceLocalMemoryType;
	uint32_t preferredHostVisibleMemoryType;
	VkBool32 useDeviceLocalHostVisible;
	VkBool32 usePinnedMemory;
	VkBool32 useCoherentMemory;
	VkBool32 useAsyncCompute;
	VkBool32 usePushDescriptors;
	VkBool32 useLargePages;
	VkBool32 useMemoryBudget;
	uint32_t computeQueueFamilyIndex;
	VkQueue computeQueue;
	VkCommandPool computeCommandPool;
};
typedef struct VKNVGplatformOptimizations VKNVGplatformOptimizations;

struct VKNVGthreadContext {
	uint32_t threadIndex;
	VkCommandBuffer* secondaryCommandBuffers;
	struct VKNVGcontext* vk;
	pthread_t thread;
	VkBool32 active;
	VkBool32 shouldExit;
};
typedef struct VKNVGthreadContext VKNVGthreadContext;

struct VKNVGworkQueue {
	VKNVGcall* calls;
	uint32_t callCount;
	uint32_t nextCallIndex;
	pthread_mutex_t mutex;
	pthread_cond_t workAvailable;
	pthread_cond_t workComplete;
	uint32_t activeWorkers;
	VkBool32 workSubmitted;
};
typedef struct VKNVGworkQueue VKNVGworkQueue;

struct VKNVGcontext {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	uint32_t queueFamilyIndex;
	VkRenderPass renderPass;
	uint32_t subpass;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkBool32 ownsCommandPool;
	VkBool32 ownsDescriptorPool;
	VkBool32 ownsRenderPass;
	VkBool32 hasDynamicBlendState;
	VkBool32 hasDynamicRendering;
	VkBool32 hasStencilBuffer;
	PFN_vkCmdSetColorBlendEquationEXT_ vkCmdSetColorBlendEquationEXT;
	PFN_vkCmdSetColorBlendEnableEXT_ vkCmdSetColorBlendEnableEXT;
	PFN_vkCmdBeginRenderingKHR_ vkCmdBeginRenderingKHR;
	PFN_vkCmdEndRenderingKHR_ vkCmdEndRenderingKHR;
	VkFormat colorFormat;
	VkFormat depthStencilFormat;
	VkImageView colorImageView;
	VkImageView depthStencilImageView;
	uint32_t maxFrames;
	uint32_t currentFrame;
	VkCommandBuffer* commandBuffers;
	VkFence* frameFences;
	VkSemaphore* imageAvailableSemaphores;
	VkSemaphore* renderFinishedSemaphores;
	VKNVGbuffer* vertexBuffers;
	VKNVGbuffer* uniformBuffer;
	VKNVGtexture* textures;
	int ntextures;
	int ctextures;
	int textureId;
	VkPipelineLayout pipelineLayout;
	VkPipeline fillPipeline;
	VkPipeline fillStencilPipeline;
	VkPipeline fillAAPipeline;
	VkPipeline strokePipeline;
	VkPipeline trianglesPipeline;
	VkPipeline textPipeline;
	VkPipeline textSDFPipeline;
	VkPipeline textSubpixelPipeline;
	VkPipeline textMSDFPipeline;
	VkPipeline textColorPipeline;
	VkPipeline textInstancedPipeline;
	VkPipeline textInstancedSDFPipeline;
	VkPipeline textInstancedSubpixelPipeline;
	VkPipeline textInstancedMSDFPipeline;
	VkPipeline textInstancedColorPipeline;
	VKNVGpipelineVariant** fillPipelineVariants;
	VKNVGpipelineVariant** strokePipelineVariants;
	VKNVGpipelineVariant** trianglesPipelineVariants;
	int pipelineVariantCacheSize;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet* descriptorSets;
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	VkShaderModule textInstancedVertShaderModule;
	VkShaderModule textSDFFragShaderModule;
	VkShaderModule textSubpixelFragShaderModule;
	VkShaderModule textMSDFFragShaderModule;
	VkShaderModule textColorFragShaderModule;
	VkShaderModule computeShaderModule;
	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorSet* computeDescriptorSets;
	VKNVGbuffer* computeInputBuffers;
	VKNVGbuffer* computeOutputBuffers;
	VKNVGcall* calls;
	int ncalls;
	int ccalls;
	VKNVGpath* paths;
	int npaths;
	int cpaths;
	NVGvertex* verts;
	int nverts;
	int cverts;
	unsigned char* uniforms;
	int nuniforms;
	int cuniforms;
	int flags;
	float view[2];
	int fragSize;
	VkSampleCountFlagBits sampleCount;
	VKNVGprofiling profiling;
	double frameStartTime;
	VKNVGplatformOptimizations platformOpt;
	uint32_t threadCount;
	VKNVGthreadContext* threadContexts;
	VKNVGworkQueue workQueue;
	VKNVGglyphInstanceBuffer* glyphInstanceBuffer;
	VkBool32 useTextInstancing;
	struct VKNVGvirtualAtlas* virtualAtlas;	// Virtual atlas for CJK support
	VkBool32 useVirtualAtlas;
	void* fontStashContext;	// FONScontext* for virtual atlas (set after init)
	int fontstashTextureId;		// Texture ID of fontstash atlas (for redirection)
	uint32_t debugUpdateTextureCount;	// DEBUG: Track renderUpdateTexture calls
	// Pending glyph uploads (coords stored before upload)
	struct {
		int origX, origY, w, h;
		int atlasX, atlasY;
	} pendingGlyphUploads[256];
	int pendingGlyphUploadCount;
	// Text run caching
	struct VKNVGtextRunCache* textCache;	// Text run cache (NULL if disabled)
	VkBool32 useTextCache;					// Enable text run caching
	VkRenderPass textCacheRenderPass;		// Render pass for text-to-texture
};
typedef struct VKNVGcontext VKNVGcontext;

#endif // NANOVG_VK_INTERNAL_H
