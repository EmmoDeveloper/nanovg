#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

// Test: Traditional render pass with shapes, text, and gradients
// This mirrors the Java NanoVGVisualTest to help diagnose rendering issues
TEST(render_pass_visual_test)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Use INTERNAL_RENDER_PASS flag to use traditional render pass (not dynamic rendering)
	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_INTERNAL_RENDER_PASS);
	ASSERT_NOT_NULL(nvg);

	// Load a font for text rendering
	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("Warning: Failed to load font, text rendering will be skipped\n");
	}

	// Begin frame
	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Top-left: Stroked circle (animated in Java test)
	nvgBeginPath(nvg);
	nvgCircle(nvg, 100, 150, 80);
	nvgStrokeColor(nvg, nvgRGBA(255, 100, 100, 255));
	nvgStrokeWidth(nvg, 2.0f);
	nvgStroke(nvg);

	// Top-center: Rectangle with linear gradient
	nvgBeginPath(nvg);
	nvgRect(nvg, 250, 100, 200, 100);
	NVGpaint linearGrad = nvgLinearGradient(nvg, 250, 100, 450, 200,
	                                        nvgRGBA(0, 255, 200, 255),
	                                        nvgRGBA(0, 100, 150, 255));
	nvgFillPaint(nvg, linearGrad);
	nvgFill(nvg);

	// Bottom-left: Filled circle
	nvgBeginPath(nvg);
	nvgCircle(nvg, 120, 450, 40);
	nvgFillColor(nvg, nvgRGBA(100, 200, 255, 255));
	nvgFill(nvg);

	// Bottom-center: Rectangle with radial gradient
	nvgBeginPath(nvg);
	nvgRect(nvg, 250, 450, 150, 100);
	NVGpaint radialGrad = nvgRadialGradient(nvg, 325, 500, 10, 100,
	                                        nvgRGBA(150, 255, 150, 255),
	                                        nvgRGBA(0, 100, 0, 255));
	nvgFillPaint(nvg, radialGrad);
	nvgFill(nvg);

	// Bottom-right: Rounded rectangle
	nvgBeginPath(nvg);
	nvgRoundedRect(nvg, 400, 470, 120, 60, 10);
	nvgStrokeColor(nvg, nvgRGBA(200, 200, 200, 255));
	nvgStrokeWidth(nvg, 2.0f);
	nvgStroke(nvg);

	// Render text if font loaded
	if (font != -1) {
		nvgFontSize(nvg, 36.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		nvgText(nvg, 250, 300, "NanoVG Render Pass Test", NULL);
	}

	nvgEndFrame(nvg);

	printf("Test completed - check for rendering issues\n");
	printf("Expected: All shapes should be visible and properly filled/stroked\n");
	printf("Expected: Text should be visible if font loaded\n");
	printf("Expected: Gradients should show smooth color transitions\n");

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Simple shape with INTERNAL_RENDER_PASS
TEST(render_pass_simple_shape)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_INTERNAL_RENDER_PASS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Simple filled rectangle
	nvgBeginPath(nvg);
	nvgRect(nvg, 100, 100, 200, 150);
	nvgFillColor(nvg, nvgRGBA(255, 128, 0, 255));
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text rendering with INTERNAL_RENDER_PASS
TEST(render_pass_text)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_INTERNAL_RENDER_PASS);
	ASSERT_NOT_NULL(nvg);

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("Warning: Failed to load font\n");
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		SKIP_TEST("Font not available");
	}

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	nvgFontSize(nvg, 48.0f);
	nvgFontFace(nvg, "sans");
	nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
	nvgText(nvg, 100, 300, "Hello NanoVG!", NULL);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	RUN_TEST(test_render_pass_simple_shape);
	RUN_TEST(test_render_pass_text);
	RUN_TEST(test_render_pass_visual_test);

	print_test_summary();
	return test_exit_code();
}
