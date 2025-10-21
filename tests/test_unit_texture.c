#include "test_framework.h"

// Define implementation before including headers
#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Create and destroy a basic RGBA texture
TEST(texture_create_rgba)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create 32x32 RGBA texture
	unsigned char data[32 * 32 * 4];
	memset(data, 0xFF, sizeof(data)); // White texture

	int img = nvgCreateImageRGBA(nvg, 32, 32, 0, data);
	ASSERT_NE(img, 0);

	// Get texture size
	int w, h;
	nvgImageSize(nvg, img, &w, &h);
	ASSERT_EQ(w, 32);
	ASSERT_EQ(h, 32);

	// Cleanup
	nvgDeleteImage(nvg, img);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Create different sized textures
TEST(texture_create_different_sizes)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create 64x64 texture
	unsigned char data64[64 * 64 * 4];
	memset(data64, 0x80, sizeof(data64));

	int img64 = nvgCreateImageRGBA(nvg, 64, 64, 0, data64);
	ASSERT_NE(img64, 0);

	// Create 128x128 texture
	unsigned char data128[128 * 128 * 4];
	memset(data128, 0xFF, sizeof(data128));

	int img128 = nvgCreateImageRGBA(nvg, 128, 128, 0, data128);
	ASSERT_NE(img128, 0);

	// Verify sizes
	int w, h;
	nvgImageSize(nvg, img64, &w, &h);
	ASSERT_EQ(w, 64);
	ASSERT_EQ(h, 64);

	nvgImageSize(nvg, img128, &w, &h);
	ASSERT_EQ(w, 128);
	ASSERT_EQ(h, 128);

	// Cleanup
	nvgDeleteImage(nvg, img64);
	nvgDeleteImage(nvg, img128);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Update texture region
TEST(texture_update_region)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create texture
	unsigned char data[64 * 64 * 4];
	memset(data, 0xFF, sizeof(data));

	int img = nvgCreateImageRGBA(nvg, 64, 64, 0, data);
	ASSERT_NE(img, 0);

	// Update 16x16 region at (10, 10)
	unsigned char updateData[16 * 16 * 4];
	memset(updateData, 0x00, sizeof(updateData)); // Black

	nvgUpdateImage(nvg, img, updateData);
	// Note: No direct way to verify pixel data without reading back from GPU
	// This test just ensures the function doesn't crash

	// Cleanup
	nvgDeleteImage(nvg, img);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple texture creation and deletion
TEST(texture_multiple_textures)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	#define NUM_TEXTURES 10
	int textures[NUM_TEXTURES];

	// Create multiple textures
	unsigned char data[32 * 32 * 4];
	for (int i = 0; i < NUM_TEXTURES; i++) {
		memset(data, i * 25, sizeof(data)); // Different colors
		textures[i] = nvgCreateImageRGBA(nvg, 32, 32, 0, data);
		ASSERT_NE(textures[i], 0);
	}

	// Verify all are valid
	for (int i = 0; i < NUM_TEXTURES; i++) {
		int w, h;
		nvgImageSize(nvg, textures[i], &w, &h);
		ASSERT_EQ(w, 32);
		ASSERT_EQ(h, 32);
	}

	// Delete all textures
	for (int i = 0; i < NUM_TEXTURES; i++) {
		nvgDeleteImage(nvg, textures[i]);
	}

	// Cleanup
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Texture flags (premultiplied alpha, flip Y)
TEST(texture_flags)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	unsigned char data[32 * 32 * 4];
	memset(data, 0xFF, sizeof(data));

	// Test with premultiplied alpha flag
	int img1 = nvgCreateImageRGBA(nvg, 32, 32, NVG_IMAGE_PREMULTIPLIED, data);
	ASSERT_NE(img1, 0);

	// Test with flip Y flag
	int img2 = nvgCreateImageRGBA(nvg, 32, 32, NVG_IMAGE_FLIPY, data);
	ASSERT_NE(img2, 0);

	// Test with both flags
	int img3 = nvgCreateImageRGBA(nvg, 32, 32, NVG_IMAGE_PREMULTIPLIED | NVG_IMAGE_FLIPY, data);
	ASSERT_NE(img3, 0);

	// Cleanup
	nvgDeleteImage(nvg, img1);
	nvgDeleteImage(nvg, img2);
	nvgDeleteImage(nvg, img3);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Invalid texture operations
TEST(texture_invalid_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Delete non-existent texture (should not crash)
	nvgDeleteImage(nvg, 9999);

	// Get size of non-existent texture
	int w, h;
	nvgImageSize(nvg, 9999, &w, &h);
	// Should return 0 for invalid texture
	ASSERT_EQ(w, 0);
	ASSERT_EQ(h, 0);

	// Cleanup
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Unit Tests: Texture\n" COLOR_RESET);
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf("\n");

	// Run all tests
	RUN_TEST(test_texture_create_rgba);
	RUN_TEST(test_texture_create_different_sizes);
	RUN_TEST(test_texture_update_region);
	RUN_TEST(test_texture_multiple_textures);
	RUN_TEST(test_texture_flags);
	RUN_TEST(test_texture_invalid_operations);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
