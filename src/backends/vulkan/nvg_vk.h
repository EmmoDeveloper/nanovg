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

#ifndef NVG_VK_H
#define NVG_VK_H

#include "../../nanovg/nanovg.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NVGcreateFlags {
	// Flag indicating if geometry based anti-aliasing is used (may not be needed for MSAA).
	NVG_ANTIALIAS = 1<<0,
	// Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
	// slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
	NVG_STENCIL_STROKES	= 1<<1,
	// Flag indicating that additional debug checks are done.
	NVG_DEBUG = 1<<2,
};

// Creates NanoVG context with Vulkan backend.
// Parameters:
//   device - Vulkan logical device
//   physicalDevice - Vulkan physical device
//   queue - Vulkan graphics queue
//   commandPool - Vulkan command pool
//   renderPass - Vulkan render pass (must have depth/stencil attachment)
//   flags - combination of NVGcreateFlags
NVGcontext* nvgCreateVk(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkQueue queue, VkCommandPool commandPool,
                        VkRenderPass renderPass, int flags);

// Deletes NanoVG context and frees all resources.
void nvgDeleteVk(NVGcontext* ctx);

// Sets the base path for shader files.
// Must be called before creating any pipelines (i.e., before nvgBeginFrame()).
// Parameters:
//   ctx - NanoVG context
//   path - Base directory path for shader files (e.g., "/path/to/nanovg/src/shaders")
//          If NULL, defaults to "src/shaders" (legacy behavior)
// Note: The path string is copied internally and can be freed after this call returns.
void nvgVkSetShaderPath(NVGcontext* ctx, const char* path);

// Returns the Vulkan command buffer that contains the rendering commands.
// This should be called after nvgEndFrame() to get the command buffer for submission.
VkCommandBuffer nvgVkGetCommandBuffer(NVGcontext* ctx);

// Sets the current framebuffer for rendering.
// This must be called before nvgBeginFrame().
void nvgVkSetFramebuffer(NVGcontext* ctx, VkFramebuffer framebuffer, uint32_t width, uint32_t height);

// Notifies NanoVG that a render pass has started.
// Call this after vkCmdBeginRenderPass() and vkCmdSetViewport/vkCmdSetScissor to allow NanoVG to track render pass state.
// The renderPassInfo, viewport, and scissor are stored so the render pass can be restarted if needed (e.g., for texture uploads).
void nvgVkBeginRenderPass(NVGcontext* ctx, const VkRenderPassBeginInfo* renderPassInfo,
                          VkViewport viewport, VkRect2D scissor);

// Notifies NanoVG that the render pass has ended.
// Call this before vkCmdEndRenderPass() if you manage render passes manually.
void nvgVkEndRenderPass(NVGcontext* ctx);

// Glyph ready callback type for virtual atlas
// Called from background thread when a glyph finishes rasterizing
typedef void (*NVGVkGlyphReadyCallback)(void* userdata, uint32_t fontID, uint32_t codepoint, uint32_t size);

// Set callback to be notified when MSDF glyphs finish loading
// Callback is fired from background thread when glyph is rasterized and ready for rendering
void nvgVkSetGlyphReadyCallback(NVGcontext* ctx, NVGVkGlyphReadyCallback callback, void* userdata);

// Color space and HDR control functions

// Set HDR luminance scaling factor
// scale: Luminance multiplier (1.0 = no scaling, >1.0 for HDR displays)
//        Use 0.0 to reset to default based on color space
void nvgVkSetHDRScale(NVGcontext* ctx, float scale);

// Enable/disable soft gamut mapping
// enabled: 0 = hard clip out-of-gamut colors, 1 = soft map (preserves hue)
void nvgVkSetGamutMapping(NVGcontext* ctx, int enabled);

// Enable/disable tone mapping for HDR content
// enabled: 0 = linear scale, 1 = apply tone mapping (ACES, Reinhard, etc.)
void nvgVkSetToneMapping(NVGcontext* ctx, int enabled);

// Debug function to dump the font atlas texture to a PPM file
void nvgVkDumpAtlasTexture(NVGcontext* ctx, const char* filename);

// Debug function to dump a specific texture by index to a PPM file
void nvgVkDumpAtlasTextureByIndex(NVGcontext* ctx, int textureIndex, const char* filename);

// Debug function to dump atlas by format (VK_FORMAT_R8_UNORM for ALPHA, VK_FORMAT_R8G8B8A8_UNORM for RGBA)
void nvgVkDumpAtlasByFormat(NVGcontext* ctx, VkFormat format, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // NVG_VK_H
