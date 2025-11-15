#!/usr/bin/env python3
"""
Generate comprehensive API coverage tests for NanoVG.
Creates one test file per function/parameter combination.
"""

import os
import re
import math

# Define all NanoVG API functions with their parameter variants
API_TESTS = {
	# Frame management
	"nvgBeginFrame": [
		{"width": 800, "height": 600, "ratio": 1.0},
		{"width": 1920, "height": 1080, "ratio": 2.0},
		{"width": 400, "height": 300, "ratio": 0.5},
	],

	# State management
	"nvgSave": [{}],
	"nvgRestore": [{}],
	"nvgReset": [{}],

	# Composite operations
	"nvgGlobalCompositeOperation": [
		{"op": "NVG_SOURCE_OVER"},
		{"op": "NVG_SOURCE_IN"},
		{"op": "NVG_SOURCE_OUT"},
		{"op": "NVG_ATOP"},
		{"op": "NVG_DESTINATION_OVER"},
		{"op": "NVG_DESTINATION_IN"},
		{"op": "NVG_DESTINATION_OUT"},
		{"op": "NVG_DESTINATION_ATOP"},
		{"op": "NVG_LIGHTER"},
		{"op": "NVG_COPY"},
		{"op": "NVG_XOR"},
	],

	"nvgGlobalCompositeBlendFunc": [
		{"sfactor": "NVG_ONE", "dfactor": "NVG_ZERO"},
		{"sfactor": "NVG_SRC_ALPHA", "dfactor": "NVG_ONE_MINUS_SRC_ALPHA"},
	],

	"nvgGlobalCompositeBlendFuncSeparate": [
		{"srcRGB": "NVG_ONE", "dstRGB": "NVG_ZERO", "srcAlpha": "NVG_ONE", "dstAlpha": "NVG_ZERO"},
	],

	"nvgGlobalAlpha": [
		{"alpha": 0.0},
		{"alpha": 0.25},
		{"alpha": 0.5},
		{"alpha": 0.75},
		{"alpha": 1.0},
	],

	# Shape rendering
	"nvgShapeAntiAlias": [
		{"enabled": 0},
		{"enabled": 1},
	],

	"nvgStrokeWidth": [
		{"size": 1.0},
		{"size": 2.0},
		{"size": 5.0},
		{"size": 10.0},
	],

	"nvgLineCap": [
		{"cap": "NVG_BUTT"},
		{"cap": "NVG_ROUND"},
		{"cap": "NVG_SQUARE"},
	],

	"nvgLineJoin": [
		{"join": "NVG_MITER"},
		{"join": "NVG_ROUND"},
		{"join": "NVG_BEVEL"},
	],

	"nvgMiterLimit": [
		{"limit": 4.0},
		{"limit": 10.0},
	],

	# Transforms
	"nvgResetTransform": [{}],

	"nvgTransform": [
		{"a": 1.0, "b": 0.0, "c": 0.0, "d": 1.0, "e": 0.0, "f": 0.0},
		{"a": 1.5, "b": 0.0, "c": 0.0, "d": 1.5, "e": 100.0, "f": 50.0},
	],

	"nvgTranslate": [
		{"x": 0, "y": 0},
		{"x": 100, "y": 50},
		{"x": -50, "y": 100},
	],

	"nvgRotate": [
		{"angle": 0.0},
		{"angle": 0.785},  # 45 degrees
		{"angle": 1.57},   # 90 degrees
		{"angle": 3.14},   # 180 degrees
	],

	"nvgScale": [
		{"x": 1.0, "y": 1.0},
		{"x": 0.5, "y": 0.5},
		{"x": 2.0, "y": 2.0},
		{"x": 1.0, "y": 2.0},
	],

	"nvgSkewX": [
		{"angle": 0.0},
		{"angle": 0.1},
		{"angle": 0.5},
	],

	"nvgSkewY": [
		{"angle": 0.0},
		{"angle": 0.1},
		{"angle": 0.5},
	],

	# Scissoring
	"nvgScissor": [
		{"x": 0, "y": 0, "w": 100, "h": 100},
		{"x": 50, "y": 50, "w": 200, "h": 150},
	],

	"nvgIntersectScissor": [
		{"x": 25, "y": 25, "w": 150, "h": 150},
	],

	"nvgResetScissor": [{}],

	# Paths
	"nvgBeginPath": [{}],

	"nvgMoveTo": [
		{"x": 10, "y": 10},
		{"x": 100, "y": 100},
	],

	"nvgLineTo": [
		{"x": 100, "y": 10},
		{"x": 10, "y": 100},
	],

	"nvgBezierTo": [
		{"c1x": 50, "c1y": 10, "c2x": 100, "c2y": 50, "x": 100, "y": 100},
	],

	"nvgQuadTo": [
		{"cx": 150, "cy": 10, "x": 200, "y": 100},
	],

	"nvgArcTo": [
		{"x1": 100, "y1": 200, "x2": 10, "y2": 200, "radius": 10},
	],

	"nvgClosePath": [{}],

	"nvgPathWinding": [
		{"dir": "NVG_CCW"},
		{"dir": "NVG_CW"},
	],

	# Shapes
	"nvgArc": [
		{"cx": 100, "cy": 100, "r": 50, "a0": 0, "a1": 3.14, "dir": "NVG_CW"},
		{"cx": 200, "cy": 100, "r": 30, "a0": 0, "a1": 6.28, "dir": "NVG_CCW"},
	],

	"nvgRect": [
		{"x": 10, "y": 10, "w": 100, "h": 50},
		{"x": 150, "y": 10, "w": 80, "h": 80},
	],

	"nvgRoundedRect": [
		{"x": 10, "y": 100, "w": 100, "h": 50, "r": 5},
		{"x": 150, "y": 100, "w": 80, "h": 80, "r": 20},
	],

	"nvgRoundedRectVarying": [
		{"x": 10, "y": 200, "w": 100, "h": 50, "radTopLeft": 5, "radTopRight": 10, "radBottomRight": 15, "radBottomLeft": 20},
	],

	"nvgEllipse": [
		{"cx": 300, "cy": 100, "rx": 50, "ry": 30},
		{"cx": 400, "cy": 100, "rx": 30, "ry": 50},
	],

	"nvgCircle": [
		{"cx": 500, "cy": 100, "r": 40},
		{"cx": 600, "cy": 100, "r": 20},
	],

	# Fill and Stroke
	"nvgFill": [{}],
	"nvgStroke": [{}],

	# Font hinting
	"nvgFontHinting": [
		{"hinting": "NVG_HINTING_NONE"},
		{"hinting": "NVG_HINTING_LIGHT"},
		{"hinting": "NVG_HINTING_FULL"},
	],

	# Text direction
	"nvgTextDirection": [
		{"direction": "NVG_TEXT_DIR_AUTO"},
		{"direction": "NVG_TEXT_DIR_LTR"},
		{"direction": "NVG_TEXT_DIR_RTL"},
	],

	# Font features
	"nvgFontFeature": [
		{"tag": "NVG_FEATURE_LIGA", "enabled": 1},
		{"tag": "NVG_FEATURE_CALT", "enabled": 1},
		{"tag": "NVG_FEATURE_ZERO", "enabled": 0},
	],

	"nvgFontFeaturesReset": [{}],

	# Font size and style
	"nvgFontSize": [
		{"size": 12.0},
		{"size": 18.0},
		{"size": 24.0},
		{"size": 36.0},
	],

	"nvgFontBlur": [
		{"blur": 0.0},
		{"blur": 1.0},
		{"blur": 2.0},
	],

	"nvgTextLetterSpacing": [
		{"spacing": 0.0},
		{"spacing": 1.0},
		{"spacing": -0.5},
	],

	"nvgTextLineHeight": [
		{"lineHeight": 1.0},
		{"lineHeight": 1.5},
		{"lineHeight": 2.0},
	],

	"nvgTextAlign": [
		{"align": "NVG_ALIGN_LEFT | NVG_ALIGN_TOP"},
		{"align": "NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE"},
		{"align": "NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM"},
	],

	# Subpixel rendering
	"nvgSubpixelText": [
		{"enabled": 0},
		{"enabled": 1},
	],

	"nvgTextSubpixelMode": [
		{"mode": 0},  # NONE
		{"mode": 1},  # H_RGB
		{"mode": 2},  # H_BGR
	],

	# Baseline and kerning
	"nvgBaselineShift": [
		{"offset": 0.0},
		{"offset": 5.0},
		{"offset": -5.0},
	],

	"nvgKerningEnabled": [
		{"enabled": 0},
		{"enabled": 1},
	],
}

