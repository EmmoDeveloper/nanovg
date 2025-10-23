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
//
// Vulkan rendering backend for NanoVG
// Based on the NanoVG OpenGL backend architecture
//

#ifndef NANOVG_VK_H
#define NANOVG_VK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "nanovg_vk_types.h"

NVGcontext* nvgCreateVk(const NVGVkCreateInfo* createInfo, int flags);
void nvgDeleteVk(NVGcontext* ctx);

VkCommandBuffer nvgVkGetCommandBuffer(NVGcontext* ctx);

void nvgVkSetRenderTargets(NVGcontext* ctx, VkImageView colorImageView, VkImageView depthStencilImageView);

VkSemaphore nvgVkGetImageAvailableSemaphore(NVGcontext* ctx);
VkSemaphore nvgVkGetRenderFinishedSemaphore(NVGcontext* ctx);
VkFence nvgVkGetFrameFence(NVGcontext* ctx);

int nvgVkCreateImageFromHandle(NVGcontext* ctx, VkImage image, VkImageView imageView,
                                int w, int h, int flags);
VkImageView nvgVkImageHandle(NVGcontext* ctx, int image);

const VKNVGprofiling* nvgVkGetProfiling(NVGcontext* ctx);
void nvgVkResetProfiling(NVGcontext* ctx);

#ifdef NANOVG_VK_IMPLEMENTATION

#include "../../nanovg/src/nanovg.h"
#include "../../nanovg/src/fontstash.h"

#include "nanovg_vk_internal.h"
#include "nanovg_vk_memory.h"
#include "nanovg_vk_text_instance.h"
#include "nanovg_vk_image.h"
#include "nanovg_vk_platform.h"
#include "nanovg_vk_threading.h"
#include "nanovg_vk_pipeline.h"
// Phase 6: Emoji rendering (must be before render.h which uses these functions)
#include "nanovg_vk_text_emoji.h"
#include "nanovg_vk_render.h"
#include "nanovg_vk_cleanup.h"
#include "nanovg_vk_atlas.h"

// Fontstash glyph rasterization callback - only available when FONTSTASH_IMPLEMENTATION is defined
#ifdef FONTSTASH_IMPLEMENTATION
#ifndef VKNVG_NO_FONTSTASH_CALLBACK

static uint8_t* vknvg__rasterizeGlyph(void* fontContext, VKNVGglyphKey key,
                                       uint16_t* width, uint16_t* height,
                                       int16_t* bearingX, int16_t* bearingY,
                                       uint16_t* advance)
{
	FONScontext* fs = (FONScontext*)fontContext;
	if (!fs) return NULL;

	// Extract size from key (fixed point 16.16 format)
	float size = (float)(key.size >> 16) + (float)(key.size & 0xFFFF) / 65536.0f;
	short isize = (short)(size * 10.0f);  // Fontstash uses 1/10th pixel size
	short iblur = 0;

	// Get glyph from fontstash (this handles fallback fonts automatically)
	FONSglyph* glyph = fons__getGlyph(fs, NULL, key.codepoint, isize, iblur, FONS_GLYPH_BITMAP_REQUIRED);
	if (!glyph || glyph->x0 < 0 || glyph->y0 < 0) {
		return NULL;  // Glyph not found or has no bitmap
	}

	// Extract metrics from fontstash glyph
	*width = (uint16_t)(glyph->x1 - glyph->x0);
	*height = (uint16_t)(glyph->y1 - glyph->y0);
	*bearingX = glyph->xoff;
	*bearingY = glyph->yoff;
	*advance = (uint16_t)glyph->xadv;

	// Allocate pixel data
	size_t dataSize = (*width) * (*height);
	if (dataSize == 0) return NULL;

	uint8_t* pixelData = (uint8_t*)malloc(dataSize);
	if (!pixelData) return NULL;

	// Copy pixel data from fontstash texture
	unsigned char* atlasData = fs->texData;
	int atlasWidth = fs->params.width;

	for (int y = 0; y < *height; y++) {
		for (int x = 0; x < *width; x++) {
			int srcX = glyph->x0 + x;
			int srcY = glyph->y0 + y;
			int srcIdx = srcY * atlasWidth + srcX;
			int dstIdx = y * (*width) + x;
			pixelData[dstIdx] = atlasData[srcIdx];
		}
	}

	return pixelData;
}

