#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_is_vulkan_available(void)
{
	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkInstance instance = VK_NULL_HANDLE;
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance);

	if (result == VK_SUCCESS && instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, NULL);
		return 1;
	}
	return 0;
}

TestVulkanContext* test_create_vulkan_context(void)
{
	TestVulkanContext* ctx = (TestVulkanContext*)calloc(1, sizeof(TestVulkanContext));
	if (!ctx) return NULL;

	// Create Vulkan instance
	VkInstanceCreateInfo instanceInfo = {0};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkResult result = vkCreateInstance(&instanceInfo, NULL, &ctx->instance);
	if (result != VK_SUCCESS) {
		free(ctx);
		return NULL;
	}

	// Select physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		vkDestroyInstance(ctx->instance, NULL);
		free(ctx);
		return NULL;
	}

	VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
	ctx->physicalDevice = devices[0];
	free(devices);

	// Find graphics queue family
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);

	VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

	ctx->queueFamilyIndex = 0;
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			ctx->queueFamilyIndex = i;
			break;
		}
	}
	free(queueFamilies);

	// Create logical device
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueInfo = {0};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = ctx->queueFamilyIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceInfo = {0};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;

	result = vkCreateDevice(ctx->physicalDevice, &deviceInfo, NULL, &ctx->device);
	if (result != VK_SUCCESS) {
		vkDestroyInstance(ctx->instance, NULL);
		free(ctx);
		return NULL;
	}

	// Get queue
	vkGetDeviceQueue(ctx->device, ctx->queueFamilyIndex, 0, &ctx->queue);

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = ctx->queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	result = vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool);
	if (result != VK_SUCCESS) {
		vkDestroyDevice(ctx->device, NULL);
		vkDestroyInstance(ctx->instance, NULL);
		free(ctx);
		return NULL;
	}

	// Create descriptor pool
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 100;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 100;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.maxSets = 100;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(ctx->device, &descriptorPoolInfo, NULL, &ctx->descriptorPool);

	// Create a minimal render pass for testing
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

	vkCreateRenderPass(ctx->device, &renderPassInfo, NULL, &ctx->renderPass);

	ctx->hasVulkan = 1;
	return ctx;
}

void test_destroy_vulkan_context(TestVulkanContext* ctx)
{
	if (!ctx) return;

	if (ctx->descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(ctx->device, ctx->descriptorPool, NULL);
	}
	if (ctx->renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
	}
	if (ctx->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
	}
	if (ctx->device != VK_NULL_HANDLE) {
		vkDestroyDevice(ctx->device, NULL);
	}
	if (ctx->instance != VK_NULL_HANDLE) {
		vkDestroyInstance(ctx->instance, NULL);
	}

	free(ctx);
}

NVGcontext* test_create_nanovg_context(TestVulkanContext* vk, int flags)
{
	if (!vk || !vk->hasVulkan) return NULL;

	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = vk->instance;
	createInfo.physicalDevice = vk->physicalDevice;
	createInfo.device = vk->device;
	createInfo.queue = vk->queue;
	createInfo.queueFamilyIndex = vk->queueFamilyIndex;
	createInfo.commandPool = vk->commandPool;
	createInfo.descriptorPool = vk->descriptorPool;
	createInfo.renderPass = vk->renderPass;
	createInfo.subpass = 0;
	createInfo.maxFrames = 3;
	createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_UNDEFINED;

	return nvgCreateVk(&createInfo, flags);
}
