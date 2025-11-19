//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
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

#include "nvg_vk.h"
#include "nvg_vk_adapter.h"
#include "impl/nvg_vk_context.h"
#include "impl/nvg_vk_texture.h"
#include "impl/nvg_vk_buffer.h"
#include "impl/nvg_vk_pipeline.h"
#include "impl/nvg_vk_render.h"
#include "impl/nvg_vk_types.h"
#include "../../nanovg/font/nvg_font.h"
#include "impl/nvg_vk_color_space_ubo.h"
#include "impl/nvg_vk_color_space.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Vulkan backend context
typedef struct NVGVkBackend {
	NVGVkContext vk;
	VkRenderPass renderPass;
	VkFramebuffer currentFramebuffer;
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;
	int pipelinesCreated;
	int colorSpaceUBOCreated;
} NVGVkBackend;

// Forward declarations of callback functions
static int nvgvk__renderCreate(void* uptr);
static int nvgvk__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
static int nvgvk__renderDeleteTexture(void* uptr, int image);
static int nvgvk__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
static int nvgvk__renderGetTextureSize(void* uptr, int image, int* w, int* h);
static int nvgvk__renderCopyTexture(void* uptr, int srcImage, int dstImage, int srcX, int srcY, int dstX, int dstY, int w, int h);
static void nvgvk__renderViewport(void* uptr, float width, float height, float devicePixelRatio);
static void nvgvk__renderCancel(void* uptr);
static void nvgvk__renderFlush(void* uptr);
static void nvgvk__renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths);
static void nvgvk__renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths);
static void nvgvk__renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe);
static void nvgvk__renderDelete(void* uptr);
static void nvgvk__renderFontSystemCreated(void* uptr, void* fontSystem);

NVGcontext* nvgCreateVk(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkQueue queue, VkCommandPool commandPool,
                        VkRenderPass renderPass, int flags)
{
	NVGparams params;
	NVGcontext* ctx = NULL;
	NVGVkBackend* backend = NULL;

	backend = (NVGVkBackend*)malloc(sizeof(NVGVkBackend));
	if (backend == NULL) {
		goto error;
	}
	memset(backend, 0, sizeof(NVGVkBackend));

	backend->renderPass = renderPass;

	// Create Vulkan context FIRST before NanoVG internal init
	NVGVkCreateInfo createInfo = {0};
	createInfo.device = device;
	createInfo.physicalDevice = physicalDevice;
	createInfo.queue = queue;
	createInfo.commandPool = commandPool;
	createInfo.flags = flags;

	if (!nvgvk_create(&backend->vk, &createInfo)) {
		goto error;
	}

	// Now create NanoVG context (may call renderCreateTexture for font atlas)
	memset(&params, 0, sizeof(params));
	params.renderCreate = nvgvk__renderCreate;
	params.renderCreateTexture = nvgvk__renderCreateTexture;
	params.renderDeleteTexture = nvgvk__renderDeleteTexture;
	params.renderUpdateTexture = nvgvk__renderUpdateTexture;
	params.renderGetTextureSize = nvgvk__renderGetTextureSize;
	params.renderCopyTexture = nvgvk__renderCopyTexture;
	params.renderViewport = nvgvk__renderViewport;
	params.renderCancel = nvgvk__renderCancel;
	params.renderFlush = nvgvk__renderFlush;
	params.renderFill = nvgvk__renderFill;
	params.renderStroke = nvgvk__renderStroke;
	params.renderTriangles = nvgvk__renderTriangles;
	params.renderDelete = nvgvk__renderDelete;
	params.renderFontSystemCreated = nvgvk__renderFontSystemCreated;
	params.userPtr = backend;
	params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;
	params.msdfText = flags & (1 << 13) ? 1 : 0;  // NVG_MSDF_TEXT flag
	// Pass swapchain color space for text rendering (will be set after color space initialization)
	params.colorSpace = 0;  // Will be updated in renderCreate callback

	ctx = nvgCreateInternal(&params);
	if (ctx == NULL) {
		// nvgCreateInternal failed and already called renderDelete, which freed backend
		return NULL;
	}

	return ctx;

error:
	// Only reached if we fail before calling nvgCreateInternal
	if (backend != NULL) {
		if (backend->vk.device != VK_NULL_HANDLE) {
			nvgvk_delete(&backend->vk);
		}
		free(backend);
	}
	return NULL;
}