#endif // !VKNVG_NO_FONTSTASH_CALLBACK
#endif // FONTSTASH_IMPLEMENTATION

// Virtual atlas initialization - only needs FONS_H header
#ifdef FONS_H
#ifndef VKNVG_NO_FONTSTASH_CALLBACK

// Forward declaration of callback (defined in nanovg_vk_render.h)
struct FONSglyph;  // Forward declare
extern void vknvg__onGlyphAdded(void* uptr, struct FONSglyph* glyph, int fontIndex, const unsigned char* data, int width, int height);

// Initialize virtual atlas with fontstash context (called after NVGcontext creation)
static void vknvg__initVirtualAtlasFont(VKNVGcontext* vk)
{
	if (!vk || !vk->useVirtualAtlas || !vk->virtualAtlas || !vk->fontStashContext) {
		return;
	}

#ifdef FONTSTASH_IMPLEMENTATION
	vknvg__setAtlasFontContext(vk->virtualAtlas, vk->fontStashContext, vknvg__rasterizeGlyph);
#else
	vknvg__setAtlasFontContext(vk->virtualAtlas, vk->fontStashContext, NULL);
#endif

	// Access fontstash params via partial struct (FONScontext internals not exposed in header)
	// This matches the layout of FONScontext from fontstash.h implementation
	struct FONScontextPartial {
		FONSparams params;
		// ... rest of FONScontext fields not needed
	};

	struct FONScontextPartial* fs = (struct FONScontextPartial*)vk->fontStashContext;
	// Set callback for glyph addition (for virtual atlas coordinate patching)
	fs->params.glyphAdded = vknvg__onGlyphAdded;
	fs->params.userPtr = vk;
}

#endif // !VKNVG_NO_FONTSTASH_CALLBACK
#endif // FONS_H

NVGcontext* nvgCreateVk(const NVGVkCreateInfo* createInfo, int flags)
{
	NVGparams params;
	NVGcontext* ctx = NULL;
	VKNVGcontext* vk = (VKNVGcontext*)malloc(sizeof(VKNVGcontext));
	if (vk == NULL) goto error;
	memset(vk, 0, sizeof(VKNVGcontext));

	vk->instance = createInfo->instance;
	vk->physicalDevice = createInfo->physicalDevice;
	vk->device = createInfo->device;
	vk->queue = createInfo->queue;
	vk->queueFamilyIndex = createInfo->queueFamilyIndex;
	vk->renderPass = createInfo->renderPass;
	vk->subpass = createInfo->subpass;
	vk->commandPool = createInfo->commandPool;
	vk->descriptorPool = createInfo->descriptorPool;
	vk->maxFrames = createInfo->maxFrames > 0 ? createInfo->maxFrames : 3;
	vk->flags = flags;

	vk->colorFormat = createInfo->colorFormat;
	vk->depthStencilFormat = createInfo->depthStencilFormat;
	vk->colorImageView = VK_NULL_HANDLE;
	vk->depthStencilImageView = VK_NULL_HANDLE;

	if (flags & NVG_MSAA) {
		vk->sampleCount = createInfo->sampleCount != 0 ? createInfo->sampleCount : VK_SAMPLE_COUNT_4_BIT;
	} else {
		vk->sampleCount = VK_SAMPLE_COUNT_1_BIT;
	}

	if (flags & NVG_MULTI_THREADED) {
		vk->threadCount = createInfo->threadCount > 0 ? createInfo->threadCount : 4;
	} else {
		vk->threadCount = 0;
	}

	// Store emoji font info for later initialization
	vk->emojiFontPath = createInfo->emojiFontPath;
	vk->emojiFontData = createInfo->emojiFontData;
	vk->emojiFontDataSize = createInfo->emojiFontDataSize;

	vk->ownsCommandPool = VK_FALSE;
	vk->ownsDescriptorPool = VK_FALSE;
	vk->ownsRenderPass = VK_FALSE;

	memset(&params, 0, sizeof(params));
	params.renderCreate = vknvg__renderCreate;
	params.renderCreateTexture = vknvg__renderCreateTexture;
	params.renderDeleteTexture = vknvg__renderDeleteTexture;
	params.renderUpdateTexture = vknvg__renderUpdateTexture;
	params.renderGetTextureSize = vknvg__renderGetTextureSize;
	params.renderViewport = vknvg__renderViewport;
	params.renderCancel = vknvg__renderCancel;
	params.renderFlush = vknvg__renderFlush;
	params.renderFill = vknvg__renderFill;
	params.renderStroke = vknvg__renderStroke;
	params.renderTriangles = vknvg__renderTriangles;
	params.renderDelete = vknvg__renderDelete;
	params.userPtr = vk;
	params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;
	params.fontAtlasSize = (flags & NVG_VIRTUAL_ATLAS) ? 4096 : 0;  // Use 4096x4096 atlas for virtual atlas

	ctx = nvgCreateInternal(&params);
	if (ctx == NULL) goto error;

	// Extract FONScontext from NVGcontext for virtual atlas
	// NVGcontext is opaque, but we know its layout from nanovg.c
	// This partial struct definition matches the layout up to the fs field
	// We use a byte array for states since we don't have access to NVGstate definition
	#ifndef NVG_MAX_STATES
	#define NVG_MAX_STATES 32
	#endif
	struct NVGcontextPartial {
		NVGparams params;
		float* commands;
		int ccommands;
		int ncommands;
		float commandx, commandy;
		char states[NVG_MAX_STATES * 272];  // Opaque array (sizeof(NVGstate)=272 per state)
		int nstates;
		struct NVGpathCache* cache;
		float tessTol;
		float distTol;
		float fringeWidth;
		float devicePxRatio;
		struct FONScontext* fs;
	};
	vk->fontStashContext = ((struct NVGcontextPartial*)ctx)->fs;

	// Initialize virtual atlas with fontstash context (only if fontstash callback is available)
#if defined(FONS_H) && !defined(VKNVG_NO_FONTSTASH_CALLBACK)
	vknvg__initVirtualAtlasFont(vk);
#endif

	return ctx;

error:
	if (ctx != NULL) nvgDeleteInternal(ctx);
	return NULL;
}

