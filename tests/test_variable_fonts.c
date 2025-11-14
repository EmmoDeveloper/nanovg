#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg.h"
#include "nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== NanoVG Variable Fonts Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1400, 1000, "Variable Fonts Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load Roboto Flex variable font
	int font = nvgCreateFont(vg, "roboto", "fonts/variable/Roboto_Flex/RobotoFlex-VariableFont_GRAD,XOPQ,XTRA,YOPQ,YTAS,YTDE,YTFI,YTLC,YTUC,opsz,slnt,wdth,wght.ttf");
	if (font == -1) {
		printf("Failed to load Roboto Flex\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Set the font face first so the API knows which font to query
	nvgFontFace(vg, "roboto");

	// Check if it's a variable font using the public API
	if (!nvgFontIsVariable(vg)) {
		printf("Font is not a variable font!\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	// Get axis information using the public API
	int axisCount = nvgFontVariationAxisCount(vg);
	printf("Variable font loaded: %d axes found\n", axisCount);

	NVGvarAxis axes[20];
	for (int i = 0; i < axisCount && i < 20; i++) {
		if (nvgFontVariationAxis(vg, i, &axes[i]) == 0) {
			char tagStr[5];
			tagStr[0] = (axes[i].tag >> 24) & 0xFF;
			tagStr[1] = (axes[i].tag >> 16) & 0xFF;
			tagStr[2] = (axes[i].tag >> 8) & 0xFF;
			tagStr[3] = axes[i].tag & 0xFF;
			tagStr[4] = '\0';
			printf("  Axis %d: '%s' (%s) - min=%.1f, default=%.1f, max=%.1f\n",
			       i, tagStr, axes[i].name,
			       axes[i].minimum, axes[i].def, axes[i].maximum);
		}
	}

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

	VkViewport viewport = {0, 0, 1400, 1000, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1400, 1000, 1.0f);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1400, 1000);
	nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
	nvgFill(vg);

	// Title
	nvgFontSize(vg, 32.0f);
	nvgFontFace(vg, "roboto");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 50, 50, "Variable Font Axis Test: Roboto Flex", NULL);

	// Test text to show variations
	const char* testText = "The quick brown fox";

	// Draw variations for each axis
	float y = 100;
	float fontSize = 28.0f;

	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	// Test 1: Weight axis (wght)
	int wghtAxisIdx = -1;
	unsigned int wghtTag = ('w' << 24) | ('g' << 16) | ('h' << 8) | 't';
	for (int i = 0; i < axisCount; i++) {
		if (axes[i].tag == wghtTag) {
			wghtAxisIdx = i;
			break;
		}
	}

	if (wghtAxisIdx >= 0) {
		// Reset to default for section header
		float defaultCoords[20] = {0};
		for (int i = 0; i < axisCount; i++) {
			defaultCoords[i] = axes[i].def;
		}
		nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

		nvgFontSize(vg, 18.0f);
		nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgText(vg, 50, y, "Weight (wght) variations:", NULL);
		y += 30;

		// Show 5 weight variations
		float weights[] = {100, 300, 400, 700, 900};
		for (int i = 0; i < 5; i++) {
			// Draw label with default font
			nvgFontSetVariationAxes(vg, defaultCoords, axisCount);
			nvgFontSize(vg, 16.0f);
			nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
			char label[32];
			snprintf(label, sizeof(label), "wght=%.0f:", weights[i]);
			float labelX = nvgText(vg, 70, y, label, NULL);

			// Set variation for test text and draw
			float coords[20];
			for (int j = 0; j < axisCount; j++) {
				coords[j] = defaultCoords[j];
			}
			coords[wghtAxisIdx] = weights[i];
			nvgFontSetVariationAxes(vg, coords, axisCount);
			nvgFontSize(vg, fontSize);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, 200, y, testText, NULL);
			y += 35;
		}
		y += 10;
	}

	// Test 2: Width axis (wdth)
	int wdthAxisIdx = -1;
	unsigned int wdthTag = ('w' << 24) | ('d' << 16) | ('t' << 8) | 'h';
	for (int i = 0; i < axisCount; i++) {
		if (axes[i].tag == wdthTag) {
			wdthAxisIdx = i;
			break;
		}
	}

	if (wdthAxisIdx >= 0) {
		// Reset to default for section header
		float defaultCoords[20] = {0};
		for (int i = 0; i < axisCount; i++) {
			defaultCoords[i] = axes[i].def;
		}
		nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

		nvgFontSize(vg, 18.0f);
		nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgText(vg, 50, y, "Width (wdth) variations:", NULL);
		y += 30;

		// Show width variations
		float widths[] = {25, 75, 100, 125, 151};
		for (int i = 0; i < 5; i++) {
			// Reset to default for label
			nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

			// Draw label with default font
			nvgFontSize(vg, 16.0f);
			nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
			char label[32];
			snprintf(label, sizeof(label), "wdth=%.0f:", widths[i]);
			nvgText(vg, 70, y, label, NULL);

			// Set variation for test text
			float coords[20];
			for (int j = 0; j < axisCount; j++) {
				coords[j] = defaultCoords[j];
			}
			coords[wdthAxisIdx] = widths[i];
			nvgFontSetVariationAxes(vg, coords, axisCount);

			// Draw test text with variation
			nvgFontSize(vg, fontSize);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, 200, y, testText, NULL);
			y += 35;
		}
		y += 10;
	}

	// Test 3: Slant axis (slnt)
	int slntAxisIdx = -1;
	unsigned int slntTag = ('s' << 24) | ('l' << 16) | ('n' << 8) | 't';
	for (int i = 0; i < axisCount; i++) {
		if (axes[i].tag == slntTag) {
			slntAxisIdx = i;
			break;
		}
	}

	if (slntAxisIdx >= 0) {
		// Reset to default for section header
		float defaultCoords[20] = {0};
		for (int i = 0; i < axisCount; i++) {
			defaultCoords[i] = axes[i].def;
		}
		nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

		nvgFontSize(vg, 18.0f);
		nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgText(vg, 50, y, "Slant (slnt) variations:", NULL);
		y += 30;

		// Show slant variations
		float slants[] = {-10, -5, 0};
		for (int i = 0; i < 3; i++) {
			// Reset to default for label
			nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

			// Draw label with default font
			nvgFontSize(vg, 16.0f);
			nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
			char label[32];
			snprintf(label, sizeof(label), "slnt=%.0f:", slants[i]);
			nvgText(vg, 70, y, label, NULL);

			// Set variation for test text
			float coords[20];
			for (int j = 0; j < axisCount; j++) {
				coords[j] = defaultCoords[j];
			}
			coords[slntAxisIdx] = slants[i];
			nvgFontSetVariationAxes(vg, coords, axisCount);

			// Draw test text with variation
			nvgFontSize(vg, fontSize);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, 200, y, testText, NULL);
			y += 35;
		}
		y += 10;
	}

	// Test 4: Optical Size (opsz)
	int opszAxisIdx = -1;
	unsigned int opszTag = ('o' << 24) | ('p' << 16) | ('s' << 8) | 'z';
	for (int i = 0; i < axisCount; i++) {
		if (axes[i].tag == opszTag) {
			opszAxisIdx = i;
			break;
		}
	}

	if (opszAxisIdx >= 0) {
		// Reset to default for section header
		float defaultCoords[20] = {0};
		for (int i = 0; i < axisCount; i++) {
			defaultCoords[i] = axes[i].def;
		}
		nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

		nvgFontSize(vg, 18.0f);
		nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgText(vg, 50, y, "Optical Size (opsz) variations:", NULL);
		y += 30;

		// Show optical size variations
		float sizes[] = {8, 12, 18, 72, 144};
		for (int i = 0; i < 5; i++) {
			// Reset to default for label
			nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

			// Draw label with default font
			nvgFontSize(vg, 16.0f);
			nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
			char label[32];
			snprintf(label, sizeof(label), "opsz=%.0f:", sizes[i]);
			nvgText(vg, 70, y, label, NULL);

			// Set variation for test text
			float coords[20];
			for (int j = 0; j < axisCount; j++) {
				coords[j] = defaultCoords[j];
			}
			coords[opszAxisIdx] = sizes[i];
			nvgFontSetVariationAxes(vg, coords, axisCount);

			// Draw test text with variation
			nvgFontSize(vg, fontSize);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, 200, y, testText, NULL);
			y += 35;
		}
	}

	// Footer info
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));

	// Reset to default
	float defaultCoords[20] = {0};
	for (int i = 0; i < axisCount; i++) {
		defaultCoords[i] = axes[i].def;
	}
	nvgFontSetVariationAxes(vg, defaultCoords, axisCount);

	char footer[256];
	snprintf(footer, sizeof(footer), "Font: Roboto Flex with %d variable axes", axisCount);
	nvgText(vg, 50, 970, footer, NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/variable_fonts_test.png");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\n=== Variable Fonts Test PASSED ===\n");
	return 0;
}
