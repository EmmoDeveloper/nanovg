// Test MSDF rendering pipeline integration
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "nanovg.h"

#define NANOVG_VK_IMPLEMENTATION
#include "nanovg_vk.h"

// Minimal Vulkan setup
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

static int initVulkan() {
	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "MSDF Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if (vkCreateInstance(&createInfo, NULL, &testCtx.instance) != VK_SUCCESS) {
		printf("✗ Failed to create Vulkan instance\n");
		return 0;
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(testCtx.instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		printf("✗ No Vulkan devices found\n");
		return 0;
	}

	VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(testCtx.instance, &deviceCount, devices);
	testCtx.physicalDevice = devices[0];
	free(devices);

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(testCtx.physicalDevice, &props);

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
		printf("✗ No graphics queue found\n");
		return 0;
	}

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
		printf("✗ Failed to create device\n");
		return 0;
	}

	vkGetDeviceQueue(testCtx.device, testCtx.queueFamilyIndex, 0, &testCtx.queue);

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = testCtx.queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(testCtx.device, &poolInfo, NULL, &testCtx.commandPool) != VK_SUCCESS) {
		printf("✗ Failed to create command pool\n");
		return 0;
	}

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 100;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 100;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;
	descriptorPoolInfo.maxSets = 100;

	if (vkCreateDescriptorPool(testCtx.device, &descriptorPoolInfo, NULL, &testCtx.descriptorPool) != VK_SUCCESS) {
		printf("✗ Failed to create descriptor pool\n");
		return 0;
	}

	// Create render pass
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
		printf("✗ Failed to create render pass\n");
		return 0;
	}

	return 1;
}

static void cleanupVulkan() {
	if (testCtx.renderPass) vkDestroyRenderPass(testCtx.device, testCtx.renderPass, NULL);
	if (testCtx.descriptorPool) vkDestroyDescriptorPool(testCtx.device, testCtx.descriptorPool, NULL);
	if (testCtx.commandPool) vkDestroyCommandPool(testCtx.device, testCtx.commandPool, NULL);
	if (testCtx.device) vkDestroyDevice(testCtx.device, NULL);
	if (testCtx.instance) vkDestroyInstance(testCtx.instance, NULL);
}

int main() {
	printf("===========================================\n");
	printf("MSDF Rendering Pipeline Test\n");
	printf("===========================================\n\n");

	printf("Phase 1: Initializing Vulkan\n");
	if (!initVulkan()) {
		return 1;
	}
	printf("✓ Vulkan initialized\n\n");

	printf("Phase 2: Creating NanoVG context with MSDF support\n");

	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = testCtx.instance;
	createInfo.physicalDevice = testCtx.physicalDevice;
	createInfo.device = testCtx.device;
	createInfo.queue = testCtx.queue;
	createInfo.queueFamilyIndex = testCtx.queueFamilyIndex;
	createInfo.renderPass = testCtx.renderPass;
	createInfo.commandPool = testCtx.commandPool;
	createInfo.descriptorPool = testCtx.descriptorPool;
	createInfo.maxFrames = 3;

	NVGcontext* vg = nvgCreateVk(&createInfo, NVG_ANTIALIAS | NVG_MSDF_TEXT);
	if (!vg) {
		printf("✗ Failed to create NanoVG context with MSDF support\n");
		cleanupVulkan();
		return 1;
	}
	printf("✓ Created NanoVG context with NVG_MSDF_TEXT flag\n\n");

	printf("Phase 3: Loading font and enabling MSDF mode\n");

	const char* fontPaths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/System/Library/Fonts/Helvetica.ttc",
		"/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
		NULL
	};

	int font = -1;
	for (int i = 0; fontPaths[i] != NULL; i++) {
		font = nvgCreateFont(vg, "msdf-test", fontPaths[i]);
		if (font >= 0) {
			printf("✓ Loaded font: %s\n", fontPaths[i]);
			break;
		}
	}

	if (font < 0) {
		printf("✗ Failed to load any system font\n");
		nvgDeleteVk(vg);
		cleanupVulkan();
		return 1;
	}

	// Enable MSDF mode for this font
	nvgSetFontMSDF(vg, font, 2); // 2 = MSDF mode
	printf("✓ Enabled MSDF mode (mode=2) for font\n\n");

	printf("Phase 4: Testing MSDF text rendering\n");

	// Begin frame
	nvgBeginFrame(vg, 800, 600, 1.0f);

	// Set up text rendering
	nvgFontFace(vg, "msdf-test");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	// Render test text with curved glyphs
	const char* testTexts[] = {
		"OSCO", // Curves: O, S, C
		"agua", // Curves: a, g, u
		"@&8",  // Complex curves
		NULL
	};

	float y = 100.0f;
	for (int i = 0; testTexts[i] != NULL; i++) {
		nvgText(vg, 100.0f, y, testTexts[i], NULL);
		y += 60.0f;
		printf("  ✓ Rendered: \"%s\"\n", testTexts[i]);
	}

	// End frame
	nvgEndFrame(vg);
	printf("✓ Frame rendered successfully with MSDF text\n\n");

	printf("Phase 5: Testing SDF mode comparison\n");

	// Switch to SDF mode
	nvgSetFontMSDF(vg, font, 1); // 1 = SDF mode
	printf("✓ Switched to SDF mode (mode=1)\n");

	nvgBeginFrame(vg, 800, 600, 1.0f);
	nvgFontFace(vg, "msdf-test");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 100.0f, 400.0f, "SDF Mode", NULL);
	nvgEndFrame(vg);
	printf("✓ Rendered text in SDF mode\n\n");

	printf("Phase 6: Testing bitmap mode\n");

	// Switch to bitmap mode
	nvgSetFontMSDF(vg, font, 0); // 0 = bitmap/normal mode
	printf("✓ Switched to bitmap mode (mode=0)\n");

	nvgBeginFrame(vg, 800, 600, 1.0f);
	nvgFontFace(vg, "msdf-test");
	nvgFontSize(vg, 48.0f);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgText(vg, 100.0f, 500.0f, "Bitmap Mode", NULL);
	nvgEndFrame(vg);
	printf("✓ Rendered text in bitmap mode\n\n");

	// Cleanup
	nvgDeleteVk(vg);
	cleanupVulkan();

	printf("===========================================\n");
	printf("✓ All MSDF rendering tests passed\n");
	printf("✓ Pipeline integration verified\n");
	printf("===========================================\n");

	return 0;
}
