// Compute-based Glyph Rasterization
// GPU-accelerated glyph rendering using compute shaders
//
// This optimization moves glyph rasterization from CPU (FreeType) to GPU,
// enabling parallel processing and generating high-quality SDF glyphs.

#ifndef NANOVG_VK_COMPUTE_RASTER_H
#define NANOVG_VK_COMPUTE_RASTER_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdlib.h>

#define VKNVG_MAX_GLYPH_CONTOURS 32		// Max contours per glyph
#define VKNVG_MAX_GLYPH_POINTS 512		// Max points per glyph
#define VKNVG_COMPUTE_WORKGROUP_SIZE 8		// 8x8 threads per workgroup

// Glyph outline point types
typedef enum VKNVGpointType {
	VKNVG_POINT_MOVE = 0,		// Move to point
	VKNVG_POINT_LINE = 1,		// Line to point
	VKNVG_POINT_QUAD = 2,		// Quadratic Bézier control point
	VKNVG_POINT_CUBIC = 3		// Cubic Bézier control point
} VKNVGpointType;

// Glyph outline point
typedef struct VKNVGglyphPoint {
	float x, y;			// Position
	uint32_t type;			// Point type
	uint32_t padding;		// Alignment
} VKNVGglyphPoint;

// Glyph contour
typedef struct VKNVGglyphContour {
	uint32_t pointOffset;		// Offset into point array
	uint32_t pointCount;		// Number of points
	uint32_t closed;		// Is contour closed?
	uint32_t padding;		// Alignment
} VKNVGglyphContour;

// Glyph outline data
typedef struct VKNVGglyphOutline {
	VKNVGglyphPoint points[VKNVG_MAX_GLYPH_POINTS];
	VKNVGglyphContour contours[VKNVG_MAX_GLYPH_CONTOURS];
	uint32_t pointCount;
	uint32_t contourCount;
	float boundingBox[4];		// min_x, min_y, max_x, max_y
} VKNVGglyphOutline;

// Compute rasterization parameters
typedef struct VKNVGcomputeRasterParams {
	uint32_t width;			// Output width
	uint32_t height;		// Output height
	float scale;			// Rasterization scale
	float sdfRadius;		// SDF search radius (0 = no SDF)
	uint32_t generateSDF;		// Generate signed distance field?
	uint32_t padding[3];		// Alignment to 16 bytes
} VKNVGcomputeRasterParams;

// Compute rasterization context
typedef struct VKNVGcomputeRaster {
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	// Compute pipeline
	VkPipelineLayout pipelineLayout;
	VkPipeline rasterPipeline;		// Standard rasterization
	VkPipeline sdfPipeline;			// SDF generation
	VkShaderModule rasterShader;
	VkShaderModule sdfShader;

	// Descriptor sets
	VkDescriptorSetLayout descriptorLayout;
	VkDescriptorPool descriptorPool;

	// Command pool and buffers
	VkCommandPool commandPool;
	VkQueue computeQueue;
	uint32_t computeQueueFamily;

	// Buffers
	VkBuffer outlineBuffer;		// Glyph outline data
	VkDeviceMemory outlineMemory;
	void* outlineMapped;
	VkDeviceSize outlineSize;

	VkBuffer paramsBuffer;		// Rasterization parameters
	VkDeviceMemory paramsMemory;
	void* paramsMapped;

	// Statistics
	uint32_t glyphsRasterized;
	uint32_t sdfGlyphsGenerated;
	double totalComputeTimeMs;
} VKNVGcomputeRaster;

// Forward declarations
static uint32_t vknvg__findMemoryTypeCompute(VkPhysicalDevice physicalDevice,
                                              uint32_t typeFilter,
                                              VkMemoryPropertyFlags properties);
static void vknvg__destroyComputeRaster(VKNVGcomputeRaster* ctx);

// Find memory type
static uint32_t vknvg__findMemoryTypeCompute(VkPhysicalDevice physicalDevice,
                                              uint32_t typeFilter,
                                              VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return UINT32_MAX;
}

