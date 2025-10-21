// Test for virtual atlas system (CJK support)

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include "../src/nanovg_vk_virtual_atlas.h"
#include <stdio.h>
#include <time.h>

// Test: Virtual atlas creation and destruction
TEST(virtual_atlas_create_destroy)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	// Create virtual atlas
	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(vk->device,
	                                                      vk->physicalDevice,
	                                                      NULL,	// No font context yet
	                                                      NULL);	// No rasterize callback (uses placeholder)
	ASSERT_NOT_NULL(atlas);

	printf("  ✓ Virtual atlas created\n");
	printf("    Physical size: %dx%d\n", VKNVG_ATLAS_PHYSICAL_SIZE, VKNVG_ATLAS_PHYSICAL_SIZE);
	printf("    Page size: %d\n", VKNVG_ATLAS_PAGE_SIZE);
	printf("    Max pages: %d\n", VKNVG_ATLAS_MAX_PAGES);
	printf("    Cache size: %d\n", VKNVG_GLYPH_CACHE_SIZE);

	// Destroy atlas
	vknvg__destroyVirtualAtlas(atlas);
	printf("  ✓ Virtual atlas destroyed\n");

	test_destroy_vulkan_context(vk);
}

// Test: Glyph request and caching
TEST(virtual_atlas_glyph_request)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(vk->device,
	                                                      vk->physicalDevice,
	                                                      NULL, NULL);
	ASSERT_NOT_NULL(atlas);

	// Request a glyph
	VKNVGglyphKey key = {
		.fontID = 1,
		.codepoint = 0x4E00,	// CJK character
		.size = 16 << 16,		// 16.0 pixels
		.padding = 0
	};

	VKNVGglyphCacheEntry* entry = vknvg__requestGlyph(atlas, key);
	ASSERT_NOT_NULL(entry);
	printf("  ✓ Glyph requested: U+%04X\n", key.codepoint);
	printf("    State: %s\n", entry->state == VKNVG_GLYPH_LOADING ? "LOADING" : "OTHER");
	printf("    Atlas position: (%d, %d)\n", entry->atlasX, entry->atlasY);

	// Wait a bit for background loading
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};	// 100ms
	nanosleep(&ts, NULL);

	// Request same glyph again (should be cache hit)
	VKNVGglyphCacheEntry* entry2 = vknvg__requestGlyph(atlas, key);
	ASSERT_TRUE(entry2 == entry);	// Should be same entry
	printf("  ✓ Cache hit for same glyph\n");

	// Get statistics
	uint32_t hits, misses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &hits, &misses, &evictions, &uploads);
	printf("  Statistics:\n");
	printf("    Cache hits: %d\n", hits);
	printf("    Cache misses: %d\n", misses);
	printf("    Evictions: %d\n", evictions);
	printf("    Uploads: %d\n", uploads);

	vknvg__destroyVirtualAtlas(atlas);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple glyphs
TEST(virtual_atlas_multiple_glyphs)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(vk->device,
	                                                      vk->physicalDevice,
	                                                      NULL, NULL);
	ASSERT_NOT_NULL(atlas);

	// Request multiple CJK glyphs
	int numGlyphs = 100;
	for (int i = 0; i < numGlyphs; i++) {
		VKNVGglyphKey key = {
			.fontID = 1,
			.codepoint = 0x4E00 + i,	// Sequential CJK characters
			.size = 16 << 16,
			.padding = 0
		};

		VKNVGglyphCacheEntry* entry = vknvg__requestGlyph(atlas, key);
		ASSERT_NOT_NULL(entry);
	}

	printf("  ✓ Requested %d glyphs\n", numGlyphs);

	// Wait for background loading
	struct timespec ts2 = {.tv_sec = 0, .tv_nsec = 200000000};	// 200ms
	nanosleep(&ts2, NULL);

	// Get statistics
	uint32_t hits, misses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &hits, &misses, &evictions, &uploads);
	printf("  Statistics after %d glyphs:\n", numGlyphs);
	printf("    Cache hits: %d\n", hits);
	printf("    Cache misses: %d\n", misses);
	printf("    Evictions: %d\n", evictions);
	printf("    Uploads: %d\n", uploads);

	vknvg__destroyVirtualAtlas(atlas);
	test_destroy_vulkan_context(vk);
}

// Test: LRU eviction when atlas is full
TEST(virtual_atlas_lru_eviction)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	VKNVGvirtualAtlas* atlas = vknvg__createVirtualAtlas(vk->device,
	                                                      vk->physicalDevice,
	                                                      NULL, NULL);
	ASSERT_NOT_NULL(atlas);

	// Request more glyphs than available pages (4096 pages)
	int numGlyphs = 5000;
	for (int i = 0; i < numGlyphs; i++) {
		VKNVGglyphKey key = {
			.fontID = 1,
			.codepoint = 0x4E00 + i,
			.size = 16 << 16,
			.padding = 0
		};

		VKNVGglyphCacheEntry* entry = vknvg__requestGlyph(atlas, key);
		ASSERT_NOT_NULL(entry);
	}

	printf("  ✓ Requested %d glyphs (exceeds %d pages)\n", numGlyphs, VKNVG_ATLAS_MAX_PAGES);

	// Get statistics
	uint32_t hits, misses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &hits, &misses, &evictions, &uploads);
	printf("  Statistics after %d glyphs:\n", numGlyphs);
	printf("    Cache hits: %d\n", hits);
	printf("    Cache misses: %d\n", misses);
	printf("    Evictions: %d (expected ~%d)\n", evictions, numGlyphs - VKNVG_ATLAS_MAX_PAGES);
	printf("    Uploads: %d\n", uploads);

	ASSERT_TRUE(evictions > 0);	// Must have evicted some entries
	ASSERT_TRUE(evictions >= (numGlyphs - VKNVG_ATLAS_MAX_PAGES));	// At least this many

	vknvg__destroyVirtualAtlas(atlas);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("==========================================\n");
	printf("  Virtual Atlas Tests (CJK Support)\n");
	printf("==========================================\n\n");

	RUN_TEST(test_virtual_atlas_create_destroy);
	printf("\n");
	RUN_TEST(test_virtual_atlas_glyph_request);
	printf("\n");
	RUN_TEST(test_virtual_atlas_multiple_glyphs);
	printf("\n");
	RUN_TEST(test_virtual_atlas_lru_eviction);
	printf("\n");

	print_test_summary();
	return test_exit_code();
}
