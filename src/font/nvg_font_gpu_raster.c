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

// Internal GPU rasterizer state
struct NVGFontGpuRasterizer {
	void* vkContext;              // NVGVkContext* (opaque)
	NVGVkComputePipeline pipeline;
	NVGGpuRasterParams params;

	// Staging buffer for glyph data
	VkBuffer glyphBuffer;
	VkDeviceMemory glyphMemory;
	void* glyphMapped;
	VkDeviceSize glyphBufferSize;

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
		NVGGpuContour* contour = &ctx->glyphData->contours[ctx->currentContour];
		contour->curveCount = ctx->glyphData->curveCount - contour->firstCurve;

		// Determine winding (FreeType uses postive = counter-clockwise)
		contour->winding = 1;  // Default CCW
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
		NVGGpuContour* contour = &glyphData->contours[ctx.currentContour];
		contour->curveCount = glyphData->curveCount - contour->firstCurve;
	}

	return 1;
}

int nvgFont_ShouldUseGPU(NVGFontSystem* fs, FT_Face face,
                         unsigned int glyphIndex, int width, int height) {
	if (!fs || !fs->gpuRasterizer || !face) return 0;

	// Don't use GPU for very small glyphs (CPU overhead dominates)
	if (width < 16 || height < 16) return 0;

	// Don't use GPU for very large glyphs (buffer size limits)
	if (width > 256 || height > 256) return 0;

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

	// Too simple - CPU is efficient enough
	if (nContours < 2 && nPoints < 12) return 0;

	return 1;
}

int nvgFont_InitGpuRasterizer(NVGFontSystem* fs, void* vkContext,
                               const NVGGpuRasterParams* params) {
	if (!fs || !vkContext) return 0;

	// Allocate rasterizer state
	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)malloc(sizeof(NVGFontGpuRasterizer));
	if (!raster) return 0;

	memset(raster, 0, sizeof(NVGFontGpuRasterizer));
	raster->vkContext = vkContext;

	// Set parameters
	if (params) {
		raster->params = *params;
	} else {
		// Defaults
		raster->params.pxRange = 1.5f;
		raster->params.useWinding = 1;
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
	                                    (uint32_t*)glyph_raster_comp_spv,
	                                    glyph_raster_comp_spv_len,
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

	raster->valid = 1;
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
		// Unmap memory
		if (raster->glyphMapped) {
			vkUnmapMemory(vk->device, raster->glyphMemory);
		}

		// Free buffer and memory
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
	(void)atlasX;  // TODO: Will be used when compute shader is implemented
	(void)atlasY;
	(void)atlasIndex;

	if (!fs || !fs->gpuRasterizer || !face) return 0;

	NVGFontGpuRasterizer* raster = (NVGFontGpuRasterizer*)fs->gpuRasterizer;
	if (!raster->valid) return 0;

	// Extract outline to GPU format
	NVGGpuGlyphData glyphData;
	if (!nvgFont_ExtractOutline(face, glyphIndex, &glyphData, width, height)) {
		return 0;
	}

	printf("[nvg_gpu_raster] Extracted glyph %u: %u curves, %u contours\n",
	       glyphIndex, glyphData.curveCount, glyphData.contourCount);

	// TODO: Upload to GPU buffer, dispatch compute shader, write to atlas
	// This will be implemented when we have the compute shader

	// For now, just return failure to trigger CPU fallback
	return 0;
}