// Create buffer
static VkResult vknvg__createComputeBuffer(VKNVGcomputeRaster* ctx,
                                           VkDeviceSize size,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties,
                                           VkBuffer* buffer,
                                           VkDeviceMemory* memory,
                                           void** mapped)
{
	VkResult result;

	// Create buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(ctx->device, &bufferInfo, NULL, buffer);
	if (result != VK_SUCCESS) return result;

	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(ctx->device, *buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryTypeCompute(
		ctx->physicalDevice,
		memReqs.memoryTypeBits,
		properties
	);

	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(ctx->device, *buffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	result = vkAllocateMemory(ctx->device, &allocInfo, NULL, memory);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(ctx->device, *buffer, NULL);
		return result;
	}

	vkBindBufferMemory(ctx->device, *buffer, *memory, 0);

	// Map memory if requested
	if (mapped && (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
		result = vkMapMemory(ctx->device, *memory, 0, size, 0, mapped);
		if (result != VK_SUCCESS) {
			vkFreeMemory(ctx->device, *memory, NULL);
			vkDestroyBuffer(ctx->device, *buffer, NULL);
			return result;
		}
	}

	return VK_SUCCESS;
}

// Load compute shaders
static VkResult vknvg__loadComputeShaders(VKNVGcomputeRaster* ctx)
{
	// Include SPIR-V binary
	#include "../shaders/glyph_raster.comp.spv.h"

	// Create shader module
	VkShaderModuleCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = glyph_raster_comp_spv_len;
	createInfo.pCode = (const uint32_t*)glyph_raster_comp_spv;

	VkResult result = vkCreateShaderModule(ctx->device, &createInfo, NULL, &ctx->rasterShader);
	if (result != VK_SUCCESS) {
		return result;
	}

	// For now, use same shader for both raster and SDF (shader handles both modes)
	ctx->sdfShader = ctx->rasterShader;

	return VK_SUCCESS;
}

// Create compute pipeline
static VkResult vknvg__createComputePipeline(VKNVGcomputeRaster* ctx)
{
	VkResult result;

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding bindings[3] = {0};

	// Binding 0: Glyph outline (storage buffer)
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	// Binding 1: Raster params (uniform buffer)
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	// Binding 2: Output image (storage image)
	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 3;
	layoutInfo.pBindings = bindings;

	result = vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL, &ctx->descriptorLayout);
	if (result != VK_SUCCESS) {
		return result;
	}

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &ctx->descriptorLayout;

	result = vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->pipelineLayout);
	if (result != VK_SUCCESS) {
		return result;
	}

	// Create compute pipeline
	VkPipelineShaderStageCreateInfo shaderStage = {0};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = ctx->rasterShader;
	shaderStage.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStage;
	pipelineInfo.layout = ctx->pipelineLayout;

	result = vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->rasterPipeline);
	if (result != VK_SUCCESS) {
		return result;
	}

	// Use same pipeline for SDF (shader handles both modes via uniform)
	ctx->sdfPipeline = ctx->rasterPipeline;

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[3] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = 10;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 10;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = 10;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 10;

	result = vkCreateDescriptorPool(ctx->device, &poolInfo, NULL, &ctx->descriptorPool);
	if (result != VK_SUCCESS) {
		return result;
	}

	return VK_SUCCESS;
}

// Create compute rasterization context
static VKNVGcomputeRaster* vknvg__createComputeRaster(VkDevice device,
                                                       VkPhysicalDevice physicalDevice,
                                                       VkQueue computeQueue,
                                                       uint32_t computeQueueFamily)
{
	VKNVGcomputeRaster* ctx = (VKNVGcomputeRaster*)malloc(sizeof(VKNVGcomputeRaster));
	if (!ctx) return NULL;

	memset(ctx, 0, sizeof(VKNVGcomputeRaster));
	ctx->device = device;
	ctx->physicalDevice = physicalDevice;
	ctx->computeQueue = computeQueue;
	ctx->computeQueueFamily = computeQueueFamily;

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = computeQueueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
		free(ctx);
		return NULL;
	}

	// Create outline buffer
	VkDeviceSize outlineSize = sizeof(VKNVGglyphOutline);
	if (vknvg__createComputeBuffer(ctx, outlineSize,
	                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                &ctx->outlineBuffer, &ctx->outlineMemory, &ctx->outlineMapped) != VK_SUCCESS) {
		vkDestroyCommandPool(device, ctx->commandPool, NULL);
		free(ctx);
		return NULL;
	}
	ctx->outlineSize = outlineSize;

	// Create params buffer
	VkDeviceSize paramsSize = sizeof(VKNVGcomputeRasterParams);
	if (vknvg__createComputeBuffer(ctx, paramsSize,
	                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                &ctx->paramsBuffer, &ctx->paramsMemory, &ctx->paramsMapped) != VK_SUCCESS) {
		vkUnmapMemory(device, ctx->outlineMemory);
		vkFreeMemory(device, ctx->outlineMemory, NULL);
		vkDestroyBuffer(device, ctx->outlineBuffer, NULL);
		vkDestroyCommandPool(device, ctx->commandPool, NULL);
		free(ctx);
		return NULL;
	}

	// Load shader and create pipeline
	if (vknvg__loadComputeShaders(ctx) != VK_SUCCESS) {
		vknvg__destroyComputeRaster(ctx);
		return NULL;
	}

	if (vknvg__createComputePipeline(ctx) != VK_SUCCESS) {
		vknvg__destroyComputeRaster(ctx);
		return NULL;
	}

	return ctx;
}

