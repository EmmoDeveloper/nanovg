#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== NanoVG OpenType Features Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "OpenType Features Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load a font with OpenType features (using Inter which has extensive features)
	int font = nvgCreateFont(vg, "sans", "fonts/variable/Inter/Inter-VariableFont_opsz,wght.ttf");
	if (font == -1) {
		printf("Failed to load Inter font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Test strings showcasing different features
	const char* ligatureText = "fi fl ff ffi ffl";  // Standard ligatures
	const char* numbersText = "0123456789";  // For testing number features
	const char* fractionsText = "1/2 1/4 3/4";  // Fractions

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
	VkClearValue clearColor = {{{0.15f, 0.15f, 0.15f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1200, 900, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1200, 900, 1.0f);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1200, 900);
	nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
	nvgFill(vg);

	// Title
	nvgFontSize(vg, 32.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, 50, "OpenType Features Test", NULL);

	float y = 120;
	float fontSize = 36.0f;

	// Test 1: Standard Ligatures (liga)
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgFontFeaturesReset(vg);
	nvgText(vg, 50, y, "Standard Ligatures (liga):", NULL);
	y += 30;

	// With ligatures enabled
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_LIGA, 1);  // Enable ligatures

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Enabled:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, ligatureText, NULL);
	y += 45;

	// With ligatures disabled
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_LIGA, 0);  // Disable ligatures

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Disabled:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, ligatureText, NULL);
	y += 60;

	// Test 2: Contextual Alternates (calt)
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgFontFeaturesReset(vg);
	nvgText(vg, 50, y, "Contextual Alternates (calt):", NULL);
	y += 30;

	// Enabled
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_CALT, 1);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Enabled:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, "The quick brown fox", NULL);
	y += 45;

	// Disabled
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_CALT, 0);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Disabled:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, "The quick brown fox", NULL);
	y += 60;

	// Test 3: Tabular Numbers (tnum)
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgFontFeaturesReset(vg);
	nvgText(vg, 50, y, "Tabular Numbers (tnum):", NULL);
	y += 30;

	// Proportional (default)
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_TNUM, 0);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Proportional:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, numbersText, NULL);
	nvgText(vg, 200, y + 35, "1111111111", NULL);
	y += 80;

	// Tabular
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_TNUM, 1);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Tabular:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, numbersText, NULL);
	nvgText(vg, 200, y + 35, "1111111111", NULL);
	y += 80;

	// Test 4: Slashed Zero (zero)
	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgFontFeaturesReset(vg);
	nvgText(vg, 50, y, "Slashed Zero (zero):", NULL);
	y += 30;

	// Normal zero
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_ZERO, 0);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Normal:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, "0 O 0 O 0 O", NULL);
	y += 45;

	// Slashed zero
	nvgFontFeaturesReset(vg);
	nvgFontFeature(vg, NVG_FEATURE_ZERO, 1);

	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "Slashed:", NULL);

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 200, y, "0 O 0 O 0 O", NULL);

	// Footer
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
	nvgFontFeaturesReset(vg);
	nvgText(vg, 50, 870, "Inter font supports liga, tnum, and zero features", NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/opentype_features_test.ppm");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== OpenType Features Test PASSED ===\n");
	return 0;
}
