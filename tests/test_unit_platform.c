#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Vulkan context creation
TEST(platform_vulkan_available)
{
	int available = test_is_vulkan_available();
	ASSERT_TRUE(available);
}

// Test: Physical device detection
TEST(platform_physical_device_detected)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	ASSERT_NOT_NULL(vk->physicalDevice);

	// Get device properties
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(vk->physicalDevice, &props);

	// Device should have a name
	ASSERT_TRUE(strlen(props.deviceName) > 0);

	printf("    GPU: %s\n", props.deviceName);
	printf("    Vendor ID: 0x%04x\n", props.vendorID);
	printf("    Device ID: 0x%04x\n", props.deviceID);

	test_destroy_vulkan_context(vk);
}

// Test: Graphics queue family found
TEST(platform_graphics_queue_found)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	ASSERT_NOT_NULL(vk->device);
	ASSERT_NOT_NULL(vk->queue);

	// Queue family index should be valid
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vk->physicalDevice, &queueFamilyCount, NULL);

	ASSERT_TRUE(vk->queueFamilyIndex < queueFamilyCount);

	test_destroy_vulkan_context(vk);
}

// Test: NanoVG context with platform optimization
TEST(platform_nanovg_context_creation)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Create NanoVG context (this will trigger platform detection)
	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Context should be initialized
	printf("    NanoVG context created successfully\n");
	printf("    Platform optimizations applied automatically\n");

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Multiple context creation/destruction
TEST(platform_multiple_contexts)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Create and destroy multiple NanoVG contexts
	for (int i = 0; i < 5; i++) {
		NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	test_destroy_vulkan_context(vk);
}

// Test: Different creation flags
TEST(platform_creation_flags)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	// Test with different flag combinations
	int flags[] = {
		NVG_ANTIALIAS,
		NVG_STENCIL_STROKES,
		NVG_ANTIALIAS | NVG_STENCIL_STROKES,
		NVG_DEBUG,
		NVG_ANTIALIAS | NVG_DEBUG
	};

	for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
		NVGcontext* nvg = test_create_nanovg_context(vk, flags[i]);
		ASSERT_NOT_NULL(nvg);
		nvgDeleteVk(nvg);
	}

	test_destroy_vulkan_context(vk);
}

// Test: Memory properties available
TEST(platform_memory_properties)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	if (!vk) {
		SKIP_TEST("Vulkan not available");
	}

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProps);

	// Should have at least one memory type
	ASSERT_TRUE(memProps.memoryTypeCount > 0);

	// Should have at least one memory heap
	ASSERT_TRUE(memProps.memoryHeapCount > 0);

	printf("    Memory types: %u\n", memProps.memoryTypeCount);
	printf("    Memory heaps: %u\n", memProps.memoryHeapCount);

	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("\n");
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf(COLOR_CYAN "  NanoVG Vulkan - Unit Tests: Platform\n" COLOR_RESET);
	printf(COLOR_CYAN "======================================\n" COLOR_RESET);
	printf("\n");

	// Run all tests
	RUN_TEST(test_platform_vulkan_available);
	RUN_TEST(test_platform_physical_device_detected);
	RUN_TEST(test_platform_graphics_queue_found);
	RUN_TEST(test_platform_nanovg_context_creation);
	RUN_TEST(test_platform_multiple_contexts);
	RUN_TEST(test_platform_creation_flags);
	RUN_TEST(test_platform_memory_properties);

	// Print summary
	print_test_summary();

	return test_exit_code();
}