void nvgDeleteVk(NVGcontext* ctx)
{
	nvgDeleteInternal(ctx);
}

void nvgVkSetShaderPath(NVGcontext* ctx, const char* path)
{
	if (!ctx) {
		return;
	}

	NVGparams* params = nvgInternalParams(ctx);
	NVGVkBackend* backend = (NVGVkBackend*)params->userPtr;
	NVGVkContext* vk = &backend->vk;

	// Free existing path if any
	if (vk->shaderBasePath) {
		free(vk->shaderBasePath);
		vk->shaderBasePath = NULL;
	}

	// Copy new path if provided
	if (path) {
		size_t len = strlen(path);
		vk->shaderBasePath = (char*)malloc(len + 1);
		if (vk->shaderBasePath) {
			memcpy(vk->shaderBasePath, path, len + 1);
		}
	}
}

VkCommandBuffer nvgVkGetCommandBuffer(NVGcontext* ctx)
{
	NVGparams* params = nvgInternalParams(ctx);
	NVGVkBackend* backend = (NVGVkBackend*)params->userPtr;
	return backend->vk.commandBuffer;
}

void nvgVkSetFramebuffer(NVGcontext* ctx, VkFramebuffer framebuffer, uint32_t width, uint32_t height)
{
	NVGparams* params = nvgInternalParams(ctx);
	NVGVkBackend* backend = (NVGVkBackend*)params->userPtr;
	backend->currentFramebuffer = framebuffer;
	backend->framebufferWidth = width;
	backend->framebufferHeight = height;
}

// Callback implementations

static int nvgvk__renderCreate(void* uptr)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	// Context already created in nvgCreateVk
	return 1;
}

static int nvgvk__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	return nvgvk_create_texture(&backend->vk, type, w, h, imageFlags, data);
}

static int nvgvk__renderDeleteTexture(void* uptr, int image)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	nvgvk_delete_texture(&backend->vk, image);
	return 1;
}

static int nvgvk__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	return nvgvk_update_texture(&backend->vk, image, x, y, w, h, data);
}

static int nvgvk__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	return nvgvk_get_texture_size(&backend->vk, image, w, h);
}

static int nvgvk__renderCopyTexture(void* uptr, int srcImage, int dstImage, int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	return nvgvk_copy_texture(&backend->vk, srcImage, dstImage, srcX, srcY, dstX, dstY, w, h);
}

static void nvgvk__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	nvgvk_viewport(&backend->vk, width, height, devicePixelRatio);
}

static void nvgvk__renderCancel(void* uptr)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	nvgvk_cancel(&backend->vk);
}

static void nvgvk__renderFlush(void* uptr)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;

	// Create pipelines if not created yet
	if (!backend->pipelinesCreated) {
		if (!nvgvk_create_pipelines(&backend->vk, backend->renderPass)) {
			fprintf(stderr, "NanoVG Vulkan: Failed to create pipelines\n");
			return;
		}
		backend->pipelinesCreated = 1;
	}

	// Create color space UBO after pipelines (reuses descriptor pool)
	if (!backend->colorSpaceUBOCreated && backend->pipelinesCreated) {
		if (!nvgvk_init_color_space_ubo(&backend->vk)) {
			fprintf(stderr, "NanoVG Vulkan: Failed to initialize color space UBO\n");
			// Not fatal - rendering will work but without color space conversion
		} else {
			backend->colorSpaceUBOCreated = 1;
		}
	}

	nvgvk_flush(&backend->vk);
}

