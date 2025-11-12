#include "nvg_font_gpu_raster.h"
#include "nvg_font_internal.h"
#include "../vulkan/nvg_vk_compute.h"
#include "../vulkan/nvg_vk_types.h"
#include "../shaders/glyph_raster_comp.h"
#include <freetype/ftoutln.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Queued GPU rasterization job
typedef struct {
	NVGGpuGlyphData glyphData;
	unsigned int glyphIndex;
	int width;
	int height;
	int atlasX;
	int atlasY;
	int atlasIndex;
} NVGGpuRasterJob;

#define MAX_GPU_RASTER_JOBS 256

// Internal GPU rasterizer state
struct NVGFontGpuRasterizer {
	void* vkContext;              // NVGVkContext* (opaque)
	NVGVkComputePipeline pipeline;
	NVGGpuRasterParams params;
	NVGRasterMode mode;           // CPU/GPU/AUTO mode

	// Staging buffer for glyph data
	VkBuffer glyphBuffer;
	VkDeviceMemory glyphMemory;
	void* glyphMapped;
	VkDeviceSize glyphBufferSize;

	// Uniform buffer for atlas parameters
	VkBuffer atlasParamsBuffer;
	VkDeviceMemory atlasParamsMemory;
	void* atlasParamsMapped;
	VkDeviceSize atlasParamsSize;

	// Command buffer for compute dispatches
	VkCommandBuffer computeCmd;
	VkFence computeFence;

	// Queue of pending rasterization jobs
	NVGGpuRasterJob jobs[MAX_GPU_RASTER_JOBS];
	int jobCount;

	int valid;
};

// Context for FT_Outline_Decompose callbacks
typedef struct {
	NVGGpuGlyphData* glyphData;
	FT_Vector firstPoint;
	FT_Vector currentPoint;
	int currentContour;
	int overflowed;
} OutlineDecomposeContext;

// FT_Outline_Decompose callbacks
static int gpu_moveTo(const FT_Vector* to, void* user) {
	OutlineDecomposeContext* ctx = (OutlineDecomposeContext*)user;

	// Start new contour
	if (ctx->glyphData->contourCount >= NVG_GPU_MAX_CONTOURS) {
		ctx->overflowed = 1;
		return 1;
	}

	// Finalize previous contour
	if (ctx->currentContour >= 0) {
		printf("    [FINALIZE] Contour %d: firstPoint=(%.2f,%.2f) currentPoint=(%.2f,%.2f)\n",
		       ctx->currentContour, ctx->firstPoint.x/64.0f, ctx->firstPoint.y/64.0f,
		       ctx->currentPoint.x/64.0f, ctx->currentPoint.y/64.0f);
		// Add implicit closing curve from currentPoint back to firstPoint
		// FreeType contours are always closed
		float dx = ctx->firstPoint.x - ctx->currentPoint.x;
		float dy = ctx->firstPoint.y - ctx->currentPoint.y;
		float dist = sqrtf((dx/64.0f)*(dx/64.0f) + (dy/64.0f)*(dy/64.0f));

		// Only add closing curve if not already closed (distance > epsilon)
		if (dist > 0.01f && ctx->glyphData->curveCount < NVG_GPU_MAX_CURVES) {
			printf("    [CLOSE] Adding closing curve for contour %d (dist=%.4f)\n", ctx->currentContour, dist);
			NVGGpuCurve* closingCurve = &ctx->glyphData->curves[ctx->glyphData->curveCount++];
			closingCurve->type = NVG_GPU_CURVE_LINEAR;
			closingCurve->contourId = ctx->currentContour;

			// Line from currentPoint to firstPoint
			closingCurve->p0[0] = ctx->currentPoint.x / 64.0f;
			closingCurve->p0[1] = ctx->currentPoint.y / 64.0f;
			closingCurve->p3[0] = ctx->firstPoint.x / 64.0f;
			closingCurve->p3[1] = ctx->firstPoint.y / 64.0f;

			// Linear curve: control points = endpoints
			closingCurve->p1[0] = closingCurve->p0[0];
			closingCurve->p1[1] = closingCurve->p0[1];
			closingCurve->p2[0] = closingCurve->p3[0];
			closingCurve->p2[1] = closingCurve->p3[1];
		}

		NVGGpuContour* contour = &ctx->glyphData->contours[ctx->currentContour];
		contour->curveCount = ctx->glyphData->curveCount - contour->firstCurve;
	}

	// Start new contour
	ctx->currentContour = ctx->glyphData->contourCount++;
	NVGGpuContour* contour = &ctx->glyphData->contours[ctx->currentContour];
	contour->firstCurve = ctx->glyphData->curveCount;
	contour->curveCount = 0;

	ctx->firstPoint = *to;
	ctx->currentPoint = *to;

	return 0;
}

