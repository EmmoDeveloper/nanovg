#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "nanovg.h"

#define NANOVG_VK_IMPLEMENTATION
#include "nanovg_vk.h"

// Minimal Vulkan setup for testing
typedef struct {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	uint32_t queueFamilyIndex;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;
} TestVulkanContext;

static TestVulkanContext testCtx = {0};

int initVulkanContext() {
	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "NanoVG Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if (vkCreateInstance(&createInfo, NULL, &testCtx.instance) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return 0;
	}

	printf("✓ Created Vulkan instance\n");

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(testCtx.instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
		return 0;
	}

	VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(testCtx.instance, &deviceCount, devices);
	testCtx.physicalDevice = devices[0];
	free(devices);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(testCtx.physicalDevice, &props);
	printf("✓ Selected GPU: %s\n", props.deviceName);

	// Find queue family
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(testCtx.physicalDevice, &queueFamilyCount, NULL);
	VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(testCtx.physicalDevice, &queueFamilyCount, queueFamilies);

	testCtx.queueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			testCtx.queueFamilyIndex = i;
			break;
		}
	}
	free(queueFamilies);

	if (testCtx.queueFamilyIndex == UINT32_MAX) {
		fprintf(stderr, "Failed to find graphics queue family\n");
		return 0;
	}

	printf("✓ Found graphics queue family: %u\n", testCtx.queueFamilyIndex);

	// Create logical device
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {0};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = testCtx.queueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {0};

	VkDeviceCreateInfo deviceCreateInfo = {0};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	if (vkCreateDevice(testCtx.physicalDevice, &deviceCreateInfo, NULL, &testCtx.device) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create logical device\n");
		return 0;
	}

	printf("✓ Created logical device\n");

	vkGetDeviceQueue(testCtx.device, testCtx.queueFamilyIndex, 0, &testCtx.queue);

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = testCtx.queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(testCtx.device, &poolInfo, NULL, &testCtx.commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return 0;
	}

	printf("✓ Created command pool\n");

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 10;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 10;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;
	descriptorPoolInfo.maxSets = 10;

	if (vkCreateDescriptorPool(testCtx.device, &descriptorPoolInfo, NULL, &testCtx.descriptorPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create descriptor pool\n");
		return 0;
	}

	printf("✓ Created descriptor pool\n");

	// Create dummy render pass for testing
	VkAttachmentDescription colorAttachment = {0};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {0};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(testCtx.device, &renderPassInfo, NULL, &testCtx.renderPass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass\n");
		return 0;
	}

	printf("✓ Created render pass\n");

	return 1;
}

void cleanupVulkanContext() {
	if (testCtx.renderPass) vkDestroyRenderPass(testCtx.device, testCtx.renderPass, NULL);
	if (testCtx.descriptorPool) vkDestroyDescriptorPool(testCtx.device, testCtx.descriptorPool, NULL);
	if (testCtx.commandPool) vkDestroyCommandPool(testCtx.device, testCtx.commandPool, NULL);
	if (testCtx.device) vkDestroyDevice(testCtx.device, NULL);
	if (testCtx.instance) vkDestroyInstance(testCtx.instance, NULL);
}

int main(void) {
	printf("===========================================\n");
	printf("NanoVG Vulkan Backend Integration Test\n");
	printf("===========================================\n\n");

	printf("Phase 1: Initializing Vulkan...\n");
	if (!initVulkanContext()) {
		fprintf(stderr, "✗ Failed to initialize Vulkan\n");
		return 1;
	}
	printf("\n");

	printf("Phase 2: Creating NanoVG context...\n");

	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = testCtx.instance;
	createInfo.physicalDevice = testCtx.physicalDevice;
	createInfo.device = testCtx.device;
	createInfo.queue = testCtx.queue;
	createInfo.queueFamilyIndex = testCtx.queueFamilyIndex;
	createInfo.renderPass = testCtx.renderPass;
	createInfo.subpass = 0;
	createInfo.commandPool = testCtx.commandPool;
	createInfo.descriptorPool = testCtx.descriptorPool;
	createInfo.maxFrames = 3;

	NVGcontext* vg = nvgCreateVk(&createInfo, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (vg == NULL) {
		fprintf(stderr, "✗ Failed to create NanoVG context\n");
		cleanupVulkanContext();
		return 1;
	}

	printf("✓ Created NanoVG Vulkan context\n\n");

	printf("Phase 3: Testing NanoVG API calls...\n");

	// Test frame begin/end
	nvgBeginFrame(vg, 800, 600, 1.0f);
	printf("✓ nvgBeginFrame()\n");

	// Test simple path
	nvgBeginPath(vg);
	nvgRect(vg, 100, 100, 300, 200);
	nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
	nvgFill(vg);
	printf("✓ nvgRect() + nvgFill()\n");

	// Test stroke
	nvgBeginPath(vg);
	nvgCircle(vg, 400, 300, 50);
	nvgStrokeColor(vg, nvgRGBA(0, 160, 255, 255));
	nvgStrokeWidth(vg, 3.0f);
	nvgStroke(vg);
	printf("✓ nvgCircle() + nvgStroke()\n");

	nvgEndFrame(vg);
	printf("✓ nvgEndFrame()\n\n");

	printf("Phase 4: Cleanup...\n");
	nvgDeleteVk(vg);
	printf("✓ Destroyed NanoVG context\n");

	cleanupVulkanContext();
	printf("✓ Cleaned up Vulkan\n\n");

	printf("===========================================\n");
	printf("✓ All tests passed successfully!\n");
	printf("===========================================\n");

	return 0;
}
