#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== NanoVG Bidirectional Text Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "BiDi Text Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load fonts
	int arabicFont = nvgCreateFont(vg, "arabic", "fonts/arabic/NotoSansArabic-Regular.ttf");
	int hebrewFont = nvgCreateFont(vg, "hebrew", "fonts/hebrew/NotoSansHebrew-Regular.ttf");
	int latinFont = nvgCreateFont(vg, "latin", "fonts/sans/NotoSans-Regular.ttf");

	if (arabicFont == -1 || hebrewFont == -1 || latinFont == -1) {
		printf("Failed to load fonts\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Test strings
	// Arabic: "Hello" (مرحبا) + space + "World" (عالم)
	const char* arabicText = "مرحبا عالم";
	// Hebrew: "Shalom" (שלום) + space + "World" (עולם)
	const char* hebrewText = "שלום עולם";
	// Mixed: English + Arabic
	const char* mixedText = "Hello مرحبا World";
	// Numbers in Arabic text
	const char* arabicNumbers = "رقم 123 في النص";

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
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgText(vg, 50, 50, "Bidirectional Text Test", NULL);

	float y = 120;
	float fontSize = 36.0f;

	// Test 1: Arabic (RTL)
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Arabic (Auto Direction):", NULL);
	y += 30;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "arabic");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, arabicText, NULL);
	y += 60;

	// Test 2: Arabic (Forced LTR for comparison)
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Arabic (Forced LTR - incorrect):", NULL);
	y += 30;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "arabic");
	nvgFillColor(vg, nvgRGBA(255, 128, 128, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_LTR);
	nvgText(vg, 50, y, arabicText, NULL);
	y += 60;

	// Test 3: Hebrew (RTL)
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, "Hebrew (Auto Direction):", NULL);
	y += 30;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "hebrew");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, hebrewText, NULL);
	y += 60;

	// Test 4: Mixed text (bidirectional within one line)
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgText(vg, 50, y, "Mixed English + Arabic (Auto):", NULL);
	y += 30;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "arabic");
	nvgFillColor(vg, nvgRGBA(180, 255, 180, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, mixedText, NULL);
	y += 60;

	// Test 5: Arabic with numbers
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, "Arabic with numbers (Auto):", NULL);
	y += 30;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "arabic");
	nvgFillColor(vg, nvgRGBA(255, 220, 150, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, y, arabicNumbers, NULL);

	// Footer
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "latin");
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
	nvgTextDirection(vg, NVG_TEXT_DIR_AUTO);
	nvgText(vg, 50, 870, "BiDi support using FriBidi + HarfBuzz", NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/bidi_test.png");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== BiDi Test PASSED ===\n");
	return 0;
}