def format_value(value):
	"""Format a parameter value for C code."""
	if isinstance(value, str):
		return value
	elif isinstance(value, float):
		return f"{value}f"
	else:
		return str(value)

def get_test_code(func_name, params):
	"""Generate appropriate test code for each function category."""

	# Shape drawing functions - draw the shape being tested
	shape_funcs = ['nvgRect', 'nvgRoundedRect', 'nvgRoundedRectVarying', 'nvgCircle', 'nvgEllipse', 'nvgArc']

	# Path building functions - create a simple path to demonstrate
	path_funcs = ['nvgMoveTo', 'nvgLineTo', 'nvgBezierTo', 'nvgQuadTo', 'nvgArcTo']

	# State functions - show effect with a visible shape
	state_funcs = ['nvgTranslate', 'nvgRotate', 'nvgScale', 'nvgSkewX', 'nvgSkewY', 'nvgTransform',
	               'nvgGlobalAlpha', 'nvgStrokeWidth', 'nvgLineCap', 'nvgLineJoin', 'nvgMiterLimit']

	# Fill/stroke functions
	fill_stroke = ['nvgFill', 'nvgStroke']

	# Text functions
	text_funcs = ['nvgFontSize', 'nvgFontBlur', 'nvgTextLetterSpacing', 'nvgTextLineHeight',
	              'nvgTextAlign', 'nvgFontHinting', 'nvgTextDirection', 'nvgSubpixelText',
	              'nvgTextSubpixelMode', 'nvgBaselineShift', 'nvgKerningEnabled', 'nvgFontFeature']

	param_call = []
	for key, value in params.items():
		param_call.append(format_value(value))

	if param_call:
		func_call = f"{func_name}(vg, {', '.join(param_call)});"
	else:
		func_call = f"{func_name}(vg);"

	# Generate appropriate test visualization
	if func_name in shape_funcs:
		# Just draw the shape being tested
		return f"""	nvgBeginPath(vg);
	{func_call}
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);"""

	elif func_name in path_funcs:
		# Create a path demonstrating the function
		return f"""	nvgBeginPath(vg);
	nvgMoveTo(vg, 100, 100);
	{func_call}
	nvgStrokeColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgStrokeWidth(vg, 3.0f);
	nvgStroke(vg);"""

	elif func_name == 'nvgFill':
		return f"""	nvgBeginPath(vg);
	nvgCircle(vg, 200, 200, 100);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	{func_call}"""

	elif func_name == 'nvgStroke':
		return f"""	nvgBeginPath(vg);
	nvgCircle(vg, 200, 200, 100);
	nvgStrokeColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgStrokeWidth(vg, 5.0f);
	{func_call}"""

	elif func_name in state_funcs:
		# Apply state then draw a rectangle to show the effect
		return f"""	{func_call}
	nvgBeginPath(vg);
	nvgRect(vg, 100, 100, 150, 100);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);"""

	elif func_name in text_funcs:
		# Text rendering functions
		return f"""	{func_call}
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgText(vg, 100, 300, "Test Text", NULL);"""

	else:
		# Generic test - just call the function
		return f"""	{func_call}
	nvgBeginPath(vg);
	nvgRect(vg, 100, 100, 150, 100);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);"""

