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

#include "nanovg.h"
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

#ifdef __cplusplus
}
#endif

#endif // NVG_VK_H
