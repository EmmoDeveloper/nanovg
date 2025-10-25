#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <vulkan/vulkan.h>
#include "nanovg.h"
#include "nanovg_vk.h"

// Simplified Vulkan context for testing
typedef struct {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	uint32_t queueFamilyIndex;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;
	VkDebugUtilsMessengerEXT debugMessenger;
	int hasVulkan;
} TestVulkanContext;

// Create minimal Vulkan context for testing
// Returns NULL if Vulkan is not available (tests should skip)
TestVulkanContext* test_create_vulkan_context(void);

// Destroy Vulkan context
void test_destroy_vulkan_context(TestVulkanContext* ctx);

// Create NanoVG context using test Vulkan context
NVGcontext* test_create_nanovg_context(TestVulkanContext* vk, int flags);

// Helper: Check if Vulkan is available
int test_is_vulkan_available(void);

#endif // TEST_UTILS_H