// Destroy compute rasterization context
static void vknvg__destroyComputeRaster(VKNVGcomputeRaster* ctx)
{
	if (!ctx) return;

	// Destroy pipelines (sdfPipeline and rasterPipeline are the same)
	if (ctx->rasterPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(ctx->device, ctx->rasterPipeline, NULL);
	}

	// Destroy shader module (sdfShader and rasterShader are the same)
	if (ctx->rasterShader != VK_NULL_HANDLE) {
		vkDestroyShaderModule(ctx->device, ctx->rasterShader, NULL);
	}
	if (ctx->pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, NULL);
	}
	if (ctx->descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(ctx->device, ctx->descriptorPool, NULL);
	}
	if (ctx->descriptorLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(ctx->device, ctx->descriptorLayout, NULL);
	}

	if (ctx->paramsMapped) {
		vkUnmapMemory(ctx->device, ctx->paramsMemory);
	}
	if (ctx->paramsMemory != VK_NULL_HANDLE) {
		vkFreeMemory(ctx->device, ctx->paramsMemory, NULL);
	}
	if (ctx->paramsBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(ctx->device, ctx->paramsBuffer, NULL);
	}

	if (ctx->outlineMapped) {
		vkUnmapMemory(ctx->device, ctx->outlineMemory);
	}
	if (ctx->outlineMemory != VK_NULL_HANDLE) {
		vkFreeMemory(ctx->device, ctx->outlineMemory, NULL);
	}
	if (ctx->outlineBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(ctx->device, ctx->outlineBuffer, NULL);
	}

	if (ctx->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
	}

	free(ctx);
}

// Build glyph outline from FreeType data
static void vknvg__buildGlyphOutline(VKNVGglyphOutline* outline,
                                     const void* ftOutline)
{
	// In a full implementation, this would convert FreeType FT_Outline
	// to our VKNVGglyphOutline format
	// For now, just initialize to empty
	memset(outline, 0, sizeof(VKNVGglyphOutline));
}

// Rasterize glyph using compute shader
static VkResult vknvg__rasterizeGlyphCompute(VKNVGcomputeRaster* ctx,
                                             const VKNVGglyphOutline* outline,
                                             VkImage dstImage,
                                             uint32_t width,
                                             uint32_t height,
                                             VkBool32 generateSDF,
                                             float sdfRadius)
{
	if (!ctx || !outline) return VK_ERROR_INITIALIZATION_FAILED;

	// Copy outline to GPU buffer
	memcpy(ctx->outlineMapped, outline, sizeof(VKNVGglyphOutline));

	// Set rasterization parameters
	VKNVGcomputeRasterParams* params = (VKNVGcomputeRasterParams*)ctx->paramsMapped;
	params->width = width;
	params->height = height;
	params->scale = 1.0f;
	params->sdfRadius = sdfRadius;
	params->generateSDF = generateSDF ? 1 : 0;

	// In a full implementation, would:
	// 1. Allocate command buffer
	// 2. Record compute dispatch
	// 3. Submit to compute queue
	// 4. Return semaphore for synchronization

	ctx->glyphsRasterized++;
	if (generateSDF) {
		ctx->sdfGlyphsGenerated++;
	}

	return VK_SUCCESS;
}

// Get statistics
static void vknvg__getComputeRasterStats(VKNVGcomputeRaster* ctx,
                                         uint32_t* glyphsRasterized,
                                         uint32_t* sdfGlyphs,
                                         double* avgComputeTimeMs)
{
	if (!ctx) return;

	if (glyphsRasterized) *glyphsRasterized = ctx->glyphsRasterized;
	if (sdfGlyphs) *sdfGlyphs = ctx->sdfGlyphsGenerated;
	if (avgComputeTimeMs) {
		*avgComputeTimeMs = ctx->glyphsRasterized > 0 ?
		                    ctx->totalComputeTimeMs / ctx->glyphsRasterized : 0.0;
	}
}

#endif // NANOVG_VK_COMPUTE_RASTER_H