static int gpu_lineTo(const FT_Vector* to, void* user) {
	OutlineDecomposeContext* ctx = (OutlineDecomposeContext*)user;
	NVGGpuGlyphData* data = ctx->glyphData;

	if (data->curveCount >= NVG_GPU_MAX_CURVES) {
		ctx->overflowed = 1;
		return 1;
	}

	NVGGpuCurve* curve = &data->curves[data->curveCount++];
	curve->type = NVG_GPU_CURVE_LINEAR;
	curve->contourId = ctx->currentContour;

	// Convert FreeType 26.6 fixed point to float
	curve->p0[0] = ctx->currentPoint.x / 64.0f;
	curve->p0[1] = ctx->currentPoint.y / 64.0f;
	curve->p3[0] = to->x / 64.0f;
	curve->p3[1] = to->y / 64.0f;

	// For linear curves, control points equal endpoints
	curve->p1[0] = curve->p0[0];
	curve->p1[1] = curve->p0[1];
	curve->p2[0] = curve->p3[0];
	curve->p2[1] = curve->p3[1];

	ctx->currentPoint = *to;
	return 0;
}

static int gpu_conicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
	// Convert quadratic Bezier to cubic
	// Cubic control points: p1 = p0 + 2/3*(control-p0), p2 = p3 + 2/3*(control-p3)
	OutlineDecomposeContext* ctx = (OutlineDecomposeContext*)user;
	NVGGpuGlyphData* data = ctx->glyphData;

	if (data->curveCount >= NVG_GPU_MAX_CURVES) {
		ctx->overflowed = 1;
		return 1;
	}

	NVGGpuCurve* curve = &data->curves[data->curveCount++];
	curve->type = NVG_GPU_CURVE_CUBIC;
	curve->contourId = ctx->currentContour;

	// Convert FreeType 26.6 fixed point to float
	float p0x = ctx->currentPoint.x / 64.0f;
	float p0y = ctx->currentPoint.y / 64.0f;
	float cx = control->x / 64.0f;
	float cy = control->y / 64.0f;
	float p3x = to->x / 64.0f;
	float p3y = to->y / 64.0f;

	curve->p0[0] = p0x;
	curve->p0[1] = p0y;
	curve->p1[0] = p0x + (2.0f / 3.0f) * (cx - p0x);
	curve->p1[1] = p0y + (2.0f / 3.0f) * (cy - p0y);
	curve->p2[0] = p3x + (2.0f / 3.0f) * (cx - p3x);
	curve->p2[1] = p3y + (2.0f / 3.0f) * (cy - p3y);
	curve->p3[0] = p3x;
	curve->p3[1] = p3y;

	ctx->currentPoint = *to;
	return 0;
}

static int gpu_cubicTo(const FT_Vector* c1, const FT_Vector* c2,
                       const FT_Vector* to, void* user) {
	// Already cubic, direct mapping
	OutlineDecomposeContext* ctx = (OutlineDecomposeContext*)user;
	NVGGpuGlyphData* data = ctx->glyphData;

	if (data->curveCount >= NVG_GPU_MAX_CURVES) {
		ctx->overflowed = 1;
		return 1;
	}

	NVGGpuCurve* curve = &data->curves[data->curveCount++];
	curve->type = NVG_GPU_CURVE_CUBIC;
	curve->contourId = ctx->currentContour;

	// Convert FreeType 26.6 fixed point to float
	curve->p0[0] = ctx->currentPoint.x / 64.0f;
	curve->p0[1] = ctx->currentPoint.y / 64.0f;
	curve->p1[0] = c1->x / 64.0f;
	curve->p1[1] = c1->y / 64.0f;
	curve->p2[0] = c2->x / 64.0f;
	curve->p2[1] = c2->y / 64.0f;
	curve->p3[0] = to->x / 64.0f;
	curve->p3[1] = to->y / 64.0f;

	ctx->currentPoint = *to;
	return 0;
}

