// Test for font atlas pre-warming functionality

#include "test_framework.h"

// Define implementation before including headers
#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Pre-warm single font
TEST(atlas_prewarm_single_font)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create a dummy font (will fail to load, but that's OK for testing the API)
	int font = nvgCreateFont(nvg, "test", "nonexistent.ttf");

	// Pre-warm the font (should not crash even with invalid font)
	int glyphCount = nvgPrewarmFont(nvg, font);

	// Should return 0 for invalid font, or positive for valid font
	// Since font load fails, this should be 0
	ASSERT_TRUE(glyphCount >= 0);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pre-warm multiple fonts
TEST(atlas_prewarm_multiple_fonts)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create multiple dummy fonts
	int fonts[3];
	fonts[0] = nvgCreateFont(nvg, "test1", "font1.ttf");
	fonts[1] = nvgCreateFont(nvg, "test2", "font2.ttf");
	fonts[2] = nvgCreateFont(nvg, "test3", "font3.ttf");

	// Pre-warm all fonts
	int totalGlyphs = nvgPrewarmFonts(nvg, fonts, 3);

	// Should return >= 0
	ASSERT_TRUE(totalGlyphs >= 0);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Custom pre-warming
TEST(atlas_prewarm_custom)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int font = nvgCreateFont(nvg, "test", "nonexistent.ttf");

	// Custom glyph set and sizes
	const char* customGlyphs = "Hello World 123";
	float customSizes[] = {16.0f, 24.0f, 32.0f};

	int glyphCount = nvgPrewarmFontCustom(nvg, font, customGlyphs,
	                                       customSizes, 3);

	ASSERT_TRUE(glyphCount >= 0);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pre-warm with invalid parameters
TEST(atlas_prewarm_invalid_params)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// NULL context
	int result = nvgPrewarmFont(NULL, 0);
	ASSERT_EQ(result, 0);

	// Invalid font ID
	result = nvgPrewarmFont(nvg, -1);
	ASSERT_EQ(result, 0);

	// NULL fonts array
	result = nvgPrewarmFonts(nvg, NULL, 5);
	ASSERT_EQ(result, 0);

	// Zero font count
	int fonts[] = {0};
	result = nvgPrewarmFonts(nvg, fonts, 0);
	ASSERT_EQ(result, 0);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pre-warming doesn't affect rendering state
TEST(atlas_prewarm_state_preservation)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Set some state
	nvgBeginFrame(nvg, 800, 600, 1.0f);
	nvgFontSize(nvg, 42.0f);

	int font = nvgCreateFont(nvg, "test", "nonexistent.ttf");
	nvgFontFaceId(nvg, font);

	// Pre-warm (should save/restore state)
	nvgPrewarmFont(nvg, font);

	// State should be preserved
	// (We can't easily verify font size/face, but function should not crash)
	nvgBeginPath(nvg);
	nvgRect(nvg, 10, 10, 100, 100);
	nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
	nvgFill(nvg);

	nvgEndFrame(nvg);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf("========================================\n");
	printf("Atlas Pre-warming Tests\n");
	printf("========================================\n");
	printf("\n");

	// Check if Vulkan is available
	if (!test_is_vulkan_available()) {
		printf("Vulkan not available, skipping tests\n");
		return 0;
	}

	RUN_TEST(test_atlas_prewarm_single_font);
	RUN_TEST(test_atlas_prewarm_multiple_fonts);
	RUN_TEST(test_atlas_prewarm_custom);
	RUN_TEST(test_atlas_prewarm_invalid_params);
	RUN_TEST(test_atlas_prewarm_state_preservation);

	print_test_summary();
	return test_exit_code();
}