// Helper: Convert NVGpaint to internal uniforms
static void nvgvk__convertPaint(NVGVkBackend* backend, NVGVkUniforms* frag, NVGpaint* paint,
                                NVGscissor* scissor, float width, float strokeThr)
{
	memset(frag, 0, sizeof(*frag));

	// Paint matrix
	float invxform[6];
	nvgTransformInverse(invxform, paint->xform);

	// Convert 2x3 matrix to 3x4 layout
	frag->paintMat[0] = invxform[0];
	frag->paintMat[1] = invxform[1];
	frag->paintMat[2] = 0.0f;
	frag->paintMat[3] = 0.0f;
	frag->paintMat[4] = invxform[2];
	frag->paintMat[5] = invxform[3];
	frag->paintMat[6] = 0.0f;
	frag->paintMat[7] = 0.0f;
	frag->paintMat[8] = invxform[4];
	frag->paintMat[9] = invxform[5];
	frag->paintMat[10] = 1.0f;
	frag->paintMat[11] = 0.0f;

	// Scissor matrix
	if (scissor) {
		float invscissor[6];
		nvgTransformInverse(invscissor, scissor->xform);

		frag->scissorMat[0] = invscissor[0];
		frag->scissorMat[1] = invscissor[1];
		frag->scissorMat[2] = 0.0f;
		frag->scissorMat[3] = 0.0f;
		frag->scissorMat[4] = invscissor[2];
		frag->scissorMat[5] = invscissor[3];
		frag->scissorMat[6] = 0.0f;
		frag->scissorMat[7] = 0.0f;
		frag->scissorMat[8] = invscissor[4];
		frag->scissorMat[9] = invscissor[5];
		frag->scissorMat[10] = 1.0f;
		frag->scissorMat[11] = 0.0f;

		frag->scissorExt[0] = scissor->extent[0];
		frag->scissorExt[1] = scissor->extent[1];
		frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]);
		frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]);
	} else {
		// No scissor - use identity that covers everything
		frag->scissorMat[0] = 1.0f;
		frag->scissorMat[5] = 1.0f;
		frag->scissorMat[10] = 1.0f;
		frag->scissorExt[0] = 1e6f;
		frag->scissorExt[1] = 1e6f;
		frag->scissorScale[0] = 1.0f;
		frag->scissorScale[1] = 1.0f;
	}

	// Colors
	frag->innerCol[0] = paint->innerColor.r;
	frag->innerCol[1] = paint->innerColor.g;
	frag->innerCol[2] = paint->innerColor.b;
	frag->innerCol[3] = paint->innerColor.a;

	frag->outerCol[0] = paint->outerColor.r;
	frag->outerCol[1] = paint->outerColor.g;
	frag->outerCol[2] = paint->outerColor.b;
	frag->outerCol[3] = paint->outerColor.a;

	// Paint parameters
	frag->extent[0] = paint->extent[0];
	frag->extent[1] = paint->extent[1];
	frag->radius = paint->radius;
	frag->feather = paint->feather;

	// Stroke parameters
	frag->strokeMult = (width*0.5f + 0.5f) * 0.5f;
	frag->strokeThr = strokeThr;

	// Image or gradient
	if (paint->image > 0) {
		frag->type = 1; // Image
		// Set texType based on actual texture type
		// OpenGL encoding: 0=RGBA premult, 1=RGBA non-premult, 2=ALPHA
		int texId = paint->image - 1;
		if (texId >= 0 && texId < NVGVK_MAX_TEXTURES) {
			NVGVkTexture* tex = &backend->vk.textures[texId];
			printf("[nvg_vk] texId=%d, tex->type=%d (RGBA=%d, ALPHA=%d, MSDF=%d)\n",
				texId, tex->type, NVG_TEXTURE_RGBA, NVG_TEXTURE_ALPHA, NVG_TEXTURE_MSDF);
			if (tex->type == NVG_TEXTURE_RGBA) {
				// RGBA texture: check premultiply flag
				frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1;
			} else {
				// ALPHA texture
				frag->texType = 2;
			}
			printf("[nvg_vk] Setting frag->texType=%d for texId=%d\n", frag->texType, texId);
		} else {
			frag->texType = 0;
		}
	} else {
		frag->type = 0; // Gradient
		frag->texType = 0;
	}
}

