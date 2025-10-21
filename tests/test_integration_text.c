#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Font loading
TEST(text_font_loading)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Note: This test just verifies the API doesn't crash
	// Actual font loading requires font files
	nvgBeginFrame(nvg, 800, 600, 1.0f);
	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text rendering with different sizes
TEST(text_different_sizes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Note: Without actual fonts loaded, this won't render anything
	// but it should not crash
	float sizes[] = {12.0f, 16.0f, 24.0f, 36.0f, 48.0f};

	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		nvgFontSize(nvg, sizes[i]);
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		// Would call nvgText() here if fonts were loaded
	}

	nvgEndFrame(nvg);

	printf("    Tested %zu different font sizes\n", sizeof(sizes) / sizeof(sizes[0]));

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text rendering modes
TEST(text_rendering_modes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Test standard mode
	{
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	// Test with SDF text
	{
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_SDF_TEXT);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	// Test with MSDF text
	{
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_MSDF_TEXT);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	// Test with subpixel text
	{
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_SUBPIXEL_TEXT);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	// Test with color text (emoji)
	{
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_COLOR_TEXT);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	printf("    Created contexts with all text rendering modes\n");

	test_destroy_vulkan_context(vk);
}

// Test: Text alignment modes
TEST(text_alignment)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Test different alignments
	int alignments[] = {
		NVG_ALIGN_LEFT,
		NVG_ALIGN_CENTER,
		NVG_ALIGN_RIGHT,
		NVG_ALIGN_TOP,
		NVG_ALIGN_MIDDLE,
		NVG_ALIGN_BOTTOM,
		NVG_ALIGN_LEFT | NVG_ALIGN_TOP,
		NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
		NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM
	};

	for (size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
		nvgTextAlign(nvg, alignments[i]);
		// Would call nvgText() here if fonts were loaded
	}

	nvgEndFrame(nvg);

	printf("    Tested %zu text alignment modes\n", sizeof(alignments) / sizeof(alignments[0]));

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text metrics
TEST(text_metrics)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Test font metrics API
	nvgFontSize(nvg, 18.0f);
	nvgFontFace(nvg, "sans"); // Will fail without font, but shouldn't crash

	float ascender, descender, lineh;
	nvgTextMetrics(nvg, &ascender, &descender, &lineh);

	// Without loaded fonts, metrics might be zero
	printf("    Text metrics: ascender=%.2f, descender=%.2f, lineh=%.2f\n",
	       ascender, descender, lineh);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text with transformations
TEST(text_transformations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Test text with various transformations
	for (int i = 0; i < 5; i++) {
		nvgSave(nvg);

		nvgTranslate(nvg, 100 + i * 100, 100 + i * 50);
		nvgRotate(nvg, i * 0.3f);
		nvgScale(nvg, 1.0f + i * 0.2f, 1.0f + i * 0.2f);

		nvgFontSize(nvg, 18.0f);
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		// Would call nvgText() here if fonts were loaded

		nvgRestore(nvg);
	}

	nvgEndFrame(nvg);

	printf("    Applied transformations to text rendering\n");

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple text rendering calls
TEST(text_multiple_calls)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 10;
	int text_calls_per_frame = 50;

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < text_calls_per_frame; i++) {
			float x = (float)(i % 10) * 80.0f;
			float y = (float)(i / 10) * 60.0f;

			nvgFontSize(nvg, 16.0f);
			nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
			// Would call nvgText(nvg, x, y, "Text", NULL) here if fonts were loaded
		}

		nvgEndFrame(nvg);
	}

	printf("    Rendered %d frames with %d text calls each\n",
	       num_frames, text_calls_per_frame);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text blur effect
TEST(text_blur_effect)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Test different blur levels
	float blur_values[] = {0.0f, 1.0f, 2.0f, 5.0f, 10.0f};

	for (size_t i = 0; i < sizeof(blur_values) / sizeof(blur_values[0]); i++) {
		nvgFontBlur(nvg, blur_values[i]);
		nvgFontSize(nvg, 18.0f);
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		// Would call nvgText() here if fonts were loaded
	}

	nvgEndFrame(nvg);

	printf("    Tested %zu blur levels\n", sizeof(blur_values) / sizeof(blur_values[0]));

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text with different colors
TEST(text_colors)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	nvgBeginFrame(nvg, 800, 600, 1.0f);

	// Test different colors
	NVGcolor colors[] = {
		nvgRGBA(255, 0, 0, 255),
		nvgRGBA(0, 255, 0, 255),
		nvgRGBA(0, 0, 255, 255),
		nvgRGBA(255, 255, 0, 255),
		nvgRGBA(255, 0, 255, 255),
		nvgRGBA(0, 255, 255, 255),
		nvgRGBA(255, 255, 255, 255),
		nvgRGBA(128, 128, 128, 255),
		nvgRGBA(255, 128, 0, 255),
		nvgRGBA(128, 0, 255, 128) // Semi-transparent
	};

	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
		nvgFontSize(nvg, 18.0f);
		nvgFillColor(nvg, colors[i]);
		// Would call nvgText() here if fonts were loaded
	}

	nvgEndFrame(nvg);

	printf("    Tested %zu different text colors\n", sizeof(colors) / sizeof(colors[0]));

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Integration Tests: Text\n" COLOR_RESET);
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf("\n");
	printf(COLOR_YELLOW "Note: These tests validate text API without actual fonts.\n" COLOR_RESET);
	printf(COLOR_YELLOW "      Full text rendering requires font file loading.\n" COLOR_RESET);
	printf("\n");

	// Run all tests
	RUN_TEST(test_text_font_loading);
	RUN_TEST(test_text_different_sizes);
	RUN_TEST(test_text_rendering_modes);
	RUN_TEST(test_text_alignment);
	RUN_TEST(test_text_metrics);
	RUN_TEST(test_text_transformations);
	RUN_TEST(test_text_multiple_calls);
	RUN_TEST(test_text_blur_effect);
	RUN_TEST(test_text_colors);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
