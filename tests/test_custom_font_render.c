#include "../src/nvg_font.h"
#include "../src/vulkan/nvg_vk_context.h"
#include "../src/vulkan/nvg_vk_buffer.h"
#include "../src/vulkan/nvg_vk_texture.h"
#include "../src/vulkan/nvg_vk_pipeline.h"
#include "../src/vulkan/nvg_vk_render.h"
#include "../src/vulkan/nvg_vk_types.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FONT_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"

int main(void)
{
	printf("=== Custom Font Vulkan Rendering Test ===\n\n");

	// Initialize font system
	if (!nvgfont_init()) {
		fprintf(stderr, "Failed to initialize font system\n");
		return 1;
	}

	// Load font and rasterize glyphs
	NVGFont* font = nvgfont_create(FONT_PATH);
	if (!font) {
		nvgfont_shutdown();
		return 1;
	}

	nvgfont_set_size(font, 48.0f);
	
	const char* text = "abjg?";
	printf("Rasterizing '%s'...\n", text);
	
	for (const char* p = text; *p; p++) {
		nvgfont_get_glyph(font, *p);
	}

	// Get atlas data
	int atlasWidth, atlasHeight;
	const unsigned char* atlasData = nvgfont_get_atlas_data(font, &atlasWidth, &atlasHeight);
	printf("Atlas size: %dx%d\n\n", atlasWidth, atlasHeight);

	// Create window and Vulkan context
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Custom Font Test");
	if (!winCtx) {
		nvgfont_destroy(font);
		nvgfont_shutdown();
		return 1;
	}

	// Create NanoVG Vulkan context
	NVGVkContext nvgCtx = {0};
	NVGVkCreateInfo createInfo = {0};
	createInfo.device = winCtx->device;
	createInfo.physicalDevice = winCtx->physicalDevice;
	createInfo.queue = winCtx->graphicsQueue;
	createInfo.commandPool = winCtx->commandPool;
	createInfo.flags = 0;

	if (!nvgvk_create(&nvgCtx, &createInfo)) {
		fprintf(stderr, "Failed to create NanoVG Vulkan context\n");
		window_destroy_context(winCtx);
		nvgfont_destroy(font);
		nvgfont_shutdown();
		return 1;
	}

	nvgvk_viewport(&nvgCtx, 800.0f, 600.0f, 1.0f);

	// Create ALPHA texture for font atlas
	printf("Creating font atlas texture...\n");
	int texId = nvgvk_create_texture(&nvgCtx, NVG_TEXTURE_ALPHA, atlasWidth, atlasHeight, 0, atlasData);
	if (texId < 0) {
		fprintf(stderr, "Failed to create texture\n");
		window_destroy_context(winCtx);
		nvgfont_destroy(font);
		nvgfont_shutdown();
		return 1;
	}
	printf("✓ Texture created (id=%d)\n", texId);

	// Create pipelines
	printf("\nCreating pipelines...\n");
	if (!nvgvk_create_pipelines(&nvgCtx, winCtx->renderPass)) {
		fprintf(stderr, "Failed to create pipelines\n");
		nvgvk_delete(&nvgCtx);
		window_destroy_context(winCtx);
		nvgfont_destroy(font);
		nvgfont_shutdown();
		return 1;
	}
	printf("✓ Pipelines created\n");

	// Generate text geometry in NVGVkContext
	printf("\nGenerating text geometry...\n");
	int start = nvgCtx.vertexCount;
	NVGvertex* verts = (NVGvertex*)nvgCtx.vertices;

	float x = 100.0f, y = 300.0f;

	for (const char* p = text; *p; p++) {
		NVGGlyph* glyph = nvgfont_get_glyph(font, *p);
		if (!glyph) continue;

		// FreeType metrics:
		// bearingY = distance from baseline UP to top of glyph bitmap
		// height = total pixel height of bitmap
		//
		// NOTE: The vertex shader NEGATES Y (-ndc.y), which flips the coordinate system!
		// This means smaller Y values appear HIGHER on screen (top of window = y=0)
		// So we need to think in terms of Y increasing UPWARD from bottom of window
		//
		// For baseline at y:
		// - Top of glyph should be ABOVE baseline: y + bearingY (larger Y = higher up after flip)
		// - Bottom should be below: y - (height - bearingY)
		float x0 = x + glyph->bearingX;
		float y1 = y - (glyph->height - glyph->bearingY);  // bottom (lower Y value = lower on screen after flip)
		float x1 = x0 + glyph->width;
		float y0 = y + glyph->bearingY;                    // top (higher Y value = higher on screen after flip)

		// Two triangles per glyph
		// Use UVs as-is (don't flip)
		verts[nvgCtx.vertexCount++] = (NVGvertex){x0, y0, glyph->u0, glyph->v0};
		verts[nvgCtx.vertexCount++] = (NVGvertex){x1, y1, glyph->u1, glyph->v1};
		verts[nvgCtx.vertexCount++] = (NVGvertex){x1, y0, glyph->u1, glyph->v0};

		verts[nvgCtx.vertexCount++] = (NVGvertex){x0, y0, glyph->u0, glyph->v0};
		verts[nvgCtx.vertexCount++] = (NVGvertex){x0, y1, glyph->u0, glyph->v1};
		verts[nvgCtx.vertexCount++] = (NVGvertex){x1, y1, glyph->u1, glyph->v1};

		x += glyph->advance;

		printf("  Glyph '%c': quad (%.0f,%.0f)-(%.0f,%.0f) uv=(%.3f,%.3f)-(%.3f,%.3f)\n",
		       *p, x0, y0, x1, y1, glyph->u0, glyph->v0, glyph->u1, glyph->v1);
	}

	// Add render call
	int callIdx = nvgCtx.callCount++;
	nvgCtx.calls[callIdx].type = NVGVK_TRIANGLES;
	nvgCtx.calls[callIdx].image = texId;
	nvgCtx.calls[callIdx].triangleOffset = start;
	nvgCtx.calls[callIdx].triangleCount = nvgCtx.vertexCount - start;
	nvgCtx.calls[callIdx].uniformOffset = nvgCtx.uniformCount;
	nvgCtx.calls[callIdx].blendFunc = 0;

	// Setup uniforms (white color for text)
	memset(&nvgCtx.uniforms[nvgCtx.uniformCount], 0, sizeof(NVGVkUniforms));
	nvgCtx.uniforms[nvgCtx.uniformCount].viewSize[0] = 800.0f;
	nvgCtx.uniforms[nvgCtx.uniformCount].viewSize[1] = 600.0f;
	nvgCtx.uniforms[nvgCtx.uniformCount].innerCol[0] = 1.0f;  // R
	nvgCtx.uniforms[nvgCtx.uniformCount].innerCol[1] = 1.0f;  // G
	nvgCtx.uniforms[nvgCtx.uniformCount].innerCol[2] = 1.0f;  // B
	nvgCtx.uniforms[nvgCtx.uniformCount].innerCol[3] = 1.0f;  // A
	nvgCtx.uniforms[nvgCtx.uniformCount].texType = 2;  // ALPHA texture (OpenGL encoding: 0=RGBA premult, 1=RGBA, 2=ALPHA)
	nvgCtx.uniformCount++;

	printf("✓ Generated %d vertices, %d calls\n", nvgCtx.vertexCount, nvgCtx.callCount);

	// Setup descriptors
	printf("\nSetting up descriptors...\n");
	nvgvk_setup_render(&nvgCtx);
	printf("✓ Descriptors set up\n");

	// Render frame
	printf("\nRendering frame...\n");
	uint32_t imageIndex;
	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);
	vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &renderFinishedSemaphore);

	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(nvgCtx.commandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

	VkClearValue clearValues[2] = {0};
	clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.1f, 1.0f}};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(nvgCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.width = 800.0f;
	viewport.height = 600.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(nvgCtx.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.extent = winCtx->swapchainExtent;
	vkCmdSetScissor(nvgCtx.commandBuffer, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgvk_begin_render_pass(&nvgCtx, renderPassInfo.renderPass, renderPassInfo.framebuffer,
	                        renderPassInfo.renderArea, clearValues, 2, viewport, scissor);

	nvgvk_flush(&nvgCtx);

	vkCmdEndRenderPass(nvgCtx.commandBuffer);
	vkEndCommandBuffer(nvgCtx.commandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &nvgCtx.commandBuffer;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(winCtx->graphicsQueue);

	vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);
	vkDestroySemaphore(winCtx->device, renderFinishedSemaphore, NULL);

	printf("✓ Frame rendered\n");

	// Save screenshot
	printf("\nSaving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "custom_font_test.ppm")) {
		printf("✓ Screenshot saved to custom_font_test.ppm\n");
	}

	printf("\n=== Test PASSED ===\n");
	printf("Check custom_font_test.ppm to verify text rendering!\n");

	// Cleanup
	nvgvk_delete(&nvgCtx);
	window_destroy_context(winCtx);
	nvgfont_destroy(font);
	nvgfont_shutdown();

	printf("\n=== Test PASSED ===\n");
	return 0;
}