static void nvgvk__renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                              NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	NVGVkContext* vk = &backend->vk;

	// Convert paths to internal format
	for (int i = 0; i < npaths; i++) {
		if (vk->pathCount >= NVGVK_MAX_CALLS) break;

		NVGVkPath* dstPath = &vk->paths[vk->pathCount++];
		const NVGpath* srcPath = &paths[i];

		dstPath->fillOffset = vk->vertexCount;
		dstPath->fillCount = srcPath->nfill;

		// Copy fill vertices
		for (int j = 0; j < srcPath->nfill && vk->vertexCount < vk->vertexCapacity; j++) {
			NVGvertex* dstVert = (NVGvertex*)&vk->vertices[vk->vertexCount * 4];
			*dstVert = srcPath->fill[j];
			vk->vertexCount++;
		}

		// Copy fringe vertices (for AA)
		dstPath->strokeOffset = vk->vertexCount;
		dstPath->strokeCount = srcPath->nstroke;
		for (int j = 0; j < srcPath->nstroke && vk->vertexCount < vk->vertexCapacity; j++) {
			NVGvertex* dstVert = (NVGvertex*)&vk->vertices[vk->vertexCount * 4];
			*dstVert = srcPath->stroke[j];
			vk->vertexCount++;
		}
	}

	// Create bounding quad for cover pass (bounds = [minx, miny, maxx, maxy])
	int quadOffset = vk->vertexCount;
	if (vk->vertexCount + 6 <= vk->vertexCapacity && bounds) {
		float minx = bounds[0], miny = bounds[1];
		float maxx = bounds[2], maxy = bounds[3];

		// Triangle 1: top-left, top-right, bottom-left
		NVGvertex* v0 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v0->x = minx; v0->y = miny; v0->u = 0.5f; v0->v = 1.0f;

		NVGvertex* v1 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v1->x = maxx; v1->y = miny; v1->u = 0.5f; v1->v = 1.0f;

		NVGvertex* v2 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v2->x = minx; v2->y = maxy; v2->u = 0.5f; v2->v = 1.0f;

		// Triangle 2: top-right, bottom-right, bottom-left
		NVGvertex* v3 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v3->x = maxx; v3->y = miny; v3->u = 0.5f; v3->v = 1.0f;

		NVGvertex* v4 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v4->x = maxx; v4->y = maxy; v4->u = 0.5f; v4->v = 1.0f;

		NVGvertex* v5 = (NVGvertex*)&vk->vertices[vk->vertexCount++ * 4];
		v5->x = minx; v5->y = maxy; v5->u = 0.5f; v5->v = 1.0f;
	}

	// Add render call
	if (vk->callCount >= NVGVK_MAX_CALLS) return;
	NVGVkCall* call = &vk->calls[vk->callCount++];

	call->type = NVGVK_FILL;
	call->image = paint->image;
	call->pathOffset = vk->pathCount - npaths;
	call->pathCount = npaths;
	call->triangleOffset = quadOffset;
	call->triangleCount = 6;  // Two triangles for bounding quad
	call->uniformOffset = vk->uniformCount;
	call->blendFunc = compositeOperation.srcRGB; // Simplified

	// Setup uniforms
	if (vk->uniformCount < NVGVK_MAX_CALLS) {
		nvgvk__convertPaint(backend, &vk->uniforms[vk->uniformCount++], paint, scissor, fringe, 0.0f);
	}
}

static void nvgvk__renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                                NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	NVGVkContext* vk = &backend->vk;

	// Convert paths to internal format
	for (int i = 0; i < npaths; i++) {
		if (vk->pathCount >= NVGVK_MAX_CALLS) break;

		NVGVkPath* dstPath = &vk->paths[vk->pathCount++];
		const NVGpath* srcPath = &paths[i];

		dstPath->strokeOffset = vk->vertexCount;
		dstPath->strokeCount = srcPath->nstroke;

		// Copy stroke vertices
		for (int j = 0; j < srcPath->nstroke && vk->vertexCount < vk->vertexCapacity; j++) {
			NVGvertex* dstVert = (NVGvertex*)&vk->vertices[vk->vertexCount * 4];
			*dstVert = srcPath->stroke[j];
			vk->vertexCount++;
		}

		dstPath->fillOffset = 0;
		dstPath->fillCount = 0;
	}

	// Add render call
	if (vk->callCount >= NVGVK_MAX_CALLS) return;
	NVGVkCall* call = &vk->calls[vk->callCount++];

	call->type = NVGVK_STROKE;
	call->image = paint->image;
	call->pathOffset = vk->pathCount - npaths;
	call->pathCount = npaths;
	call->triangleOffset = 0;
	call->triangleCount = 0;
	call->uniformOffset = vk->uniformCount;
	call->blendFunc = compositeOperation.srcRGB;

	// Setup uniforms
	if (vk->uniformCount < NVGVK_MAX_CALLS) {
		nvgvk__convertPaint(backend, &vk->uniforms[vk->uniformCount++], paint, scissor, strokeWidth, -1.0f);
	}
}