int nvgFont_ExtractOutline(FT_Face face, unsigned int glyphIndex,
                            NVGGpuGlyphData* glyphData,
                            int width, int height) {
	if (!face || !glyphData) return 0;

	memset(glyphData, 0, sizeof(NVGGpuGlyphData));

	// Load glyph outline
	if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE) != 0) {
		return 0;
	}

	// Check if glyph has outline
	if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
		return 0;
	}

	FT_Outline* outline = &face->glyph->outline;

	// Check complexity limits
	if (outline->n_contours > NVG_GPU_MAX_CONTOURS) {
		return 0;
	}
	if (outline->n_points > NVG_GPU_MAX_CURVES * 3) {
		return 0;  // Conservative estimate
	}

	// Get bounding box
	FT_BBox bbox;
	FT_Outline_Get_CBox(outline, &bbox);

	glyphData->bbox[0] = bbox.xMin / 64.0f;
	glyphData->bbox[1] = bbox.yMin / 64.0f;
	glyphData->bbox[2] = bbox.xMax / 64.0f;
	glyphData->bbox[3] = bbox.yMax / 64.0f;

	// Calculate scale (glyph units to pixels)
	float glyphWidth = (bbox.xMax - bbox.xMin) / 64.0f;

	if (glyphWidth > 0) {
		glyphData->scale = width / glyphWidth;
	} else {
		glyphData->scale = 1.0f;
	}

	glyphData->outputWidth = width;
	glyphData->outputHeight = height;

	// Decompose outline using callbacks
	OutlineDecomposeContext ctx = {
		.glyphData = glyphData,
		.currentContour = -1,
		.overflowed = 0
	};

	FT_Outline_Funcs funcs = {
		.move_to = gpu_moveTo,
		.line_to = gpu_lineTo,
		.conic_to = gpu_conicTo,
		.cubic_to = gpu_cubicTo,
		.shift = 0,
		.delta = 0
	};

	if (FT_Outline_Decompose(outline, &funcs, &ctx) != 0) {
		return 0;
	}

	if (ctx.overflowed) {
		return 0;
	}

	// Finalize last contour
	if (ctx.currentContour >= 0) {
		// Add implicit closing curve for last contour
		float dx = ctx.firstPoint.x - ctx.currentPoint.x;
		float dy = ctx.firstPoint.y - ctx.currentPoint.y;
		float dist = sqrtf((dx/64.0f)*(dx/64.0f) + (dy/64.0f)*(dy/64.0f));

		if (dist > 0.01f && glyphData->curveCount < NVG_GPU_MAX_CURVES) {
			printf("    [CLOSE_LAST] Adding closing curve for contour %d (dist=%.4f)\n", ctx.currentContour, dist);
			NVGGpuCurve* closingCurve = &glyphData->curves[glyphData->curveCount++];
			closingCurve->type = NVG_GPU_CURVE_LINEAR;
			closingCurve->contourId = ctx.currentContour;

			closingCurve->p0[0] = ctx.currentPoint.x / 64.0f;
			closingCurve->p0[1] = ctx.currentPoint.y / 64.0f;
			closingCurve->p3[0] = ctx.firstPoint.x / 64.0f;
			closingCurve->p3[1] = ctx.firstPoint.y / 64.0f;

			closingCurve->p1[0] = closingCurve->p0[0];
			closingCurve->p1[1] = closingCurve->p0[1];
			closingCurve->p2[0] = closingCurve->p3[0];
			closingCurve->p2[1] = closingCurve->p3[1];
		}

		NVGGpuContour* contour = &glyphData->contours[ctx.currentContour];
		contour->curveCount = glyphData->curveCount - contour->firstCurve;
	}

	// Compute winding direction for each contour using signed area
	for (uint32_t i = 0; i < glyphData->contourCount; i++) {
		NVGGpuContour* contour = &glyphData->contours[i];
		float signedArea = 0.0f;

		// Compute signed area using shoelace formula
		for (uint32_t j = 0; j < contour->curveCount; j++) {
			NVGGpuCurve* curve = &glyphData->curves[contour->firstCurve + j];
			// For each segment, add (x0 * y1 - x1 * y0) / 2
			// Using endpoints p0 and p3 (works for both linear and curves as approximation)
			signedArea += (curve->p0[0] * curve->p3[1] - curve->p3[0] * curve->p0[1]);
		}

		// Positive area = CCW = outer contour (winding +1)
		// Negative area = CW = hole (winding -1)
		contour->winding = (signedArea > 0.0f) ? 1 : -1;
	}

	return 1;
}

