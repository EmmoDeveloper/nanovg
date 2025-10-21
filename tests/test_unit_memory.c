#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Context allocation/deallocation
TEST(memory_context_lifecycle)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Create and destroy context multiple times to check for leaks
	for (int i = 0; i < 10; i++) {
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	test_destroy_vulkan_context(vk);
}

// Test: Texture allocation stress test
TEST(memory_texture_allocation)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	#define NUM_TEXTURES 50
	int textures[NUM_TEXTURES];

	// Allocate many textures
	unsigned char data[32 * 32 * 4];
	memset(data, 0xFF, sizeof(data));

	for (int i = 0; i < NUM_TEXTURES; i++) {
		textures[i] = nvgCreateImageRGBA(nvg, 32, 32, 0, data);
		ASSERT_NE(textures[i], 0);
	}

	// Free all textures
	for (int i = 0; i < NUM_TEXTURES; i++) {
		nvgDeleteImage(nvg, textures[i]);
	}

	printf("    Allocated and freed %d textures\n", NUM_TEXTURES);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Large texture allocation
TEST(memory_large_textures)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Try different large sizes
	int sizes[] = {256, 512, 1024, 2048};

	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		int size = sizes[i];

		// Allocate texture data (this will be large)
		unsigned char* data = (unsigned char*)malloc(size * size * 4);
		ASSERT_NOT_NULL(data);
		memset(data, 0x80, size * size * 4);

		int img = nvgCreateImageRGBA(nvg, size, size, 0, data);
		ASSERT_NE(img, 0);

		// Verify size
		int w, h;
		nvgImageSize(nvg, img, &w, &h);
		ASSERT_EQ(w, size);
		ASSERT_EQ(h, size);

		nvgDeleteImage(nvg, img);
		free(data);

		printf("    Created and freed %dx%d texture (%.1f MB)\n",
		       size, size, (size * size * 4) / (1024.0 * 1024.0));
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Vertex buffer stress test
TEST(memory_vertex_buffer_stress)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	int num_frames = 50;
	int shapes_per_frame = 200;

	// Render many frames with many shapes to stress vertex buffers
	for (int frame = 0; frame < num_frames; frame++) {
		nvgBeginFrame(nvg, 1920, 1080, 1.0f);

		for (int i = 0; i < shapes_per_frame; i++) {
			float x = (float)(i % 20) * 96.0f;
			float y = (float)(i / 20) * 54.0f;

			nvgBeginPath(nvg);

			// Vary shape types
			if (i % 3 == 0) {
				nvgRect(nvg, x, y, 80, 40);
			} else if (i % 3 == 1) {
				nvgCircle(nvg, x + 40, y + 20, 20);
			} else {
				nvgRoundedRect(nvg, x, y, 80, 40, 5);
			}

			nvgFillColor(nvg, nvgRGBA((i * 17) % 256, (i * 31) % 256, (i * 47) % 256, 255));
			nvgFill(nvg);
		}

		nvgEndFrame(nvg);
	}

	printf("    Rendered %d frames with %d shapes each\n", num_frames, shapes_per_frame);
	printf("    Total vertices: ~%d\n", num_frames * shapes_per_frame * 100);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Texture update stress
TEST(memory_texture_updates)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Create texture
	unsigned char data[128 * 128 * 4];
	int img = nvgCreateImageRGBA(nvg, 128, 128, 0, data);
	ASSERT_NE(img, 0);

	// Update texture many times
	int num_updates = 100;
	for (int i = 0; i < num_updates; i++) {
		// Fill with different pattern each time
		memset(data, (i * 13) % 256, sizeof(data));
		nvgUpdateImage(nvg, img, data);
	}

	printf("    Performed %d texture updates\n", num_updates);

	nvgDeleteImage(nvg, img);
	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple contexts simultaneously
TEST(memory_multiple_contexts)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	#define NUM_CONTEXTS 5
	NVGcontext* contexts[NUM_CONTEXTS];

	// Create multiple contexts
	for (int i = 0; i < NUM_CONTEXTS; i++) {
		contexts[i] = test_create_nanovg_context(vk, NVG_ANTIALIAS);
		ASSERT_NOT_NULL(contexts[i]);
	}

	// Use each context
	for (int i = 0; i < NUM_CONTEXTS; i++) {
		nvgBeginFrame(contexts[i], 800, 600, 1.0f);

		nvgBeginPath(contexts[i]);
		nvgRect(contexts[i], 100, 100, 200, 200);
		nvgFillColor(contexts[i], nvgRGBA(255, 0, 0, 255));
		nvgFill(contexts[i]);

		nvgEndFrame(contexts[i]);
	}

	// Destroy all contexts
	for (int i = 0; i < NUM_CONTEXTS; i++) {
		nvgDeleteVk(contexts[i]);
	}

	printf("    Created and used %d simultaneous contexts\n", NUM_CONTEXTS);

	test_destroy_vulkan_context(vk);
}

// Test: Memory properties query
TEST(memory_vulkan_properties)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProps);

	ASSERT_TRUE(memProps.memoryTypeCount > 0);
	ASSERT_TRUE(memProps.memoryHeapCount > 0);

	// Check that we have device-local memory
	int hasDeviceLocal = 0;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			hasDeviceLocal = 1;
			break;
		}
	}
	ASSERT_TRUE(hasDeviceLocal);

	// Check that we have host-visible memory
	int hasHostVisible = 0;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
			hasHostVisible = 1;
			break;
		}
	}
	ASSERT_TRUE(hasHostVisible);

	printf("    Memory types: %u\n", memProps.memoryTypeCount);
	printf("    Memory heaps: %u\n", memProps.memoryHeapCount);

	// Print heap sizes
	for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
		printf("    Heap %u: %.2f GB%s\n", i,
		       memProps.memoryHeaps[i].size / (1024.0 * 1024.0 * 1024.0),
		       (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? " (device-local)" : "");
	}

	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Unit Tests: Memory\n" COLOR_RESET);
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf("\n");

	// Run all tests
	RUN_TEST(test_memory_context_lifecycle);
	RUN_TEST(test_memory_texture_allocation);
	RUN_TEST(test_memory_large_textures);
	RUN_TEST(test_memory_vertex_buffer_stress);
	RUN_TEST(test_memory_texture_updates);
	RUN_TEST(test_memory_multiple_contexts);
	RUN_TEST(test_memory_vulkan_properties);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