static void nvgvk__renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
                                   NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	NVGVkContext* vk = &backend->vk;

	// Copy triangle vertices
	int triangleOffset = vk->vertexCount;
	for (int i = 0; i < nverts && vk->vertexCount < vk->vertexCapacity; i++) {
		NVGvertex* dstVert = (NVGvertex*)&vk->vertices[vk->vertexCount * 4];
		*dstVert = verts[i];
		vk->vertexCount++;
	}

	// Add render call
	if (vk->callCount >= NVGVK_MAX_CALLS) return;
	NVGVkCall* call = &vk->calls[vk->callCount++];

	call->type = NVGVK_TRIANGLES;
	call->image = paint->image;
	call->pathOffset = 0;
	call->pathCount = 0;
	call->triangleOffset = triangleOffset;
	call->triangleCount = nverts;
	call->uniformOffset = vk->uniformCount;
	call->blendFunc = compositeOperation.srcRGB;

	// Setup uniforms
	if (vk->uniformCount < NVGVK_MAX_CALLS) {
		nvgvk__convertPaint(backend, &vk->uniforms[vk->uniformCount++], paint, scissor, fringe, -1.0f);
	}
}

static void nvgvk__renderDelete(void* uptr)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;

	if (backend->pipelinesCreated) {
		nvgvk_destroy_pipelines(&backend->vk);
	}

	nvgvk_delete(&backend->vk);
	free(backend);
}

void nvgVkBeginRenderPass(NVGcontext* ctx, const VkRenderPassBeginInfo* renderPassInfo,
                          VkViewport viewport, VkRect2D scissor)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend || !renderPassInfo) return;

	nvgvk_begin_render_pass(&backend->vk,
	                        renderPassInfo->renderPass,
	                        renderPassInfo->framebuffer,
	                        renderPassInfo->renderArea,
	                        renderPassInfo->pClearValues,
	                        renderPassInfo->clearValueCount,
	                        viewport,
	                        scissor);
}

void nvgVkEndRenderPass(NVGcontext* ctx)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend) return;

	nvgvk_end_render_pass(&backend->vk);
}


void nvgVkSetHDRScale(NVGcontext* ctx, float scale)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend) return;

	// If color space not set up yet, the settings will take effect when it is
	if (backend->vk.colorSpace) {
		nvgvk_set_hdr_scale((NVGVkColorSpace*)backend->vk.colorSpace, scale);
		backend->vk.colorSpaceChanged = 1;
		nvgvk_update_color_space_ubo(&backend->vk);
	}
}

void nvgVkSetGamutMapping(NVGcontext* ctx, int enabled)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend) return;

	// If color space not set up yet, the settings will take effect when it is
	if (backend->vk.colorSpace) {
		nvgvk_set_gamut_mapping((NVGVkColorSpace*)backend->vk.colorSpace, enabled);
		backend->vk.colorSpaceChanged = 1;
		nvgvk_update_color_space_ubo(&backend->vk);
	}
}

void nvgVkSetToneMapping(NVGcontext* ctx, int enabled)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend) return;

	// If color space not set up yet, the settings will take effect when it is
	if (backend->vk.colorSpace) {
		nvgvk_set_tone_mapping((NVGVkColorSpace*)backend->vk.colorSpace, enabled);
		backend->vk.colorSpaceChanged = 1;
		nvgvk_update_color_space_ubo(&backend->vk);
	}
}

