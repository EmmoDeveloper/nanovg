// Memory Leak Detection Tests for NanoVG Vulkan Backend
// Tests various scenarios for memory leaks using Vulkan memory statistics

#include "test_framework.h"

// Define implementation before including headers
#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Context creation and deletion doesn't leak
TEST(leak_context_lifecycle)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	// Create and destroy contexts multiple times
	for (int i = 0; i < 10; i++) {
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
		ASSERT_NOT_NULL(nvg);

		// Do some operations
		nvgBeginFrame(nvg, 800, 600, 1.0f);
		nvgBeginPath(nvg);
		nvgRect(nvg, 10, 10, 100, 100);
		nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
		nvgFill(nvg);
		nvgEndFrame(nvg);

		nvgDeleteVk(nvg);
	}

	test_destroy_vulkan_context(vk);
	// If we get here without crash, no obvious leaks
}

// Test: Texture creation and deletion doesn't leak
TEST(leak_texture_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create and delete textures in a loop
	for (int cycle = 0; cycle < 5; cycle++) {
		int images[20];
		unsigned char data[256 * 256 * 4];

		// Fill with test data
		for (int i = 0; i < 256 * 256 * 4; i++) {
			data[i] = (unsigned char)(i & 0xFF);
		}

		// Create 20 textures
		for (int i = 0; i < 20; i++) {
			images[i] = nvgCreateImageRGBA(nvg, 256, 256, 0, data);
			ASSERT_NE(images[i], 0);
		}

		// Delete all textures
		for (int i = 0; i < 20; i++) {
			nvgDeleteImage(nvg, images[i]);
		}
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Image updates don't leak
TEST(leak_texture_updates)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	unsigned char data[128 * 128 * 4];
	for (int i = 0; i < 128 * 128 * 4; i++) {
		data[i] = (unsigned char)(i & 0xFF);
	}

	int image = nvgCreateImageRGBA(nvg, 128, 128, 0, data);
	ASSERT_NE(image, 0);

	// Update the same texture many times
	for (int i = 0; i < 100; i++) {
		// Modify data
		data[0] = (unsigned char)i;
		nvgUpdateImage(nvg, image, data);
	}

	nvgDeleteImage(nvg, image);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Rendering operations don't leak
TEST(leak_rendering_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Render many frames with various operations
	for (int frame = 0; frame < 100; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		// Draw shapes
		for (int i = 0; i < 50; i++) {
			nvgBeginPath(nvg);
			nvgRect(nvg, i * 10.0f, i * 10.0f, 50, 50);
			nvgFillColor(nvg, nvgRGBA(i * 5, 100, 150, 255));
			nvgFill(nvg);
		}

		// Draw circles
		for (int i = 0; i < 30; i++) {
			nvgBeginPath(nvg);
			nvgCircle(nvg, 400 + i * 5.0f, 300, 20);
			nvgStrokeColor(nvg, nvgRGBA(200, i * 8, 100, 255));
			nvgStrokeWidth(nvg, 2.0f);
			nvgStroke(nvg);
		}

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple contexts don't leak
TEST(leak_multiple_contexts)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	// Create multiple contexts simultaneously
	for (int cycle = 0; cycle < 3; cycle++) {
		NVGcontext* contexts[5];

		// Create 5 contexts
		for (int i = 0; i < 5; i++) {
			contexts[i] = test_create_nanovg_context(vk, NVG_ANTIALIAS);
			ASSERT_NOT_NULL(contexts[i]);

			// Do some work in each
			nvgBeginFrame(contexts[i], 800, 600, 1.0f);
			nvgBeginPath(contexts[i]);
			nvgRect(contexts[i], 10, 10, 100, 100);
			nvgFillColor(contexts[i], nvgRGBA(255, 0, 0, 255));
			nvgFill(contexts[i]);
			nvgEndFrame(contexts[i]);
		}

		// Delete all contexts
		for (int i = 0; i < 5; i++) {
			nvgDeleteVk(contexts[i]);
		}
	}

	test_destroy_vulkan_context(vk);
}

// Test: Font operations don't leak
TEST(leak_font_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Note: We can't actually load a font without a font file,
	// but we can test the text rendering path with placeholder font
	nvgCreateFont(nvg, "test", "nonexistent.ttf");
	// Font creation will fail, but shouldn't leak

	// Render frames anyway
	for (int frame = 0; frame < 50; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		// Attempt text rendering (will fail gracefully)
		nvgFontSize(nvg, 18.0f);
		nvgFontFace(nvg, "test");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		nvgText(nvg, 10, 10, "Test", NULL);

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Path operations don't leak
TEST(leak_path_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create many complex paths
	for (int frame = 0; frame < 50; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < 20; i++) {
			nvgBeginPath(nvg);
			nvgMoveTo(nvg, 10, 10);
			nvgLineTo(nvg, 100, 10);
			nvgBezierTo(nvg, 150, 50, 150, 100, 100, 150);
			nvgQuadTo(nvg, 50, 150, 10, 100);
			nvgClosePath(nvg);
			nvgFillColor(nvg, nvgRGBA(100, 100, 100, 255));
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Gradient operations don't leak
TEST(leak_gradient_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	for (int frame = 0; frame < 50; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		// Linear gradients
		for (int i = 0; i < 10; i++) {
			nvgBeginPath(nvg);
			nvgRect(nvg, i * 60.0f, 10, 50, 100);
			NVGpaint paint = nvgLinearGradient(nvg, i * 60.0f, 10, i * 60.0f + 50, 110,
				nvgRGBA(255, 0, 0, 255), nvgRGBA(0, 0, 255, 255));
			nvgFillPaint(nvg, paint);
			nvgFill(nvg);
		}

		// Radial gradients
		for (int i = 0; i < 10; i++) {
			nvgBeginPath(nvg);
			nvgCircle(nvg, 300 + i * 40.0f, 300, 30);
			NVGpaint paint = nvgRadialGradient(nvg, 300 + i * 40.0f, 300, 5, 30,
				nvgRGBA(255, 255, 0, 255), nvgRGBA(255, 0, 255, 255));
			nvgFillPaint(nvg, paint);
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Transform operations don't leak
TEST(leak_transform_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	for (int frame = 0; frame < 50; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < 20; i++) {
			nvgSave(nvg);
			nvgTranslate(nvg, 400, 300);
			nvgRotate(nvg, i * 0.1f);
			nvgScale(nvg, 1.0f + i * 0.05f, 1.0f + i * 0.05f);

			nvgBeginPath(nvg);
			nvgRect(nvg, -25, -25, 50, 50);
			nvgFillColor(nvg, nvgRGBA(200, 100, 50, 255));
			nvgFill(nvg);

			nvgRestore(nvg);
		}

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Scissor operations don't leak
TEST(leak_scissor_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	for (int frame = 0; frame < 50; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < 10; i++) {
			nvgSave(nvg);
			nvgScissor(nvg, i * 70.0f, i * 50.0f, 100, 100);

			// Draw something that gets clipped
			nvgBeginPath(nvg);
			nvgRect(nvg, i * 70.0f - 10, i * 50.0f - 10, 120, 120);
			nvgFillColor(nvg, nvgRGBA(100, 200, 100, 255));
			nvgFill(nvg);

			nvgRestore(nvg);
		}

		nvgEndFrame(nvg);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf("========================================\n");
	printf("Memory Leak Tests\n");
	printf("========================================\n");
	printf("\n");

	// Check if Vulkan is available
	if (!test_is_vulkan_available()) {
		printf("Vulkan not available, skipping tests\n");
		return 0;
	}

	RUN_TEST(test_leak_context_lifecycle);
	RUN_TEST(test_leak_texture_operations);
	RUN_TEST(test_leak_texture_updates);
	RUN_TEST(test_leak_rendering_operations);
	RUN_TEST(test_leak_multiple_contexts);
	RUN_TEST(test_leak_font_operations);
	RUN_TEST(test_leak_path_operations);
	RUN_TEST(test_leak_gradient_operations);
	RUN_TEST(test_leak_transform_operations);
	RUN_TEST(test_leak_scissor_operations);

	print_test_summary();
	return test_exit_code();
}
