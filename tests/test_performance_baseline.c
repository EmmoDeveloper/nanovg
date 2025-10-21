// Performance baseline benchmark for text rendering
// Measures performance with all optimizations ON vs OFF

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static double get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

typedef struct {
	double totalTime;
	double avgFrameTime;
	double fps;
	int glyphsRendered;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t evictions;
} BenchmarkResult;

static BenchmarkResult benchmark_text_rendering(
	int useVirtualAtlas,
	int useInstancing,
	int usePrewarm,
	int numFrames,
	int glyphsPerFrame)
{
	BenchmarkResult result = {0};

	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		return result;
	}

	int flags = NVG_ANTIALIAS;
	if (useVirtualAtlas) {
		flags |= NVG_VIRTUAL_ATLAS;
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, flags);
	if (!nvg) {
		test_destroy_vulkan_context(vk);
		return result;
	}

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// Control instancing
	vknvg->useTextInstancing = useInstancing ? VK_TRUE : VK_FALSE;

	int font = nvgCreateFont(nvg, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font == -1) {
		nvgDeleteVk(nvg);
		test_destroy_vulkan_context(vk);
		return result;
	}

	// Pre-warm if requested
	if (usePrewarm) {
		nvgPrewarmFont(nvg, font);
	}

	// Benchmark loop
	double start = get_time_ms();

	for (int frame = 0; frame < numFrames; frame++) {
		nvgBeginFrame(nvg, 1920, 1080, 1.0f);
		nvgFontSize(nvg, 14.0f);
		nvgFontFace(nvg, "sans");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));

		// Render text
		int linesPerFrame = glyphsPerFrame / 50;  // ~50 chars per line
		for (int i = 0; i < linesPerFrame; i++) {
			char buf[256];
			snprintf(buf, sizeof(buf), "Line %d: The quick brown fox jumps over the lazy dog.", i);
			nvgText(nvg, 10, 20 + i * 16, buf, NULL);
		}

		// Capture metrics on last frame
		if (frame == numFrames - 1) {
			if (useInstancing && vknvg->glyphInstanceBuffer) {
				result.glyphsRendered = vknvg->glyphInstanceBuffer->count;
			} else {
				result.glyphsRendered = vknvg->nverts / 6;  // Estimate from vertices
			}

			if (useVirtualAtlas && vknvg->virtualAtlas) {
				uint32_t uploads;
				vknvg__getAtlasStats(vknvg->virtualAtlas,
				                      &result.cacheHits,
				                      &result.cacheMisses,
				                      &result.evictions,
				                      &uploads);
			}
		}

		nvgEndFrame(nvg);

		// Reset instance buffer if using instancing
		if (useInstancing && vknvg->glyphInstanceBuffer) {
			vknvg__resetGlyphInstances(vknvg->glyphInstanceBuffer);
		}
	}

	double end = get_time_ms();
	result.totalTime = end - start;
	result.avgFrameTime = result.totalTime / numFrames;
	result.fps = 1000.0 / result.avgFrameTime;

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);

	return result;
}

TEST(baseline_no_optimizations)
{
	printf("    Running baseline (NO optimizations)...\n");
	BenchmarkResult result = benchmark_text_rendering(
		0,    // no virtual atlas
		0,    // no instancing
		0,    // no prewarm
		500,  // frames
		500   // glyphs per frame
	);

	if (result.totalTime == 0) {
		SKIP_TEST("Benchmark failed to run");
	}

	printf("      Total time: %.2f ms\n", result.totalTime);
	printf("      Avg frame time: %.3f ms\n", result.avgFrameTime);
	printf("      FPS: %.1f\n", result.fps);
	printf("      Glyphs rendered: %d\n", result.glyphsRendered);
}

TEST(optimized_instancing_only)
{
	printf("    Running with INSTANCING only...\n");
	BenchmarkResult result = benchmark_text_rendering(
		0,    // no virtual atlas
		1,    // YES instancing
		0,    // no prewarm
		500,
		500
	);

	if (result.totalTime == 0) {
		SKIP_TEST("Benchmark failed to run");
	}

	printf("      Total time: %.2f ms\n", result.totalTime);
	printf("      Avg frame time: %.3f ms\n", result.avgFrameTime);
	printf("      FPS: %.1f\n", result.fps);
	printf("      Glyphs rendered: %d\n", result.glyphsRendered);
}

