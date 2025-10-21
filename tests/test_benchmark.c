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

// Benchmark: Simple shapes
TEST(benchmark_simple_shapes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 100;
	int shapes_per_frame = 100;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < shapes_per_frame; i++) {
			float x = (i % 10) * 80.0f;
			float y = (i / 10) * 60.0f;

			nvgBeginPath(nvg);
			nvgRect(nvg, x, y, 60, 40);
			nvgFillColor(nvg, nvgRGBA(255, 192, 0, 255));
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    Frames: %d\n", num_frames);
	printf("    Shapes per frame: %d\n", shapes_per_frame);
	printf("    Total time: %.2f ms\n", elapsed);
	printf("    Avg frame time: %.2f ms\n", avg_frame_time);
	printf("    FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Complex paths
TEST(benchmark_complex_paths)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 50;
	int paths_per_frame = 50;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < paths_per_frame; i++) {
			float x = (float)(i % 10) * 80.0f;
			float y = (float)(i / 10) * 60.0f;

			// Draw complex path with Bezier curves
			nvgBeginPath(nvg);
			nvgMoveTo(nvg, x, y);
			nvgBezierTo(nvg, x + 20, y + 10, x + 40, y + 30, x + 60, y + 20);
			nvgBezierTo(nvg, x + 40, y + 40, x + 20, y + 30, x, y + 40);
			nvgClosePath(nvg);
			nvgFillColor(nvg, nvgRGBA(0, 160, 255, 255));
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    Frames: %d\n", num_frames);
	printf("    Paths per frame: %d\n", paths_per_frame);
	printf("    Total time: %.2f ms\n", elapsed);
	printf("    Avg frame time: %.2f ms\n", avg_frame_time);
	printf("    FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Texture rendering
TEST(benchmark_textures)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create texture
	unsigned char data[64 * 64 * 4];
	for (int i = 0; i < 64 * 64 * 4; i++) {
		data[i] = (i % 256);
	}

	int img = nvgCreateImageRGBA(nvg, 64, 64, 0, data);
	ASSERT_NE(img, 0);

	int num_frames = 100;
	int quads_per_frame = 50;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < quads_per_frame; i++) {
			float x = (float)(i % 10) * 80.0f;
			float y = (float)(i / 10) * 60.0f;

			NVGpaint imgPaint = nvgImagePattern(nvg, x, y, 64, 64, 0, img, 1.0f);

			nvgBeginPath(nvg);
			nvgRect(nvg, x, y, 64, 64);
			nvgFillPaint(nvg, imgPaint);
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    Frames: %d\n", num_frames);
	printf("    Textured quads per frame: %d\n", quads_per_frame);
	printf("    Total time: %.2f ms\n", elapsed);
	printf("    Avg frame time: %.2f ms\n", avg_frame_time);
	printf("    FPS: %.1f\n", fps);

	nvgDeleteImage(nvg, img);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: Gradient fills
TEST(benchmark_gradients)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 100;
	int rects_per_frame = 50;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < rects_per_frame; i++) {
			float x = (float)(i % 10) * 80.0f;
			float y = (float)(i / 10) * 60.0f;

			NVGpaint gradient = nvgLinearGradient(nvg, x, y, x + 60, y + 40,
			                                       nvgRGBA(255, 0, 0, 255),
			                                       nvgRGBA(0, 0, 255, 255));

			nvgBeginPath(nvg);
			nvgRect(nvg, x, y, 60, 40);
			nvgFillPaint(nvg, gradient);
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    Frames: %d\n", num_frames);
	printf("    Gradient fills per frame: %d\n", rects_per_frame);
	printf("    Total time: %.2f ms\n", elapsed);
	printf("    Avg frame time: %.2f ms\n", avg_frame_time);
	printf("    FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Benchmark: State changes (save/restore)
TEST(benchmark_state_changes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 100;
	int changes_per_frame = 100;

	double start = get_time_ms();

	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);

		for (int i = 0; i < changes_per_frame; i++) {
			nvgSave(nvg);
			nvgTranslate(nvg, (float)i, (float)i);
			nvgRotate(nvg, (float)i * 0.01f);

			nvgBeginPath(nvg);
			nvgRect(nvg, 0, 0, 10, 10);
			nvgFillColor(nvg, nvgRGBA(255, 192, 0, 255));
			nvgFill(nvg);

			nvgRestore(nvg);
		}

		nvgEndFrame(nvg);
	}

	double end = get_time_ms();
	double elapsed = end - start;
	double avg_frame_time = elapsed / num_frames;
	double fps = 1000.0 / avg_frame_time;

	printf("    Frames: %d\n", num_frames);
	printf("    State changes per frame: %d\n", changes_per_frame);
	printf("    Total time: %.2f ms\n", elapsed);
	printf("    Avg frame time: %.2f ms\n", avg_frame_time);
	printf("    FPS: %.1f\n", fps);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Benchmark Tests\n" COLOR_RESET);
	printf(COLOR_CYAN "==========================================\n" COLOR_RESET);
	printf("\n");

	// Run all benchmarks
	RUN_TEST(test_benchmark_simple_shapes);
	RUN_TEST(test_benchmark_complex_paths);
	RUN_TEST(test_benchmark_textures);
	RUN_TEST(test_benchmark_gradients);
	RUN_TEST(test_benchmark_state_changes);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