int nvgFont_ShouldUseGPU(NVGFontSystem* fs, FT_Face face,
                         unsigned int glyphIndex, int width, int height) {
	if (!fs || !fs->gpuRasterizer || !face) return 0;

	// Check rasterization mode
	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	NVGRasterMode mode = raster->mode;

	// CPU mode: never use GPU
	if (mode == NVG_RASTER_CPU) {
		return 0;
	}

	// Load glyph to check outline
	if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE) != 0) {
		return 0;
	}

	// Must have outline format
	if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
		return 0;
	}

	int nContours = face->glyph->outline.n_contours;
	int nPoints = face->glyph->outline.n_points;

	// Too complex for GPU buffers
	if (nContours > NVG_GPU_MAX_CONTOURS) return 0;
	if (nPoints > NVG_GPU_MAX_CURVES * 3) return 0;

	// GPU mode: use GPU for all glyphs with positive dimensions
	if (mode == NVG_RASTER_GPU) {
		if (width > 0 && height > 0) {
			printf("[GPU Check] glyph %u: ACCEPTED for GPU rasterization (FORCED)\n", glyphIndex);
			return 1;
		}
		return 0;
	}

	// AUTO mode: use heuristics to decide
	// For now, use GPU for all glyphs with positive dimensions
	// In future, could add size thresholds or complexity checks
	if (width > 0 && height > 0) {
		printf("[GPU Check] glyph %u: ACCEPTED for GPU rasterization (AUTO)\n", glyphIndex);
		return 1;
	}

	return 0;
}

