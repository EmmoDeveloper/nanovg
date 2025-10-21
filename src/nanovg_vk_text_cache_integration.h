// Text Run Cache Integration
// Integrates text run caching with NanoVG rendering pipeline

#ifndef NANOVG_VK_TEXT_CACHE_INTEGRATION_H
#define NANOVG_VK_TEXT_CACHE_INTEGRATION_H

#include "nanovg_vk_text_cache.h"
#include <vulkan/vulkan.h>

// Create render pass for text-to-texture rendering
static VkRenderPass vknvg__createTextCacheRenderPass(VkDevice device)
{
	if (!device) return VK_NULL_HANDLE;

	// Single color attachment: R8_UNORM for text
	VkAttachmentDescription attachment = {0};
	attachment.format = VK_FORMAT_R8_UNORM;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorRef = {0};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	// Subpass dependency for layout transition
	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return renderPass;
}

// Initialize text cache with context
static int vknvg__initTextCache(void* vkContext)
{
	// This will be implemented to add cache to VKNVGcontext
	// For now, return success
	(void)vkContext;
	return 1;
}

// Cleanup text cache
static void vknvg__cleanupTextCache(void* vkContext)
{
	// This will be implemented to destroy cache from VKNVGcontext
	(void)vkContext;
}

// Check if text run is cached
static VkBool32 vknvg__isTextCached(void* vkContext,
                                     const char* text,
                                     int fontId,
                                     float fontSize,
                                     float letterSpacing,
                                     float blur,
                                     uint32_t colorRGBA,
                                     uint32_t textAlign)
{
	// Placeholder - will implement cache lookup
	(void)vkContext;
	(void)text;
	(void)fontId;
	(void)fontSize;
	(void)letterSpacing;
	(void)blur;
	(void)colorRGBA;
	(void)textAlign;
	return VK_FALSE;
}

// Render text to cache texture
static VkBool32 vknvg__renderTextToCache(void* vkContext,
                                          const char* text,
                                          float x, float y,
                                          int fontId,
                                          float fontSize,
                                          float letterSpacing,
                                          float blur,
                                          uint32_t colorRGBA,
                                          uint32_t textAlign)
{
	// Placeholder - will implement text-to-texture rendering
	(void)vkContext;
	(void)text;
	(void)x;
	(void)y;
	(void)fontId;
	(void)fontSize;
	(void)letterSpacing;
	(void)blur;
	(void)colorRGBA;
	(void)textAlign;
	return VK_FALSE;
}

// Draw cached text texture (blit to screen)
static void vknvg__drawCachedText(void* vkContext,
                                   const char* text,
                                   float x, float y,
                                   int fontId,
                                   float fontSize,
                                   float letterSpacing,
                                   float blur,
                                   uint32_t colorRGBA,
                                   uint32_t textAlign)
{
	// Placeholder - will implement cached texture rendering
	(void)vkContext;
	(void)text;
	(void)x;
	(void)y;
	(void)fontId;
	(void)fontSize;
	(void)letterSpacing;
	(void)blur;
	(void)colorRGBA;
	(void)textAlign;
}

#endif // NANOVG_VK_TEXT_CACHE_INTEGRATION_H
