#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Begin/End frame
TEST(render_begin_end_frame)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Begin frame
	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// End frame (no drawing)
	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Rectangle rendering
TEST(render_rectangle)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw a rectangle
	nvgBeginPath(nvg);
	nvgRect(nvg, 100, 100, 300, 200);
	nvgFillColor(nvg, nvgRGBA(255, 192, 0, 255));
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Circle rendering
TEST(render_circle)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw a circle
	nvgBeginPath(nvg);
	nvgCircle(nvg, 400, 300, 50);
	nvgFillColor(nvg, nvgRGBA(0, 160, 255, 255));
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Stroke rendering
TEST(render_stroke)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw a stroked circle
	nvgBeginPath(nvg);
	nvgCircle(nvg, 400, 300, 50);
	nvgStrokeColor(nvg, nvgRGBA(255, 0, 0, 255));
	nvgStrokeWidth(nvg, 3.0f);
	nvgStroke(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple shapes in one frame
TEST(render_multiple_shapes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw rectangle
	nvgBeginPath(nvg);
	nvgRect(nvg, 50, 50, 100, 100);
	nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
	nvgFill(nvg);

	// Draw circle
	nvgBeginPath(nvg);
	nvgCircle(nvg, 250, 100, 50);
	nvgFillColor(nvg, nvgRGBA(0, 255, 0, 255));
	nvgFill(nvg);

	// Draw rounded rectangle
	nvgBeginPath(nvg);
	nvgRoundedRect(nvg, 350, 50, 100, 100, 10);
	nvgFillColor(nvg, nvgRGBA(0, 0, 255, 255));
	nvgFill(nvg);

	// Draw ellipse
	nvgBeginPath(nvg);
	nvgEllipse(nvg, 550, 100, 50, 30);
	nvgFillColor(nvg, nvgRGBA(255, 255, 0, 255));
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Linear gradient
TEST(render_linear_gradient)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw rectangle with linear gradient
	NVGpaint gradient = nvgLinearGradient(nvg, 100, 100, 400, 100,
	                                       nvgRGBA(255, 0, 0, 255),
	                                       nvgRGBA(0, 0, 255, 255));

	nvgBeginPath(nvg);
	nvgRect(nvg, 100, 100, 300, 200);
	nvgFillPaint(nvg, gradient);
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Radial gradient
TEST(render_radial_gradient)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Draw circle with radial gradient
	NVGpaint gradient = nvgRadialGradient(nvg, 400, 300, 10, 100,
	                                       nvgRGBA(255, 255, 255, 255),
	                                       nvgRGBA(0, 0, 0, 255));

	nvgBeginPath(nvg);
	nvgCircle(nvg, 400, 300, 100);
	nvgFillPaint(nvg, gradient);
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Transforms (translate, rotate, scale)
TEST(render_transforms)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Save state
	nvgSave(nvg);

	// Translate
	nvgTranslate(nvg, 200, 200);

	// Rotate
	nvgRotate(nvg, 0.785f); // 45 degrees

	// Scale
	nvgScale(nvg, 1.5f, 1.5f);

	// Draw transformed rectangle
	nvgBeginPath(nvg);
	nvgRect(nvg, -50, -50, 100, 100);
	nvgFillColor(nvg, nvgRGBA(255, 128, 0, 255));
	nvgFill(nvg);

	// Restore state
	nvgRestore(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple frames
TEST(render_multiple_frames)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Render 10 frames
	for (int i = 0; i < 10; i++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		// Draw animated circle (moves across screen)
		nvgBeginPath(nvg);
		nvgCircle(nvg, 100 + i * 60, 300, 30);
		nvgFillColor(nvg, nvgRGBA(255, 192, 0, 255));
		nvgFill(nvg);

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Scissor clipping
TEST(render_scissor)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Set scissor region
	nvgScissor(nvg, 100, 100, 200, 200);

	// Draw circle (will be clipped)
	nvgBeginPath(nvg);
	nvgCircle(nvg, 200, 200, 100);
	nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
	nvgFill(nvg);

	// Reset scissor
	nvgResetScissor(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Integration Tests: Render\n" COLOR_RESET);
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf("\n");

	// Run all tests
	RUN_TEST(test_render_begin_end_frame);
	RUN_TEST(test_render_rectangle);
	RUN_TEST(test_render_circle);
	RUN_TEST(test_render_stroke);
	RUN_TEST(test_render_multiple_shapes);
	RUN_TEST(test_render_linear_gradient);
	RUN_TEST(test_render_radial_gradient);
	RUN_TEST(test_render_transforms);
	RUN_TEST(test_render_multiple_frames);
	RUN_TEST(test_render_scissor);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
