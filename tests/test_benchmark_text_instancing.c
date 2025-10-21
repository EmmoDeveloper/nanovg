// Benchmark for instanced vs non-instanced text rendering

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>
#include <time.h>

// Simple timing utility
static double get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

// Benchmark: Text rendering with instancing
TEST(benchmark_text_instanced)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// Ensure instancing is enabled
	vknvg->useTextInstancing = VK_TRUE;

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("    Warning: Failed to load font\n");
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		SKIP_TEST("Font not available");
	}

	int num_frames = 1000;
	int glyphs_per_frame = 50;

	double start = get_time_ms();

	int totalGlyphs = 0;
	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		nvgFontSize(nvg, 18.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));

		// Render multiple text strings to generate many glyphs
		for (int i = 0; i < glyphs_per_frame / 20; i++) {
			float y = 20.0f + i * 25.0f;
			nvgText(nvg, 10, y, "Hello World! Testing instanced rendering.", NULL);
		}

		// Track actual glyphs rendered (last frame, before endFrame)
		if (frame == num_frames - 1 && vknvg->glyphInstanceBuffer) {
			totalGlyphs = vknvg->glyphInstanceBuffer->count;
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    INSTANCED MODE:\n");
	printf("      Frames: %d\n", num_frames);
	printf("      Glyphs per frame: ~%d\n", glyphs_per_frame);
	printf("      Actual glyphs rendered (last frame): %d\n", totalGlyphs);
	printf("      Total time: %.2f ms\n", elapsed);
	printf("      Avg frame time: %.2f ms\n", avg_frame_time);
	printf("      FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Text rendering without instancing
TEST(benchmark_text_non_instanced)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// Disable instancing for comparison
	vknvg->useTextInstancing = VK_FALSE;

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("    Warning: Failed to load font\n");
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		SKIP_TEST("Font not available");
	}

	int num_frames = 1000;
	int glyphs_per_frame = 50;

	double start = get_time_ms();

	int totalVertices = 0;
	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		nvgFontSize(nvg, 18.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));

		// Render same text strings as instanced test
		for (int i = 0; i < glyphs_per_frame / 20; i++) {
			float y = 20.0f + i * 25.0f;
			nvgText(nvg, 10, y, "Hello World! Testing instanced rendering.", NULL);
		}

		// Track actual vertices rendered (last frame, before endFrame)
		if (frame == num_frames - 1) {
			totalVertices = vknvg->nverts;
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    NON-INSTANCED MODE:\n");
	printf("      Frames: %d\n", num_frames);
	printf("      Glyphs per frame: ~%d\n", glyphs_per_frame);
	printf("      Actual vertices (last frame): %d\n", totalVertices);
	printf("      Total time: %.2f ms\n", elapsed);
	printf("      Avg frame time: %.2f ms\n", avg_frame_time);
	printf("      FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Heavy text load with instancing
TEST(benchmark_heavy_text_instanced)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;
	vknvg->useTextInstancing = VK_TRUE;

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("    Warning: Failed to load font\n");
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		SKIP_TEST("Font not available");
	}

	int num_frames = 1000;
	int glyphs_per_frame = 200;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		nvgFontSize(nvg, 14.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));

		for (int i = 0; i < glyphs_per_frame / 20; i++) {
			float y = 10.0f + (i % 40) * 15.0f;
			float x = (i / 40) * 200.0f;
			nvgText(nvg, x, y, "The quick brown fox!", NULL);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    HEAVY LOAD - INSTANCED:\n");
	printf("      Frames: %d\n", num_frames);
	printf("      Glyphs per frame: ~%d\n", glyphs_per_frame);
	printf("      Total time: %.2f ms\n", elapsed);
	printf("      Avg frame time: %.3f ms\n", avg_frame_time);
	printf("      FPS: %.1f\n", fps);
	if (vknvg->glyphInstanceBuffer) {
		printf("      Instance buffer used: %d/%d instances\n",
		       vknvg->glyphInstanceBuffer->count, vknvg->glyphInstanceBuffer->capacity);
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Heavy text load without instancing
TEST(benchmark_heavy_text_non_instanced)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;
	vknvg->useTextInstancing = VK_FALSE;

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		printf("    Warning: Failed to load font\n");
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		SKIP_TEST("Font not available");
	}

	int num_frames = 1000;
	int glyphs_per_frame = 200;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		nvgFontSize(nvg, 14.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));

		for (int i = 0; i < glyphs_per_frame / 20; i++) {
			float y = 10.0f + (i % 40) * 15.0f;
			float x = (i / 40) * 200.0f;
			nvgText(nvg, x, y, "The quick brown fox!", NULL);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    HEAVY LOAD - NON-INSTANCED:\n");
	printf("      Frames: %d\n", num_frames);
	printf("      Glyphs per frame: ~%d\n", glyphs_per_frame);
	printf("      Total time: %.2f ms\n", elapsed);
	printf("      Avg frame time: %.3f ms\n", avg_frame_time);
	printf("      FPS: %.1f\n", fps);
	printf("      Vertices generated: %d\n", vknvg->nverts);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("==========================================\n");
	printf("  Text Instancing Performance Benchmark\n");
	printf("==========================================\n\n");

	printf("Comparing instanced vs non-instanced text rendering...\n\n");

	printf("Test 1: Normal Load (500 glyphs/frame)\n");
	printf("----------------------------------------\n");
	RUN_TEST(test_benchmark_text_instanced);
	printf("\n");
	RUN_TEST(test_benchmark_text_non_instanced);
	printf("\n");

	printf("Test 2: Heavy Load (2000 glyphs/frame)\n");
	printf("----------------------------------------\n");
	RUN_TEST(test_benchmark_heavy_text_instanced);
	printf("\n");
	RUN_TEST(test_benchmark_heavy_text_non_instanced);
	printf("\n");

	print_test_summary();
	return test_exit_code();
}
