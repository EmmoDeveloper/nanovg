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
	// Already implemented in vknvg__renderCreate
	(void)vkContext;
	return 1;
}

// Cleanup text cache
static void vknvg__cleanupTextCache(void* vkContext)
{
	// Already implemented in vknvg__renderDelete
	(void)vkContext;
}

// Check if text run is cached and return cache entry
static VKNVGtextRunEntry* vknvg__lookupTextCache(VKNVGcontext* vk,
                                                  const char* text,
                                                  int fontId,
                                                  float fontSize,
                                                  float letterSpacing,
                                                  float blur,
                                                  uint32_t colorRGBA,
                                                  uint32_t textAlign)
{
	if (!vk || !vk->textCache || !vk->useTextCache) {
		return NULL;
	}

	// Build cache key
	VKNVGtextRunKey key;
	vknvg__buildTextRunKey(&key, text, fontId, fontSize, letterSpacing, blur, colorRGBA, textAlign);

	// Lookup in cache
	VKNVGtextRunEntry* entry = vknvg__findTextRun(vk->textCache, &key);

	return entry;
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
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;
	VKNVGtextRunEntry* entry = vknvg__lookupTextCache(vk, text, fontId, fontSize,
	                                                    letterSpacing, blur, colorRGBA, textAlign);

	return (entry != NULL && entry->texture.allocated) ? VK_TRUE : VK_FALSE;
}

// Render text to cache texture (cache miss path)
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
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;

	if (!vk || !vk->textCache || !vk->useTextCache) {
		return VK_FALSE;
	}

	// Build cache key
	VKNVGtextRunKey key;
	vknvg__buildTextRunKey(&key, text, fontId, fontSize, letterSpacing, blur, colorRGBA, textAlign);

	// Add to cache (evict LRU if full)
	VKNVGtextRunEntry* entry = vknvg__addTextRun(vk->textCache, &key);
	if (!entry) {
		return VK_FALSE;
	}

	// Create cached texture if not already allocated
	if (!entry->texture.allocated) {
		VkResult result = vknvg__createCachedTexture(vk->textCache, &entry->texture,
		                                              VKNVG_TEXT_CACHE_TEXTURE_SIZE,
		                                              VKNVG_TEXT_CACHE_TEXTURE_SIZE);
		if (result != VK_SUCCESS) {
			return VK_FALSE;
		}
	}

	// In a full implementation, would:
	// 1. Begin render pass with cached texture framebuffer
	// 2. Render text to texture using standard text rendering pipeline
	// 3. End render pass
	// 4. Store text metrics (width, height, origin) in entry
	//
	// For now, just mark texture as allocated and store placeholder metrics
	entry->texture.width = 100.0f;  // TODO: Calculate actual text bounds
	entry->texture.height = 20.0f;
	entry->texture.originX = x;
	entry->texture.originY = y;

	return VK_TRUE;
}

// Draw cached text texture (cache hit path - texture blit)
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
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;

	if (!vk || !vk->textCache || !vk->useTextCache) {
		return;
	}

	// Lookup cached entry
	VKNVGtextRunEntry* entry = vknvg__lookupTextCache(vk, text, fontId, fontSize,
	                                                    letterSpacing, blur, colorRGBA, textAlign);

	if (!entry || !entry->texture.allocated) {
		return;
	}

	// In a full implementation, would:
	// 1. Bind cached texture to descriptor set
	// 2. Submit a textured quad covering the text bounds
	// 3. Use standard triangle rendering pipeline
	//
	// Pseudo-code:
	// - Create quad vertices at (x, y) with size (entry->texture.width, entry->texture.height)
	// - Bind cached texture image view
	// - Submit draw call with 6 vertices (2 triangles)
	//
	// For now, this is a placeholder that would integrate with the existing
	// triangle rendering path in nvgText()

	(void)x;
	(void)y;
	(void)entry;
}

// Try to use cached text, return true if cache hit
static VkBool32 vknvg__tryUseCachedText(void* vkContext,
                                        const char* text,
                                        float x, float y,
                                        int fontId,
                                        float fontSize,
                                        float letterSpacing,
                                        float blur,
                                        uint32_t colorRGBA,
                                        uint32_t textAlign)
{
	// Check if text is cached
	if (vknvg__isTextCached(vkContext, text, fontId, fontSize, letterSpacing, blur, colorRGBA, textAlign)) {
		// Cache hit - draw cached texture
		vknvg__drawCachedText(vkContext, text, x, y, fontId, fontSize, letterSpacing, blur, colorRGBA, textAlign);
		return VK_TRUE;
	}

	// Cache miss - caller should render text normally and cache it
	return VK_FALSE;
}

// Update cache after rendering text (cache miss path)
static void vknvg__updateTextCache(void* vkContext,
                                    const char* text,
                                    float x, float y,
                                    int fontId,
                                    float fontSize,
                                    float letterSpacing,
                                    float blur,
                                    uint32_t colorRGBA,
                                    uint32_t textAlign)
{
	// Render text to cache texture
	vknvg__renderTextToCache(vkContext, text, x, y, fontId, fontSize, letterSpacing, blur, colorRGBA, textAlign);
}

// Advance frame counter for LRU tracking
// Should be called once per frame (e.g., at end of nvgEndFrame)
static void vknvg__advanceTextCacheFrame(void* vkContext)
{
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;

	if (!vk || !vk->textCache || !vk->useTextCache) {
		return;
	}

	vknvg__textCacheNextFrame(vk->textCache);
}

// Get text cache statistics
static void vknvg__getTextCacheStatistics(void* vkContext,
                                          uint32_t* hits,
                                          uint32_t* misses,
                                          uint32_t* evictions,
                                          uint32_t* validEntries)
{
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;

	if (!vk || !vk->textCache || !vk->useTextCache) {
		if (hits) *hits = 0;
		if (misses) *misses = 0;
		if (evictions) *evictions = 0;
		if (validEntries) *validEntries = 0;
		return;
	}

	vknvg__getTextCacheStats(vk->textCache, hits, misses, evictions, validEntries);
}

// Reset cache statistics
static void vknvg__resetTextCacheStatistics(void* vkContext)
{
	VKNVGcontext* vk = (VKNVGcontext*)vkContext;

	if (!vk || !vk->textCache || !vk->useTextCache) {
		return;
	}

	vk->textCache->cacheHits = 0;
	vk->textCache->cacheMisses = 0;
	vk->textCache->evictions = 0;
}

#endif // NANOVG_VK_TEXT_CACHE_INTEGRATION_H
