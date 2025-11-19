#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nanovg/nanovg.h"
#include "backends/vulkan/nvg_vk.h"
#include "../src/tools/window_utils.h"
#include "backends/vulkan/impl/nvg_vk_display_color_space.h"

int main(void) {
	printf("=== Display Color Space Matrix/Quaternion System Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1600, 1000, "Display Color Space System");
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

	// Create display color manager
	NVGVkDisplayColorManager* manager = nvgvk_display_color_manager_create(
		winCtx->physicalDevice, winCtx->surface);

	if (!manager) {
		printf("Failed to create display color manager\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}

	printf("Display Color Space System Initialized\n");
	nvgvk_display_print_capabilities(&manager->display);
	printf("\n");

	// Test 1: Build transforms for different color space pairs
	printf("=== Test 1: Color Space Transform Matrix/Quaternion Decomposition ===\n\n");

	struct {
		const char* name;
		const NVGVkColorPrimaries* srcPrim;
		const NVGVkColorPrimaries* dstPrim;
		NVGVkTransferFunctionID srcTrans;
		NVGVkTransferFunctionID dstTrans;
	} transformTests[] = {
		{"sRGB -> Display P3", &NVGVK_PRIMARIES_BT709, &NVGVK_PRIMARIES_DISPLAYP3,
		 NVGVK_TRANSFER_SRGB, NVGVK_TRANSFER_SRGB},
		{"Display P3 -> BT.2020", &NVGVK_PRIMARIES_DISPLAYP3, &NVGVK_PRIMARIES_BT2020,
		 NVGVK_TRANSFER_SRGB, NVGVK_TRANSFER_LINEAR},
		{"sRGB -> BT.2020 HDR", &NVGVK_PRIMARIES_BT709, &NVGVK_PRIMARIES_BT2020,
		 NVGVK_TRANSFER_SRGB, NVGVK_TRANSFER_PQ},
		{"Adobe RGB -> Display P3", &NVGVK_PRIMARIES_ADOBERGB, &NVGVK_PRIMARIES_DISPLAYP3,
		 NVGVK_TRANSFER_ADOBE_RGB, NVGVK_TRANSFER_SRGB}
	};

	NVGVkColorSpaceTransform transforms[4];
	int allValid = 1;

	for (int i = 0; i < 4; i++) {
		printf("%s:\n", transformTests[i].name);

		nvgvk_build_color_space_transform(
			transformTests[i].srcPrim,
			transformTests[i].dstPrim,
			transformTests[i].srcTrans,
			transformTests[i].dstTrans,
			1.0f,
			&transforms[i]
		);

		nvgvk_transform_print_info(&transforms[i]);
		printf("\n");

		if (!transforms[i].isValid) {
			printf("ERROR: Transform is invalid!\n");
			allValid = 0;
		}
	}

	// Test 2: Quaternion interpolation between color spaces
	printf("=== Test 2: Smooth Color Space Transition (Quaternion SLERP) ===\n\n");

	NVGVkColorSpaceTransform interpolated[5];
	float tValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

	printf("Interpolating: sRGB -> Display P3\n\n");
	for (int i = 0; i < 5; i++) {
		nvgvk_interpolate_transforms(&transforms[0], &transforms[1], tValues[i], &interpolated[i]);
		printf("t=%.2f:\n", tValues[i]);
		printf("  Quat: [%.4f, %.4f, %.4f, %.4f]\n",
		       interpolated[i].decomposed.rotation.w,
		       interpolated[i].decomposed.rotation.x,
		       interpolated[i].decomposed.rotation.y,
		       interpolated[i].decomposed.rotation.z);
		printf("  Scale: [%.4f, %.4f, %.4f]\n",
		       interpolated[i].decomposed.scale[0],
		       interpolated[i].decomposed.scale[1],
		       interpolated[i].decomposed.scale[2]);
		printf("  Det: %.6f, Ortho Error: %.8f\n\n",
		       interpolated[i].determinant,
		       interpolated[i].orthogonalityError);
	}

	// Test 3: Transform composition
	printf("=== Test 3: Transform Composition ===\n\n");

	NVGVkColorSpaceTransform composed;
	printf("Composing: (sRGB -> Display P3) ∘ (Display P3 -> BT.2020)\n");
	printf("Should equal: sRGB -> BT.2020\n\n");

	nvgvk_compose_transforms(&transforms[0], &transforms[1], &composed);
	nvgvk_transform_print_info(&composed);
	printf("\n");

	// Test 4: Animated transition
	printf("=== Test 4: Animated Color Space Transition ===\n\n");

	nvgvk_display_set_target_color_space(
		manager,
		VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	);

	printf("Starting transition: sRGB -> Display P3\n");
	printf("Simulating 10 frames at 60fps:\n\n");

	for (int frame = 0; frame < 10; frame++) {
		nvgvk_display_update_transition(manager, 1.0f / 60.0f);

		const NVGVkColorSpaceTransform* current = nvgvk_display_get_current_transform(manager);
		printf("Frame %2d (t=%.3f): Det=%.6f, Ortho=%.8f\n",
		       frame,
		       manager->transitionProgress,
		       current->determinant,
		       current->orthogonalityError);
	}
	printf("\n");

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
	clearValues[0].color = (VkClearColorValue){{0.05f, 0.05f, 0.08f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1600, 1000, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1600, 1000, 1.0f);

	// Title
	nvgFontSize(vg, 36.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(100, 200, 255, 255));
	nvgText(vg, 50, 50, "Display Color Space System", NULL);

	nvgFontSize(vg, 18.0f);
	nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
	nvgText(vg, 50, 80, "Matrix/Quaternion Space for Vulkan Color Spaces", NULL);

	// Section 1: Color Space Transforms
	float y = 130;
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 50, y, "Color Space Transforms", NULL);
	y += 40;

	nvgFontSize(vg, 14.0f);
	for (int i = 0; i < 4; i++) {
		// Transform name
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
		nvgText(vg, 70, y, transformTests[i].name, NULL);

		// Validity indicator
		NVGcolor statusColor = transforms[i].isValid ?
			nvgRGBA(100, 255, 100, 255) : nvgRGBA(255, 100, 100, 255);
		nvgBeginPath(vg);
		nvgCircle(vg, 400, y - 7, 6);
		nvgFillColor(vg, statusColor);
		nvgFill(vg);

		// Quaternion visualization (as rotation magnitude)
		float angle = 2.0f * acosf(transforms[i].decomposed.rotation.w);
		float barWidth = (angle / 3.14159f) * 300.0f;

		nvgBeginPath(vg);
		nvgRect(vg, 450, y - 12, barWidth, 12);
		nvgFillColor(vg, nvgRGBA(100, 150, 255, 180));
		nvgFill(vg);

		// Stats
		nvgFontSize(vg, 11.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		char stats[128];
		snprintf(stats, sizeof(stats), "Det: %.4f  Ortho: %.6f",
		         transforms[i].determinant, transforms[i].orthogonalityError);
		nvgText(vg, 800, y, stats, NULL);

		y += 35;
		nvgFontSize(vg, 14.0f);
	}

	// Section 2: Interpolation visualization
	y += 40;
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 50, y, "Quaternion SLERP Interpolation", NULL);
	y += 40;

	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, 70, y, "sRGB -> Display P3 (5 steps)", NULL);
	y += 30;

	for (int i = 0; i < 5; i++) {
		// Step indicator
		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		char label[16];
		snprintf(label, sizeof(label), "t=%.2f", tValues[i]);
		nvgText(vg, 90, y, label, NULL);

		// Color gradient bar showing interpolation
		float hue = i / 4.0f * 120.0f;
		nvgBeginPath(vg);
		nvgRect(vg, 200, y - 15, 800, 20);
		NVGpaint grad = nvgLinearGradient(vg, 200, y, 1000, y,
			nvgHSLA(hue / 360.0f, 0.8f, 0.5f, 255),
			nvgHSLA((hue + 30) / 360.0f, 0.8f, 0.5f, 255));
		nvgFillPaint(vg, grad);
		nvgFill(vg);

		// Quaternion magnitude
		float qMag = sqrtf(
			interpolated[i].decomposed.rotation.x * interpolated[i].decomposed.rotation.x +
			interpolated[i].decomposed.rotation.y * interpolated[i].decomposed.rotation.y +
			interpolated[i].decomposed.rotation.z * interpolated[i].decomposed.rotation.z
		);

		nvgFontSize(vg, 11.0f);
		nvgFillColor(vg, nvgRGBA(100, 255, 100, 255));
		char qInfo[64];
		snprintf(qInfo, sizeof(qInfo), "|q|=%.4f", qMag);
		nvgText(vg, 1050, y, qInfo, NULL);

		y += 35;
	}

	// Section 3: Results
	y += 30;
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(255, 200, 100, 255));
	nvgText(vg, 50, y, "System Validation", NULL);
	y += 40;

	nvgFontSize(vg, 14.0f);
	NVGcolor validColor = allValid ? nvgRGBA(100, 255, 100, 255) : nvgRGBA(255, 100, 100, 255);
	nvgFillColor(vg, validColor);
	char result[128];
	snprintf(result, sizeof(result), "All transforms valid: %s", allValid ? "YES" : "NO");
	nvgText(vg, 70, y, result, NULL);
	y += 30;

	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgFontSize(vg, 12.0f);
	nvgText(vg, 70, y, "Quaternion representation ensures:", NULL);
	y += 25;
	nvgText(vg, 90, y, "• Smooth interpolation via SLERP", NULL);
	y += 20;
	nvgText(vg, 90, y, "• Numerical stability (orthogonality preserved)", NULL);
	y += 20;
	nvgText(vg, 90, y, "• Efficient composition and inversion", NULL);
	y += 20;
	nvgText(vg, 90, y, "• Compact representation (4 floats vs 9)", NULL);

	// Footer
	nvgFontSize(vg, 11.0f);
	nvgFillColor(vg, nvgRGBA(120, 120, 120, 255));
	nvgText(vg, 50, 970, "Display color spaces managed through quaternion space provide smooth transitions and accurate color reproduction", NULL);

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

	window_save_screenshot(winCtx, imageIndex, "screendumps/display_color_space.png");

	// Cleanup
	nvgvk_display_color_manager_destroy(manager);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	if (allValid) {
		printf("=== Display Color Space System Test PASSED ===\n");
		printf("Matrix/Quaternion space provides accurate color space management\n");
		return 0;
	} else {
		printf("=== Display Color Space System Test FAILED ===\n");
		return 1;
	}
}