int nvgFont_InitGpuRasterizer(NVGFontSystem* fs, void* vkContext,
                               const NVGGpuRasterParams* params) {
	if (!fs || !vkContext) return 0;

	// Allocate rasterizer state
	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)malloc(sizeof(NVGFontGpuRasterizer));
	if (!raster) return 0;

	memset(raster, 0, sizeof(NVGFontGpuRasterizer));
	raster->vkContext = vkContext;
	raster->mode = NVG_RASTER_GPU;  // Default to GPU mode

	// Set parameters
	if (params) {
		raster->params = *params;
	} else {
		// Defaults
		raster->params.pxRange = 1.5f;
		raster->params.useWinding = 1;  // Use winding number method
		raster->params.maxCurvesPerGlyph = NVG_GPU_MAX_CURVES;
		raster->params.maxContoursPerGlyph = NVG_GPU_MAX_CONTOURS;
	}

	NVGVkContext* vk = (NVGVkContext*)vkContext;

	// Create compute pipeline
	VkDescriptorSetLayoutBinding bindings[3] = {
		// Binding 0: Glyph data buffer (storage buffer, read-only)
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
		},
		// Binding 1: Atlas image (storage image, write-only)
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
		},
		// Binding 2: Atlas parameters (uniform buffer)
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
		}
	};

	// Push constants size (16 bytes)
	uint32_t pushConstantSize = sizeof(NVGGpuRasterPushConstants);

	if (!nvgvk_create_compute_pipeline(vk,
	                                    (uint32_t*)src_shaders_glyph_raster_comp_spv,
	                                    src_shaders_glyph_raster_comp_spv_len,
	                                    bindings, 3,
	                                    pushConstantSize,
	                                    &raster->pipeline)) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to create compute pipeline\n");
		free(raster);
		return 0;
	}

	// Allocate staging buffer for glyph data (host-visible, device-local)
	raster->glyphBufferSize = sizeof(NVGGpuGlyphData);

	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = raster->glyphBufferSize,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateBuffer(vk->device, &bufferInfo, NULL, &raster->glyphBuffer) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to create glyph buffer\n");
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	// Allocate memory (host-visible and coherent for easy updates)
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(vk->device, raster->glyphBuffer, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProps);

	uint32_t memoryTypeIndex = UINT32_MAX;
	VkMemoryPropertyFlags desiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReqs.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == UINT32_MAX) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to find suitable memory type\n");
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memReqs.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	if (vkAllocateMemory(vk->device, &allocInfo, NULL, &raster->glyphMemory) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to allocate glyph memory\n");
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	vkBindBufferMemory(vk->device, raster->glyphBuffer, raster->glyphMemory, 0);

	// Map memory for persistent access
	if (vkMapMemory(vk->device, raster->glyphMemory, 0, raster->glyphBufferSize, 0,
	                &raster->glyphMapped) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to map glyph memory\n");
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	// Bind glyph buffer to descriptor set
	nvgvk_update_compute_descriptors(vk, &raster->pipeline, 0,
	                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	                                  raster->glyphBuffer, 0, raster->glyphBufferSize);

	// Allocate atlas parameters uniform buffer
	raster->atlasParamsSize = sizeof(NVGGpuAtlasParams);
	VkBufferCreateInfo atlasBufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = raster->atlasParamsSize,
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateBuffer(vk->device, &atlasBufferInfo, NULL, &raster->atlasParamsBuffer) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to create atlas params buffer\n");
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	VkMemoryRequirements atlasMemReqs;
	vkGetBufferMemoryRequirements(vk->device, raster->atlasParamsBuffer, &atlasMemReqs);

	uint32_t atlasMemoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((atlasMemReqs.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags) {
			atlasMemoryTypeIndex = i;
			break;
		}
	}

	if (atlasMemoryTypeIndex == UINT32_MAX) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to find memory type for atlas params\n");
		vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	VkMemoryAllocateInfo atlasAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = atlasMemReqs.size,
		.memoryTypeIndex = atlasMemoryTypeIndex
	};

	if (vkAllocateMemory(vk->device, &atlasAllocInfo, NULL, &raster->atlasParamsMemory) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to allocate atlas params memory\n");
		vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	vkBindBufferMemory(vk->device, raster->atlasParamsBuffer, raster->atlasParamsMemory, 0);

	if (vkMapMemory(vk->device, raster->atlasParamsMemory, 0, raster->atlasParamsSize, 0,
	                &raster->atlasParamsMapped) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to map atlas params memory\n");
		vkFreeMemory(vk->device, raster->atlasParamsMemory, NULL);
		vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	// Bind atlas params buffer to descriptor set
	nvgvk_update_compute_descriptors(vk, &raster->pipeline, 2,
	                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	                                  raster->atlasParamsBuffer, 0, raster->atlasParamsSize);

	// Allocate command buffer for compute dispatches
	VkCommandBufferAllocateInfo cmdAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vk->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	if (vkAllocateCommandBuffers(vk->device, &cmdAllocInfo, &raster->computeCmd) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to allocate compute command buffer\n");
		vkUnmapMemory(vk->device, raster->atlasParamsMemory);
		vkFreeMemory(vk->device, raster->atlasParamsMemory, NULL);
		vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	// Create fence for synchronization
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0
	};

	if (vkCreateFence(vk->device, &fenceInfo, NULL, &raster->computeFence) != VK_SUCCESS) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to create compute fence\n");
		vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &raster->computeCmd);
		vkUnmapMemory(vk->device, raster->atlasParamsMemory);
		vkFreeMemory(vk->device, raster->atlasParamsMemory, NULL);
		vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		vkUnmapMemory(vk->device, raster->glyphMemory);
		vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
		free(raster);
		return 0;
	}

	raster->valid = 1;
	raster->jobCount = 0;
	fs->gpuRasterizer = raster;

	printf("[nvg_gpu_raster] GPU rasterizer initialized (pxRange=%.1f, winding=%d)\n",
	       raster->params.pxRange, raster->params.useWinding);

	return 1;
}