void nvgDeleteVk(NVGcontext* ctx)
{
	nvgDeleteInternal(ctx);
}

VkCommandBuffer nvgVkGetCommandBuffer(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->currentFrame < vk->maxFrames) {
		return vk->commandBuffers[vk->currentFrame];
	}
	return VK_NULL_HANDLE;
}

void nvgVkSetRenderTargets(NVGcontext* ctx, VkImageView colorImageView, VkImageView depthStencilImageView)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	vk->colorImageView = colorImageView;
	vk->depthStencilImageView = depthStencilImageView;
}

VkSemaphore nvgVkGetImageAvailableSemaphore(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->imageAvailableSemaphores != NULL && vk->currentFrame < vk->maxFrames) {
		return vk->imageAvailableSemaphores[vk->currentFrame];
	}
	return VK_NULL_HANDLE;
}

VkSemaphore nvgVkGetRenderFinishedSemaphore(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->renderFinishedSemaphores != NULL && vk->currentFrame < vk->maxFrames) {
		return vk->renderFinishedSemaphores[vk->currentFrame];
	}
	return VK_NULL_HANDLE;
}

VkFence nvgVkGetFrameFence(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->frameFences != NULL && vk->currentFrame < vk->maxFrames) {
		return vk->frameFences[vk->currentFrame];
	}
	return VK_NULL_HANDLE;
}

const VKNVGprofiling* nvgVkGetProfiling(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->flags & NVG_PROFILING) {
		return &vk->profiling;
	}
	return NULL;
}

void nvgVkResetProfiling(NVGcontext* ctx)
{
	VKNVGcontext* vk = (VKNVGcontext*)nvgInternalParams(ctx)->userPtr;
	if (vk->flags & NVG_PROFILING) {
		vk->profiling.cpuFrameTime = 0.0;
		vk->profiling.cpuGeometryTime = 0.0;
		vk->profiling.cpuRenderTime = 0.0;
		vk->profiling.gpuFrameTime = 0.0;
		vk->profiling.drawCalls = 0;
		vk->profiling.fillCount = 0;
		vk->profiling.strokeCount = 0;
		vk->profiling.triangleCount = 0;
		vk->profiling.vertexCount = 0;
		vk->profiling.textureBinds = 0;
		vk->profiling.pipelineSwitches = 0;
	}
}

#endif // NANOVG_VK_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // NANOVG_VK_H