TEST(optimized_virtual_atlas_only)
{
	printf("    Running with VIRTUAL ATLAS only...\n");
	BenchmarkResult result = benchmark_text_rendering(
		1,    // YES virtual atlas
		0,    // no instancing
		0,    // no prewarm
		500,
		500
	);

	if (result.totalTime == 0) {
		SKIP_TEST("Benchmark failed to run");
	}

	printf("      Total time: %.2f ms\n", result.totalTime);
	printf("      Avg frame time: %.3f ms\n", result.avgFrameTime);
	printf("      FPS: %.1f\n", result.fps);
	printf("      Cache hits: %d, misses: %d\n", result.cacheHits, result.cacheMisses);
	printf("      Hit rate: %.1f%%\n",
	       100.0 * result.cacheHits / (result.cacheHits + result.cacheMisses + 0.001));
}

TEST(optimized_all_enabled)
{
	printf("    Running with ALL OPTIMIZATIONS...\n");
	BenchmarkResult result = benchmark_text_rendering(
		1,    // YES virtual atlas
		1,    // YES instancing
		1,    // YES prewarm
		500,
		500
	);

	if (result.totalTime == 0) {
		SKIP_TEST("Benchmark failed to run");
	}

	printf("      Total time: %.2f ms\n", result.totalTime);
	printf("      Avg frame time: %.3f ms\n", result.avgFrameTime);
	printf("      FPS: %.1f\n", result.fps);
	printf("      Glyphs rendered: %d\n", result.glyphsRendered);
	printf("      Cache hits: %d, misses: %d\n", result.cacheHits, result.cacheMisses);
	printf("      Hit rate: %.1f%%\n",
	       100.0 * result.cacheHits / (result.cacheHits + result.cacheMisses + 0.001));
}

TEST(stress_test_10k_glyphs)
{
	printf("    Stress test: 10,000 glyphs per frame...\n");
	BenchmarkResult result = benchmark_text_rendering(
		1,    // YES virtual atlas
		1,    // YES instancing
		1,    // YES prewarm
		100,  // fewer frames (stress test is heavy)
		10000
	);

	if (result.totalTime == 0) {
		SKIP_TEST("Benchmark failed to run");
	}

	printf("      Total time: %.2f ms\n", result.totalTime);
	printf("      Avg frame time: %.3f ms\n", result.avgFrameTime);
	printf("      FPS: %.1f\n", result.fps);
	printf("      Glyphs rendered: %d\n", result.glyphsRendered);
	printf("      Evictions: %d\n", result.evictions);

	// Should maintain >30 FPS even with 10k glyphs
	ASSERT_TRUE(result.fps > 30.0);
}

int main(void)
{
	printf("\n");
	printf("========================================================\n");
	printf("  Performance Baseline Benchmark\n");
	printf("========================================================\n");
	printf("\n");

	if (!test_is_vulkan_available()) {
		printf("Vulkan not available, skipping benchmarks\n");
		return 0;
	}

	printf("Measuring text rendering performance with different optimizations:\n\n");

	printf("Test 1: Baseline (no optimizations)\n");
	printf("------------------------------------\n");
	RUN_TEST(test_baseline_no_optimizations);
	printf("\n");

	printf("Test 2: Instancing Only\n");
	printf("------------------------\n");
	RUN_TEST(test_optimized_instancing_only);
	printf("\n");

	printf("Test 3: Virtual Atlas Only\n");
	printf("---------------------------\n");
	RUN_TEST(test_optimized_virtual_atlas_only);
	printf("\n");

	printf("Test 4: All Optimizations Enabled\n");
	printf("----------------------------------\n");
	RUN_TEST(test_optimized_all_enabled);
	printf("\n");

	printf("Test 5: Stress Test (10k glyphs/frame)\n");
	printf("---------------------------------------\n");
	RUN_TEST(test_stress_test_10k_glyphs);
	printf("\n");

	print_test_summary();
	return test_exit_code();
}