void nvgFont_DestroyGpuRasterizer(NVGFontSystem* fs) {
	if (!fs || !fs->gpuRasterizer) return;

	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	NVGVkContext* vk = (NVGVkContext*)raster->vkContext;

	if (raster->valid) {
		// Destroy fence
		if (raster->computeFence != VK_NULL_HANDLE) {
			vkDestroyFence(vk->device, raster->computeFence, NULL);
		}

		// Free command buffer
		if (raster->computeCmd != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &raster->computeCmd);
		}

		// Unmap atlas params memory
		if (raster->atlasParamsMapped) {
			vkUnmapMemory(vk->device, raster->atlasParamsMemory);
		}

		// Free atlas params buffer and memory
		if (raster->atlasParamsBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(vk->device, raster->atlasParamsBuffer, NULL);
		}
		if (raster->atlasParamsMemory != VK_NULL_HANDLE) {
			vkFreeMemory(vk->device, raster->atlasParamsMemory, NULL);
		}

		// Unmap glyph memory
		if (raster->glyphMapped) {
			vkUnmapMemory(vk->device, raster->glyphMemory);
		}

		// Free glyph buffer and memory
		if (raster->glyphBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(vk->device, raster->glyphBuffer, NULL);
		}
		if (raster->glyphMemory != VK_NULL_HANDLE) {
			vkFreeMemory(vk->device, raster->glyphMemory, NULL);
		}

		// Destroy compute pipeline
		nvgvk_destroy_compute_pipeline(vk, &raster->pipeline);
	}

	free(raster);
	fs->gpuRasterizer = NULL;

	printf("[nvg_gpu_raster] GPU rasterizer destroyed\n");
}

int nvgFont_RasterizeGlyphGPU(NVGFontSystem* fs, FT_Face face,
                               unsigned int glyphIndex,
                               int width, int height,
                               int atlasX, int atlasY,
                               int atlasIndex) {
	printf("[GPU Rasterize] Queuing glyph %u size %dx%d (char '%c')\n",
	       glyphIndex, width, height,
	       (glyphIndex >= 32 && glyphIndex < 127) ? (char)glyphIndex : '?');

	if (!fs || !fs->gpuRasterizer || !face) {
		printf("[GPU Rasterize] FAILED - missing fs/rasterizer/face\n");
		return 0;
	}

	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	if (!raster->valid) {
		printf("[GPU Rasterize] FAILED - rasterizer not valid\n");
		return 0;
	}

	// Check if queue is full
	if (raster->jobCount >= MAX_GPU_RASTER_JOBS) {
		fprintf(stderr, "[GPU Rasterize] Job queue full (%d jobs), cannot queue glyph %u\n",
		        raster->jobCount, glyphIndex);
		return 0;
	}

	// Extract outline to GPU format
	NVGGpuGlyphData glyphData;
	if (!nvgFont_ExtractOutline(face, glyphIndex, &glyphData, width, height)) {
		return 0;
	}

	// Queue the job for later execution
	NVGGpuRasterJob* job = &raster->jobs[raster->jobCount];
	job->glyphData = glyphData;
	job->glyphIndex = glyphIndex;
	job->width = width;
	job->height = height;
	job->atlasX = atlasX;
	job->atlasY = atlasY;
	job->atlasIndex = atlasIndex;
	raster->jobCount++;

	printf("[nvg_gpu_raster] Queued glyph %u (%dx%d at %d,%d) - %d jobs total\n",
	       glyphIndex, width, height, atlasX, atlasY, raster->jobCount);

	return 1;
}