static void nvgvk__renderFontSystemCreated(void* uptr, void* fontSystem)
{
	NVGVkBackend* backend = (NVGVkBackend*)uptr;
	if (!backend || !fontSystem) return;

	// Set target color space for font rendering if available
	if (backend->vk.colorSpace) {
		NVGVkColorSpace* cs = (NVGVkColorSpace*)backend->vk.colorSpace;
		nvgFontSetColorSpace((NVGFontSystem*)fontSystem, nvgvk_from_vk_color_space(cs->swapchainColorSpace));
	}
}

void nvgVkDumpAtlasTextureByIndex(NVGcontext* ctx, int textureIndex, const char* filename)
{
	NVGVkBackend* backend = (NVGVkBackend*)nvgInternalParams(ctx)->userPtr;
	if (!backend || !filename) return;

	NVGVkContext* vk = &backend->vk;

	// Check texture index
	if (textureIndex < 0 || textureIndex >= vk->textureCount) {
		printf("[Atlas Dump] Invalid texture index %d (have %d textures)\n", textureIndex, vk->textureCount);
		return;
	}

	NVGVkTexture* tex = &vk->textures[textureIndex];
	if (tex->image == VK_NULL_HANDLE) {
		printf("[Atlas Dump] Atlas texture not initialized\n");
		return;
	}

	int width = tex->width;
	int height = tex->height;
	int isRGBA = (tex->type == NVG_TEXTURE_RGBA);

	// Create staging buffer to read back texture
	VkDeviceSize imageSize = width * height * (isRGBA ? 4 : 1);  // RGBA or ALPHA
	NVGVkBuffer stagingBuffer;
	if (!nvgvk_buffer_create(vk, &stagingBuffer, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
		printf("[Atlas Dump] Failed to create staging buffer\n");
		return;
	}

	// Create command buffer for copy
	VkCommandBuffer cmd;
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vk->device, &allocInfo, &cmd) != VK_SUCCESS) {
		nvgvk_buffer_destroy(vk, &stagingBuffer);
		return;
	}

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	// Transition image to TRANSFER_SRC
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = tex->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy image to buffer
	VkBufferImageCopy region = {0};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyImageToBuffer(cmd, tex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		stagingBuffer.buffer, 1, &region);

	// Transition back to SHADER_READ
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(cmd);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->queue);

	// Write PPM file
	FILE* f = fopen(filename, "wb");
	if (f) {
		fprintf(f, "P6\n%d %d\n255\n", width, height);
		unsigned char* data = (unsigned char*)stagingBuffer.mapped;
		if (isRGBA) {
			// RGBA texture - write RGB, ignore A
			for (int i = 0; i < width * height; i++) {
				fputc(data[i*4 + 0], f);  // R
				fputc(data[i*4 + 1], f);  // G
				fputc(data[i*4 + 2], f);  // B
			}
		} else {
			// ALPHA texture - write as grayscale
			for (int i = 0; i < width * height; i++) {
				unsigned char gray = data[i];
				fputc(gray, f);  // R
				fputc(gray, f);  // G
				fputc(gray, f);  // B
			}
		}
		fclose(f);
		printf("[Atlas Dump] Wrote %dx%d %s atlas to %s\n", width, height, isRGBA ? "RGBA" : "ALPHA", filename);
	}

	// Cleanup
	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &cmd);
	nvgvk_buffer_destroy(vk, &stagingBuffer);
}

void nvgVkDumpAtlasTexture(NVGcontext* ctx, const char* filename)
{
	nvgVkDumpAtlasTextureByIndex(ctx, 0, filename);
}

void nvgVkDumpAtlasByFormat(NVGcontext* ctx, VkFormat format, const char* filename)
{
	if (!ctx || !filename) return;

	// Get the atlas texture ID for this format using the public API
	int textureId = nvgGetAtlasTextureId(ctx, (int)format);
	if (textureId == 0) {
		printf("[nvgVkDumpAtlasByFormat] No atlas found for format %d\n", format);
		return;
	}

	// Convert from 1-based to 0-based index
	nvgVkDumpAtlasTextureByIndex(ctx, textureId - 1, filename);
}
