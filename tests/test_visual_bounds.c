#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg/nanovg.h"
#include "backends/vulkan/nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== NanoVG Visual Bounds Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(900, 700, "Visual Bounds Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	int font = nvgCreateFont(vg, "sans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	if (font == -1) {
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Test strings
	const char* tests[] = {
		"Ağpqy",
		"HELLO",
		"örllo",
		"ÄÖÜäöü"
	};

	// Render
	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{1.0f, 1.0f, 1.0f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 900, 700, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 900, 700, 1.0f);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 900, 700);
	nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
	nvgFill(vg);

	// Title - RE-ENABLED TO TEST CORRUPTION
	nvgFontSize(vg, 24.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgText(vg, 50, 40, "Visual Bounding Boxes", NULL);

	// Draw each test string
	float y = 120;
	for (int i = 0; i < 4; i++) {
		float x = 100;
		float bounds[4];

		nvgFontSize(vg, 48.0f);
		nvgFontFace(vg, "sans");
		nvgTextBounds(vg, x, y, tests[i], NULL, bounds);

		// Draw bounding box
		nvgBeginPath(vg);
		nvgRect(vg, bounds[0], bounds[1],
		        bounds[2] - bounds[0], bounds[3] - bounds[1]);
		nvgStrokeColor(vg, nvgRGBA(100, 255, 100, 150));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Draw baseline
		nvgBeginPath(vg);
		nvgMoveTo(vg, x - 10, y);
		nvgLineTo(vg, x + 300, y);
		nvgStrokeColor(vg, nvgRGBA(255, 100, 100, 100));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Draw text
		nvgFontSize(vg, 48.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgText(vg, x, y, tests[i], NULL);

		// Label - DISABLED FOR DEBUGGING
		// nvgFontSize(vg, 12.0f);
		// nvgFontFace(vg, "sans");
		// nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		// char label[64];
		// snprintf(label, sizeof(label), "%.0fx%.0f px",
		//          bounds[2] - bounds[0], bounds[3] - bounds[1]);
		// nvgText(vg, x + 350, y, label, NULL);

		y += 120;
	}

	// Info - RE-ENABLED TO TEST CORRUPTION
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(100, 100, 100, 255));
	nvgText(vg, 50, 670, "Green box = visual bounds | Red line = baseline", NULL);

	nvgEndFrame(vg);

	// Dump atlas texture for debugging
	nvgVkDumpAtlasTexture(vg, "screendumps/atlas_dump.png");

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSem[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSem;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSem[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSem;

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);
	vkWaitForFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame], VK_TRUE, UINT64_MAX);

	window_save_screenshot(winCtx, imageIndex, "screendumps/visual_bounds_test.png");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Visual Bounds Test PASSED ===\n");
	return 0;
}