int nvgFont_FlushGpuRasterJobs(NVGFontSystem* fs, void* cmdBuffer) {
	if (!fs || !fs->gpuRasterizer) return 0;

	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	if (!raster->valid || raster->jobCount == 0) {
		return 0;
	}

	VkCommandBuffer cmd = (VkCommandBuffer)cmdBuffer;
	NVGVkContext* vk = (NVGVkContext*)raster->vkContext;

	printf("[nvg_gpu_raster] Flushing %d GPU rasterization jobs\n", raster->jobCount);

	// We'll transition the atlas image once at the start and once at the end
	// First, get the atlas texture from the first job (assume all jobs use same atlas for now)
	int atlasIndex = raster->jobs[0].atlasIndex;

	if (atlasIndex < 0 || atlasIndex >= vk->textureCount) {
		fprintf(stderr, "[nvg_gpu_raster] Invalid atlas index %d (textureCount=%d)\n",
		        atlasIndex, vk->textureCount);
		return 0;
	}

	NVGVkTexture* atlasTex = &vk->textures[atlasIndex];
	if (atlasTex->image == VK_NULL_HANDLE || atlasTex->imageView == VK_NULL_HANDLE) {
		fprintf(stderr, "[nvg_gpu_raster] Atlas texture %d not initialized\n", atlasIndex);
		return 0;
	}

	// Bind atlas image to descriptor set (binding 1, storage image)
	if (!nvgvk_update_compute_image_descriptor(vk, &raster->pipeline, 1,
	                                            atlasTex->imageView,
	                                            VK_IMAGE_LAYOUT_GENERAL)) {
		fprintf(stderr, "[nvg_gpu_raster] Failed to bind atlas image\n");
		return 0;
	}

	// Transition atlas image to GENERAL layout for compute writes
	VkImageMemoryBarrier preBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = atlasTex->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vkCmdPipelineBarrier(cmd,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &preBarrier);

	// Bind compute pipeline once
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, raster->pipeline.pipeline);

	// Bind descriptor set once
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
	                        raster->pipeline.layout, 0, 1,
	                        &raster->pipeline.descriptorSet, 0, NULL);

	// Process all jobs
	for (int i = 0; i < raster->jobCount; i++) {
		NVGGpuRasterJob* job = &raster->jobs[i];

		// Upload glyph data to staging buffer
		memcpy(raster->glyphMapped, &job->glyphData, sizeof(NVGGpuGlyphData));

		// Update atlas parameters
		NVGGpuAtlasParams atlasParams;
		atlasParams.offset[0] = job->atlasX;
		atlasParams.offset[1] = job->atlasY;
		atlasParams.glyphSize[0] = job->width;
		atlasParams.glyphSize[1] = job->height;
		memcpy(raster->atlasParamsMapped, &atlasParams, sizeof(NVGGpuAtlasParams));

		// Calculate workgroup counts (8x8 local size in shader)
		uint32_t groupCountX = (job->width + 7) / 8;
		uint32_t groupCountY = (job->height + 7) / 8;

		// Set push constants
		NVGGpuRasterPushConstants pushConstants;
		pushConstants.curveCount = job->glyphData.curveCount;
		pushConstants.contourCount = job->glyphData.contourCount;
		pushConstants.pxRange = raster->params.pxRange;
		pushConstants.useWinding = raster->params.useWinding;

		vkCmdPushConstants(cmd, raster->pipeline.layout,
		                   VK_SHADER_STAGE_COMPUTE_BIT, 0,
		                   sizeof(NVGGpuRasterPushConstants), &pushConstants);

		// Dispatch compute shader for this glyph
		vkCmdDispatch(cmd, groupCountX, groupCountY, 1);

		printf("[nvg_gpu_raster] Dispatched job %d: glyph %u (%dx%d at %d,%d)\n",
		       i, job->glyphIndex, job->width, job->height, job->atlasX, job->atlasY);

		// Add a memory barrier between dispatches to ensure:
		// 1. This dispatch completes writing to the atlas before the next one starts
		// 2. Buffers are synchronized between host writes and GPU reads
		if (i < raster->jobCount - 1) {
			VkMemoryBarrier memBarrier = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
			};
			vkCmdPipelineBarrier(cmd,
			                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                     0, 1, &memBarrier, 0, NULL, 0, NULL);
		}
	}

	// Transition atlas image back to shader read layout
	VkImageMemoryBarrier postBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = atlasTex->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vkCmdPipelineBarrier(cmd,
	                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &postBarrier);

	int flushedCount = raster->jobCount;
	raster->jobCount = 0;  // Clear the queue

	printf("[nvg_gpu_raster] Flushed %d GPU rasterization jobs\n", flushedCount);

	return flushedCount;
}

void nvgFont_SetGpuRasterizerMode(NVGFontSystem* fs, NVGRasterMode mode) {
	if (!fs || !fs->gpuRasterizer) return;
	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	raster->mode = mode;
}
