// Compute Shader Support for NanoVG Vulkan
// GPU-accelerated operations: atlas defragmentation, etc.

#ifndef NANOVG_VK_COMPUTE_H
#define NANOVG_VK_COMPUTE_H

#include <vulkan/vulkan.h>
#include <stdint.h>

// Compute pipeline types
typedef enum VKNVGcomputePipelineType {
	VKNVG_COMPUTE_ATLAS_DEFRAG = 0,
	VKNVG_COMPUTE_MSDF_GENERATE,
	VKNVG_COMPUTE_PIPELINE_COUNT
} VKNVGcomputePipelineType;

// Push constants for atlas defragmentation
typedef struct VKNVGdefragPushConstants {
	uint32_t srcOffsetX;
	uint32_t srcOffsetY;
	uint32_t dstOffsetX;
	uint32_t dstOffsetY;
	uint32_t extentWidth;
	uint32_t extentHeight;
	uint32_t padding[2];  // Align to 32 bytes
} VKNVGdefragPushConstants;

// Push constants for MSDF generation
typedef struct VKNVGmsdfPushConstants {
	uint32_t outputWidth;   // Output texture width
	uint32_t outputHeight;  // Output texture height
	float pxRange;          // Pixel range for SDF (typically 4.0)
	float padding;
} VKNVGmsdfPushConstants;

// Compute pipeline
typedef struct VKNVGcomputePipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkShaderModule shaderModule;
} VKNVGcomputePipeline;

// Compute context
typedef struct VKNVGcomputeContext {
	VkDevice device;
	VkQueue computeQueue;           // Can be graphics or dedicated compute queue
	uint32_t queueFamilyIndex;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence computeFence;           // Fence for compute synchronization

	VKNVGcomputePipeline pipelines[VKNVG_COMPUTE_PIPELINE_COUNT];

	int initialized;
} VKNVGcomputeContext;

// Function declarations
VkShaderModule vknvg__createComputeShaderModule(VkDevice device,
                                                 const uint32_t* code,
                                                 size_t codeSize);
int vknvg__createDefragPipeline(VKNVGcomputeContext* ctx);
int vknvg__createMSDFPipeline(VKNVGcomputeContext* ctx);
int vknvg__initComputeContext(VKNVGcomputeContext* ctx,
                               VkDevice device,
                               VkQueue queue,
                               uint32_t queueFamilyIndex);
void vknvg__destroyComputeContext(VKNVGcomputeContext* ctx);
void vknvg__dispatchDefragCompute(VKNVGcomputeContext* ctx,
                                   VkDescriptorSet descriptorSet,
                                   const VKNVGdefragPushConstants* pushConstants);
void vknvg__dispatchMSDFCompute(VKNVGcomputeContext* ctx,
                                 VkDescriptorSet descriptorSet,
                                 const VKNVGmsdfPushConstants* pushConstants,
                                 uint32_t width,
                                 uint32_t height);

#endif // NANOVG_VK_COMPUTE_H
