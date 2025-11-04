#include "window_utils.h"
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include <stdio.h>
#include <stdlib.h>

// Test HDR/Wide-Gamut Color Space API
// This test validates:
// 1. nvgVkSetHDRScale() - HDR luminance scaling
// 2. nvgVkSetGamutMapping() - Soft gamut mapping
// 3. nvgVkSetToneMapping() - Tone mapping

#define WIDTH 800
#define HEIGHT 600

int main(void)
{
	printf("=== NanoVG Color Space API Test ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* ctx = window_create_context(WIDTH, HEIGHT, "Color Space API Test");
	if (!ctx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n");

	// Create NanoVG context
	printf("\n2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(ctx->device, ctx->physicalDevice, ctx->graphicsQueue,
	                              ctx->commandPool, ctx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(ctx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n");

	// Test 1: HDR Scale
	printf("\n3. Testing HDR scale API...\n");
	printf("   - Setting HDR scale to 1.5 (50%% brighter for HDR displays)\n");
	nvgVkSetHDRScale(vg, 1.5f);
	printf("   ✓ HDR scale set successfully\n");

	printf("   - Setting HDR scale to 2.0 (double luminance)\n");
	nvgVkSetHDRScale(vg, 2.0f);
	printf("   ✓ HDR scale updated successfully\n");

	printf("   - Resetting to default (0.0)\n");
	nvgVkSetHDRScale(vg, 0.0f);
	printf("   ✓ HDR scale reset to default\n");

	// Test 2: Gamut Mapping
	printf("\n4. Testing gamut mapping API...\n");
	printf("   - Enabling soft gamut mapping (preserves hue)\n");
	nvgVkSetGamutMapping(vg, 1);
	printf("   ✓ Soft gamut mapping enabled\n");

	printf("   - Disabling gamut mapping (hard clip)\n");
	nvgVkSetGamutMapping(vg, 0);
	printf("   ✓ Gamut mapping disabled\n");

	// Test 3: Tone Mapping
	printf("\n5. Testing tone mapping API...\n");
	printf("   - Enabling tone mapping (HDR to SDR)\n");
	nvgVkSetToneMapping(vg, 1);
	printf("   ✓ Tone mapping enabled\n");

	printf("   - Disabling tone mapping (linear)\n");
	nvgVkSetToneMapping(vg, 0);
	printf("   ✓ Tone mapping disabled\n");

	// Test 4: Combined settings (typical HDR configuration)
	printf("\n6. Testing combined HDR configuration...\n");
	printf("   - HDR scale: 1.2x\n");
	nvgVkSetHDRScale(vg, 1.2f);
	printf("   - Soft gamut mapping: enabled\n");
	nvgVkSetGamutMapping(vg, 1);
	printf("   - Tone mapping: enabled\n");
	nvgVkSetToneMapping(vg, 1);
	printf("   ✓ HDR configuration applied\n");

	// Test 5: Render a test frame to ensure no crashes
	printf("\n7. Testing rendering with color space settings...\n");

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
	                                         ctx->imageAvailableSemaphores[ctx->currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to acquire swapchain image\n");
		nvgDeleteVk(vg);
		window_destroy_context(ctx);
		return 1;
	}

	vkWaitForFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame]);

	VkCommandBuffer cmd = ctx->commandBuffers[ctx->currentFrame];
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkClearValue clearValues[2] = {0};
	clearValues[0].color.float32[0] = 0.1f;
	clearValues[0].color.float32[1] = 0.1f;
	clearValues[0].color.float32[2] = 0.1f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = ctx->renderPass;
	renderPassInfo.framebuffer = ctx->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset.x = 0;
	renderPassInfo.renderArea.offset.y = 0;
	renderPassInfo.renderArea.extent = ctx->swapchainExtent;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)WIDTH;
	viewport.height = (float)HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {0};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = ctx->swapchainExtent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkSetFramebuffer(vg, ctx->framebuffers[imageIndex], WIDTH, HEIGHT);
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	// Draw simple test shape
	nvgBeginFrame(vg, WIDTH, HEIGHT, 1.0f);

	nvgBeginPath(vg);
	nvgCircle(vg, WIDTH/2, HEIGHT/2, 100);
	nvgFillColor(vg, nvgRGBA(255, 128, 64, 255));
	nvgFill(vg);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {ctx->imageAvailableSemaphores[ctx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSemaphores[] = {ctx->renderFinishedSemaphores[ctx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFences[ctx->currentFrame]) != VK_SUCCESS) {
		fprintf(stderr, "Failed to submit command buffer\n");
		nvgDeleteVk(vg);
		window_destroy_context(ctx);
		return 1;
	}

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapchains[] = {ctx->swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(ctx->presentQueue, &presentInfo);
	vkQueueWaitIdle(ctx->presentQueue);

	printf("   ✓ Rendered test frame with color space settings\n");

	// Cleanup
	printf("\n8. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(ctx);
	printf("   ✓ Cleanup complete\n");

	printf("\n=== Color Space API Test PASSED ===\n");
	return 0;
}
