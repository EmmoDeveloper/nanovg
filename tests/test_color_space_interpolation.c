#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"
#include "../src/vulkan/nvg_vk_color_space_math.h"

int main(void) {
	printf("=== Color Space Interpolation Test (Quaternion-based) ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1400, 900, "Color Space Interpolation");
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

	// Test quaternion-based matrix interpolation
	printf("Testing quaternion matrix interpolation:\n");

	// Create conversion matrices: sRGB/BT.709 -> Display P3 -> BT.2020
	NVGVkMat3 bt709ToP3, p3ToBt2020;
	nvgvk_primaries_conversion_matrix(&NVGVK_PRIMARIES_BT709, &NVGVK_PRIMARIES_DISPLAYP3, &bt709ToP3);
	nvgvk_primaries_conversion_matrix(&NVGVK_PRIMARIES_DISPLAYP3, &NVGVK_PRIMARIES_BT2020, &p3ToBt2020);

	printf("  BT.709 -> Display P3 matrix:\n");
	printf("    [%.4f %.4f %.4f]\n", bt709ToP3.m[0], bt709ToP3.m[1], bt709ToP3.m[2]);
	printf("    [%.4f %.4f %.4f]\n", bt709ToP3.m[3], bt709ToP3.m[4], bt709ToP3.m[5]);
	printf("    [%.4f %.4f %.4f]\n\n", bt709ToP3.m[6], bt709ToP3.m[7], bt709ToP3.m[8]);

	// Test interpolation at different points
	float testPoints[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
	for (int i = 0; i < 5; i++) {
		float t = testPoints[i];
		NVGVkMat3 interpolated;
		nvgvk_mat3_interpolate(&bt709ToP3, &p3ToBt2020, t, &interpolated);

		printf("  Interpolation at t=%.2f:\n", t);
		printf("    [%.4f %.4f %.4f]\n", interpolated.m[0], interpolated.m[1], interpolated.m[2]);
		printf("    [%.4f %.4f %.4f]\n", interpolated.m[3], interpolated.m[4], interpolated.m[5]);
		printf("    [%.4f %.4f %.4f]\n\n", interpolated.m[6], interpolated.m[7], interpolated.m[8]);
	}

	// Test decomposition stability
	printf("Testing matrix decomposition/recomposition:\n");
	NVGVkMat3Decomposed decomposed;
	nvgvk_mat3_decompose(&bt709ToP3, &decomposed);

	printf("  Quaternion: w=%.4f, x=%.4f, y=%.4f, z=%.4f\n",
	       decomposed.rotation.w, decomposed.rotation.x,
	       decomposed.rotation.y, decomposed.rotation.z);
	printf("  Scale: [%.4f, %.4f, %.4f]\n\n",
	       decomposed.scale[0], decomposed.scale[1], decomposed.scale[2]);

	NVGVkMat3 recomposed;
	nvgvk_mat3_compose(&decomposed, &recomposed);

	printf("  Reconstruction error:\n");
	float maxError = 0.0f;
	for (int i = 0; i < 9; i++) {
		float error = fabsf(bt709ToP3.m[i] - recomposed.m[i]);
		if (error > maxError) maxError = error;
	}
	printf("    Max error: %.10f\n\n", maxError);

	// Render visualization
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

	VkViewport viewport = {0, 0, 1400, 900, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1400, 900, 1.0f);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1400, 900);
	nvgFillColor(vg, nvgRGBA(15, 15, 20, 255));
	nvgFill(vg);

	// Title
	nvgFontSize(vg, 32.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, 50, "Quaternion-Based Color Space Interpolation", NULL);

	// Info text
	nvgFontSize(vg, 16.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 50, 90, "BT.709 (sRGB) -> Display P3 -> BT.2020", NULL);

	// Draw interpolation visualization
	float y = 140;
	const char* labels[] = {"BT.709", "25% -> P3", "50% -> P3", "75% -> P3", "Display P3"};

	for (int i = 0; i < 5; i++) {
		float t = testPoints[i];
		NVGVkMat3 interpolated;
		nvgvk_mat3_interpolate(&bt709ToP3, &p3ToBt2020, t * 0.5f, &interpolated);

		// Label
		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		nvgText(vg, 70, y, labels[i], NULL);

		// Color bar with gradient
		nvgBeginPath(vg);
		nvgRect(vg, 220, y - 20, 1000, 30);
		NVGpaint gradient = nvgLinearGradient(vg, 220, y, 1220, y,
		                                       nvgRGBA(255, 50, 50, 255),
		                                       nvgRGBA(50, 50, 255, 255));
		nvgFillPaint(vg, gradient);
		nvgFill(vg);

		// Matrix values
		nvgFontSize(vg, 11.0f);
		nvgFillColor(vg, nvgRGBA(100, 200, 100, 255));
		char matrixText[128];
		snprintf(matrixText, sizeof(matrixText), "[%.3f %.3f %.3f]",
		         interpolated.m[0], interpolated.m[1], interpolated.m[2]);
		nvgText(vg, 220, y + 50, matrixText, NULL);

		y += 100;
	}

	// Results
	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(100, 255, 100, 255));
	char resultText[256];
	snprintf(resultText, sizeof(resultText),
	         "Decomposition/Recomposition Max Error: %.10f (quaternion ensures orthogonality)",
	         maxError);
	nvgText(vg, 50, 820, resultText, NULL);

	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 50, 850, "Quaternion SLERP provides smooth rotation interpolation between color spaces", NULL);
	nvgText(vg, 50, 870, "Polar decomposition separates rotation from scale for numerical stability", NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/color_space_interpolation.png");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	if (maxError < 1e-6f) {
		printf("=== Color Space Interpolation Test PASSED ===\n");
		printf("Quaternion-based decomposition provides numerical stability\n");
		return 0;
	} else {
		printf("=== Color Space Interpolation Test FAILED ===\n");
		printf("Error too large: %.10f\n", maxError);
		return 1;
	}
}
