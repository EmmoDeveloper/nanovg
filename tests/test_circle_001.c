#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "../src/tools/window_utils.h"
#include <stdio.h>

// Test: nvgCircle with parameters: {'cx': 600, 'cy': 100, 'r': 20}

int main(void)
{
	printf("=== Testing nvgCircle variant 1 ===\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "nvgCircle Test");
	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, NVG_ANTIALIAS);

	// Load font
	int font = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");

	// Setup frame
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
	clearValues[0].color = (VkClearColorValue){{0.2f, 0.2f, 0.2f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = {0, 0, (float)winCtx->swapchainExtent.width, (float)winCtx->swapchainExtent.height, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Test the API function
	nvgBeginPath(vg);
	nvgCircle(vg, 600, 100, 20);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);

	// Draw label
	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 10, 20, "nvgCircle - variant 1", NULL);

	nvgEndFrame(vg);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/test_circle_001.ppm");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("Test PASSED: nvgCircle variant 1\n");
	return 0;
}
