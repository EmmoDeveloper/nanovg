#define _POSIX_C_SOURCE 199309L
#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>
#include <time.h>

int main(void)
{
	printf("=== NanoVG Chinese Poem Test (Vulkan Window) ===\n\n");

	// Create window context
	printf("1. Creating Vulkan window context...\n");
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Chinese Poem Test");
	if (!winCtx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}
	printf("   ✓ Window context created\n\n");

	// Create NanoVG context
	printf("2. Creating NanoVG context...\n");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ NanoVG context created\n\n");

	// Load Chinese font
	printf("3. Loading Chinese CJK font...\n");
	int fontChinese = nvgCreateFont(vg, "chinese", "fonts/cjk/NotoSansCJKsc-Regular.otf");
	if (fontChinese == -1) {
		fprintf(stderr, "Failed to load Chinese font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("   ✓ Chinese font loaded (id=%d)\n\n", fontChinese);

	// 静夜思 (Quiet Night Thought) by Li Bai
	const char* poem[] = {
		"静夜思",           // Title
		"床前明月光，",     // Line 1
		"疑是地上霜。",     // Line 2
		"举头望明月，",     // Line 3
		"低头思故乡。",     // Line 4
		"— 李白"            // Author
	};

	// Render multiple frames to display the poem
	printf("4. Rendering poem in Vulkan window...\n");
	printf("   Poem: 静夜思 (Quiet Night Thought) by Li Bai\n\n");

	for (int frame = 0; frame < 120; frame++) {  // Show for ~2 seconds at 60fps
		// Acquire swapchain image
		uint32_t imageIndex;
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkSemaphore imageAvailableSemaphore;
		vkCreateSemaphore(winCtx->device, &semaphoreInfo, NULL, &imageAvailableSemaphore);

		vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
		                      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		// Get command buffer from NanoVG
		VkCommandBuffer cmdBuf = nvgVkGetCommandBuffer(vg);

		// Begin command buffer
		VkCommandBufferBeginInfo beginInfo = {0};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmdBuf, &beginInfo);

		// Begin render pass
		VkRenderPassBeginInfo renderPassInfo = {0};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = winCtx->renderPass;
		renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
		renderPassInfo.renderArea.extent = winCtx->swapchainExtent;

		// Dark blue background
		VkClearValue clearValues[2] = {0};
		clearValues[0].color = (VkClearColorValue){{0.1f, 0.1f, 0.3f, 1.0f}};
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		VkViewport viewport = {0};
		viewport.width = 800.0f;
		viewport.height = 600.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = {0};
		scissor.extent = winCtx->swapchainExtent;
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// Notify NanoVG that render pass has started
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

		// Begin NanoVG frame
		nvgBeginFrame(vg, 800, 600, 1.0f);

		// Draw poem
		nvgFontFace(vg, "chinese");

		float y = 100.0f;

		// Title - gold color, larger size
		nvgFontSize(vg, 56.0f);
		nvgFillColor(vg, nvgRGBA(255, 230, 128, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgText(vg, 400.0f, y, poem[0], NULL);
		y += 80.0f;

		// Poem lines - different colors
		nvgFontSize(vg, 40.0f);
		NVGcolor lineColors[] = {
			nvgRGBA(230, 230, 255, 255),  // bright white-blue
			nvgRGBA(200, 230, 255, 255),  // light blue
			nvgRGBA(255, 230, 180, 255),  // warm white
			nvgRGBA(230, 200, 255, 255)   // light purple
		};

		for (int i = 0; i < 4; i++) {
			nvgFillColor(vg, lineColors[i]);
			nvgText(vg, 400.0f, y, poem[i + 1], NULL);
			y += 60.0f;
		}

		// Author - gray
		y += 20.0f;
		nvgFontSize(vg, 32.0f);
		nvgFillColor(vg, nvgRGBA(180, 180, 200, 255));
		nvgText(vg, 400.0f, y, poem[5], NULL);

		nvgEndFrame(vg);

		// End render pass and command buffer
		vkCmdEndRenderPass(cmdBuf);
		vkEndCommandBuffer(cmdBuf);

		// Submit command buffer
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.pWaitDstStageMask = waitStages;

		vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(winCtx->graphicsQueue);

		// Present
		VkPresentInfoKHR presentInfo = {0};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &winCtx->swapchain;
		presentInfo.pImageIndices = &imageIndex;
		vkQueuePresentKHR(winCtx->graphicsQueue, &presentInfo);

		vkDestroySemaphore(winCtx->device, imageAvailableSemaphore, NULL);

		// Progress indicator
		if (frame % 30 == 0) {
			printf("   Frame %d/120...\n", frame);
		}

		// Small delay to see the rendering
		struct timespec ts = {0, 16666666};  // 16.666ms = ~60 fps
		nanosleep(&ts, NULL);
	}

	printf("\n5. Poem displayed successfully\n");
	printf("\nPoem Information:\n");
	printf("  Title: 静夜思 (Jìng Yè Sī - Quiet Night Thought)\n");
	printf("  Author: 李白 (Lǐ Bái - Li Bai, 701-762 CE)\n");
	printf("  Dynasty: Tang Dynasty (唐朝)\n");
	printf("\nTranslation:\n");
	printf("  Before my bed, the bright moonlight,\n");
	printf("  I suspect it's frost on the ground.\n");
	printf("  Lifting my head, I gaze at the bright moon,\n");
	printf("  Lowering my head, I think of my hometown.\n");

	// Cleanup
	printf("\n6. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== NanoVG Chinese Poem Test PASSED ===\n");
	return 0;
}