def generate_test_file(func_name, variant_idx, params):
	"""Generate a single test file for a function/parameter combination."""

	test_name = f"test_{func_name.replace('nvg', '').lower()}_{variant_idx:03d}"
	filename = f"tests/{test_name}.c"

	# Get appropriate test code for this function
	test_code = get_test_code(func_name, params)

	# Generate C code
	code = f'''#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "../src/tools/window_utils.h"
#include <stdio.h>

// Test: {func_name} with parameters: {params}

int main(void)
{{
	printf("=== Testing {func_name} variant {variant_idx} ===\\n");

	WindowVulkanContext* winCtx = window_create_context(800, 600, "{func_name} Test");
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
	VkCommandBufferBeginInfo beginInfo = {{0}};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {{0}};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{{{0.2f, 0.2f, 0.2f, 1.0f}}}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){{1.0f, 0}};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = {{0, 0, (float)winCtx->swapchainExtent.width, (float)winCtx->swapchainExtent.height, 0, 1}};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{{{0, 0}}, winCtx->swapchainExtent}};
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, winCtx->swapchainExtent.width, winCtx->swapchainExtent.height, 1.0f);

	// Test the API function
{test_code}

	// Draw label
	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 10, 20, "{func_name} - variant {variant_idx}", NULL);

	nvgEndFrame(vg);

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submitInfo = {{0}};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSem[] = {{winCtx->imageAvailableSemaphores[winCtx->currentFrame]}};
	VkPipelineStageFlags waitStages[] = {{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSem;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSem[] = {{winCtx->renderFinishedSemaphores[winCtx->currentFrame]}};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSem;

	vkResetFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame]);
	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);
	vkWaitForFences(winCtx->device, 1, &winCtx->inFlightFences[winCtx->currentFrame], VK_TRUE, UINT64_MAX);

	window_save_screenshot(winCtx, imageIndex, "screendumps/{test_name}.ppm");

	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("Test PASSED: {func_name} variant {variant_idx}\\n");
	return 0;
}}
'''

	return filename, code

def main():
	"""Generate all test files."""
	os.makedirs("tests", exist_ok=True)

	test_files = []
	total_tests = 0

	for func_name, variants in API_TESTS.items():
		for idx, params in enumerate(variants):
			filename, code = generate_test_file(func_name, idx, params)

			with open(filename, 'w') as f:
				f.write(code)

			test_files.append(filename)
			total_tests += 1
			print(f"Generated: {filename}")

	print(f"\nTotal tests generated: {total_tests}")
	print(f"Test files: {len(test_files)}")

	# Generate master test runner
	runner_code = '''#!/bin/bash
# Run all generated API coverage tests

PASSED=0
FAILED=0

'''

	for test_file in test_files:
		test_name = os.path.basename(test_file).replace('.c', '')
		runner_code += f'''
echo "Running {test_name}..."
make build/{test_name} && ./build/{test_name}
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: {test_name}"
fi
'''

	runner_code += '''
echo ""
echo "===================="
echo "Tests passed: $PASSED"
echo "Tests failed: $FAILED"
echo "Total: $((PASSED + FAILED))"
'''

	with open("run_coverage_tests.sh", 'w') as f:
		f.write(runner_code)

	os.chmod("run_coverage_tests.sh", 0o755)
	print("Generated test runner: run_coverage_tests.sh")

if __name__ == "__main__":
	main()
