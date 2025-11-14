#include "window_utils.h"
#include "vulkan/nvg_vk_hdr_metadata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	const char* severity = "UNKNOWN";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		severity = "ERROR";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		severity = "WARNING";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		severity = "INFO";
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		severity = "VERBOSE";
	}

	const char* type = "GENERAL";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		type = "VALIDATION";
	} else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		type = "PERFORMANCE";
	}

	fprintf(stderr, "[VULKAN %s %s] %s\n", severity, type, pCallbackData->pMessage);
	return VK_FALSE;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	WindowVulkanContext* ctx = (WindowVulkanContext*)glfwGetWindowUserPointer(window);
	if (ctx) {
		ctx->framebufferResized = 1;
	}
}

static void createSwapchain(WindowVulkanContext* ctx)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, NULL);
	VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, formats);

	// Priority-based HDR/wide-gamut color space selection
	// Priority: HDR10 ST.2084 > HDR10 HLG > Display P3 > Extended sRGB > sRGB
	VkSurfaceFormatKHR surfaceFormat = formats[0];
	int foundPreferred = 0;

	// Try HDR10 ST.2084 (PQ) first
	for (uint32_t i = 0; i < formatCount && !foundPreferred; i++) {
		if (formats[i].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
			formats[i].colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
			surfaceFormat = formats[i];
			foundPreferred = 1;
			printf("Selected HDR10 ST.2084 (PQ) color space\n");
		}
	}

	// Try HDR10 HLG
	for (uint32_t i = 0; i < formatCount && !foundPreferred; i++) {
		if (formats[i].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
			formats[i].colorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT) {
			surfaceFormat = formats[i];
			foundPreferred = 1;
			printf("Selected HDR10 HLG color space\n");
		}
	}

	// Try Display P3
	for (uint32_t i = 0; i < formatCount && !foundPreferred; i++) {
		if ((formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
		     formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
			formats[i].colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT) {
			surfaceFormat = formats[i];
			foundPreferred = 1;
			printf("Selected Display P3 color space\n");
		}
	}

	// Try Extended sRGB (linear)
	for (uint32_t i = 0; i < formatCount && !foundPreferred; i++) {
		if ((formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
		     formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
			formats[i].colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
			surfaceFormat = formats[i];
			foundPreferred = 1;
			printf("Selected Extended sRGB Linear color space\n");
		}
	}

	// Fallback to standard sRGB
	for (uint32_t i = 0; i < formatCount && !foundPreferred; i++) {
		if ((formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
		     formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
			formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = formats[i];
			foundPreferred = 1;
			printf("Selected sRGB color space (fallback)\n");
		}
	}

	// Set HDR metadata if HDR color space selected
	int isHDR = (surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT ||
	             surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_HLG_EXT);

	free(formats);

	ctx->swapchainImageFormat = surfaceFormat.format;
	ctx->swapchainColorSpace = surfaceFormat.colorSpace;

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, NULL);
	VkPresentModeKHR* presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, presentModes);

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < presentModeCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
			break;
		}
	}
	free(presentModes);

	VkExtent2D extent;
	if (capabilities.currentExtent.width != UINT32_MAX) {
		extent = capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(ctx->window, &width, &height);
		extent.width = width;
		extent.height = height;

		if (extent.width < capabilities.minImageExtent.width) {
			extent.width = capabilities.minImageExtent.width;
		} else if (extent.width > capabilities.maxImageExtent.width) {
			extent.width = capabilities.maxImageExtent.width;
		}

		if (extent.height < capabilities.minImageExtent.height) {
			extent.height = capabilities.minImageExtent.height;
		} else if (extent.height > capabilities.maxImageExtent.height) {
			extent.height = capabilities.maxImageExtent.height;
		}
	}
	ctx->swapchainExtent = extent;

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = ctx->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (ctx->graphicsQueueFamilyIndex != ctx->presentQueueFamilyIndex) {
		uint32_t queueFamilyIndices[] = {ctx->graphicsQueueFamilyIndex, ctx->presentQueueFamilyIndex};
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(ctx->device, &createInfo, NULL, &ctx->swapchain) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create swapchain\n");
		return;
	}

	// Apply HDR metadata if HDR color space selected
	if (isHDR) {
		VkHdrMetadataEXT hdrMetadata;
		if (nvgvk_query_hdr_capabilities(ctx->physicalDevice, ctx->surface, &hdrMetadata)) {
			nvgvk_set_hdr_metadata(ctx->device, ctx->swapchain, &hdrMetadata);
			printf("HDR metadata applied to swapchain\n");
		}
	}

	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, NULL);
	ctx->swapchainImages = malloc(sizeof(VkImage) * ctx->swapchainImageCount);
	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, ctx->swapchainImages);

	ctx->swapchainImageViews = malloc(sizeof(VkImageView) * ctx->swapchainImageCount);
	for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
		VkImageViewCreateInfo viewInfo = {0};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = ctx->swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = ctx->swapchainImageFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->swapchainImageViews[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view %d\n", i);
		}
	}
}

static void createRenderPass(WindowVulkanContext* ctx)
{
	VkAttachmentDescription colorAttachment = {0};
	colorAttachment.format = ctx->swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthStencilAttachment = {0};
	depthStencilAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {0};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {0};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = {colorAttachment, depthStencilAttachment};

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(ctx->device, &renderPassInfo, NULL, &ctx->renderPass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass\n");
	}
}

static void createDepthStencilImage(WindowVulkanContext* ctx)
{
	ctx->depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = ctx->swapchainExtent.width;
	imageInfo.extent.height = ctx->swapchainExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = ctx->depthStencilFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(ctx->device, &imageInfo, NULL, &ctx->depthStencilImage) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create depth/stencil image\n");
		return;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ctx->device, ctx->depthStencilImage, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);

	uint32_t memoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			memoryTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->depthStencilImageMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate depth/stencil image memory\n");
		return;
	}

	vkBindImageMemory(ctx->device, ctx->depthStencilImage, ctx->depthStencilImageMemory, 0);

	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = ctx->depthStencilImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = ctx->depthStencilFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->depthStencilImageView) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create depth/stencil image view\n");
		return;
	}

	// Transition depth/stencil image to optimal layout
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = ctx->commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = ctx->depthStencilImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx->graphicsQueue);

	vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
}

static void createFramebuffers(WindowVulkanContext* ctx)
{
	printf("[VULKAN] createFramebuffers: swapchainImageCount = %u\n", ctx->swapchainImageCount);
	fflush(stdout);

	ctx->framebuffers = malloc(sizeof(VkFramebuffer) * ctx->swapchainImageCount);

	for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
		printf("[VULKAN] Creating framebuffer %u with image view %p\n", i, (void*)ctx->swapchainImageViews[i]);
		fflush(stdout);

		VkImageView attachments[] = {ctx->swapchainImageViews[i], ctx->depthStencilImageView};

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = ctx->renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = ctx->swapchainExtent.width;
		framebufferInfo.height = ctx->swapchainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(ctx->device, &framebufferInfo, NULL, &ctx->framebuffers[i]);
		printf("[VULKAN] vkCreateFramebuffer returned: %d\n", result);
		fflush(stdout);

		if (result != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffer %d\n", i);
		}
	}
}

static void createSyncObjects(WindowVulkanContext* ctx)
{
	printf("[VULKAN] createSyncObjects...\n");
	fflush(stdout);
	// Semaphores indexed by swapchain image to avoid reuse issues
	printf("[VULKAN] Allocating semaphore arrays for %u swapchain images...\n", ctx->swapchainImageCount);
	fflush(stdout);
	ctx->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * ctx->swapchainImageCount);
	ctx->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * ctx->swapchainImageCount);
	ctx->imageInFlightFences = malloc(sizeof(VkFence) * ctx->swapchainImageCount);

	// Fences and command buffers indexed by frame in flight
	printf("[VULKAN] Allocating fence/command buffer arrays for %u frames in flight...\n", ctx->maxFramesInFlight);
	fflush(stdout);
	ctx->inFlightFences = malloc(sizeof(VkFence) * ctx->maxFramesInFlight);
	ctx->commandBuffers = malloc(sizeof(VkCommandBuffer) * ctx->maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Create semaphores per swapchain image
	printf("[VULKAN] Creating semaphores...\n");
	fflush(stdout);
	for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
		printf("[VULKAN] Creating semaphore %u/%u...\n", i+1, ctx->swapchainImageCount);
		fflush(stdout);
		if (vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphores[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create semaphores for image %d\n", i);
		}
		ctx->imageInFlightFences[i] = VK_NULL_HANDLE;  // Initially no frame owns this image
	}

	printf("[VULKAN] Creating fences...\n");
	fflush(stdout);
	VkFenceCreateInfo fenceInfo = {0};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// Create fences per frame in flight
	for (uint32_t i = 0; i < ctx->maxFramesInFlight; i++) {
		printf("[VULKAN] Creating fence %u/%u...\n", i+1, ctx->maxFramesInFlight);
		fflush(stdout);
		if (vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->inFlightFences[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create fence for frame %d\n", i);
		}
	}

	printf("[VULKAN] Allocating command buffers...\n");
	fflush(stdout);
	// Allocate command buffers per frame in flight
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = ctx->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = ctx->maxFramesInFlight;

	if (vkAllocateCommandBuffers(ctx->device, &allocInfo, ctx->commandBuffers) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate command buffers\n");
	}
	printf("[VULKAN] createSyncObjects complete\n");
	fflush(stdout);
}

static void cleanupSwapchain(WindowVulkanContext* ctx)
{
	if (ctx->framebuffers) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
		}
		free(ctx->framebuffers);
		ctx->framebuffers = NULL;
	}

	if (ctx->swapchainImageViews) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
		}
		free(ctx->swapchainImageViews);
		ctx->swapchainImageViews = NULL;
	}

	if (ctx->swapchainImages) {
		free(ctx->swapchainImages);
		ctx->swapchainImages = NULL;
	}

	if (ctx->swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
		ctx->swapchain = VK_NULL_HANDLE;
	}
}

static void recreateSwapchain(WindowVulkanContext* ctx)
{
	printf("[VULKAN] recreateSwapchain called\n");
	fflush(stdout);

	int width = 0, height = 0;
	glfwGetFramebufferSize(ctx->window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(ctx->window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(ctx->device);

	// Clean up old semaphores (per swapchain image)
	if (ctx->imageAvailableSemaphores) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphores[i], NULL);
		}
		free(ctx->imageAvailableSemaphores);
		ctx->imageAvailableSemaphores = NULL;
	}

	if (ctx->renderFinishedSemaphores) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphores[i], NULL);
		}
		free(ctx->renderFinishedSemaphores);
		ctx->renderFinishedSemaphores = NULL;
	}

	if (ctx->imageInFlightFences) {
		free(ctx->imageInFlightFences);
		ctx->imageInFlightFences = NULL;
	}

	printf("[VULKAN] cleanupSwapchain...\n");
	fflush(stdout);
	cleanupSwapchain(ctx);

	// Cleanup old depth/stencil image
	if (ctx->depthStencilImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(ctx->device, ctx->depthStencilImageView, NULL);
		ctx->depthStencilImageView = VK_NULL_HANDLE;
	}
	if (ctx->depthStencilImage != VK_NULL_HANDLE) {
		vkDestroyImage(ctx->device, ctx->depthStencilImage, NULL);
		ctx->depthStencilImage = VK_NULL_HANDLE;
	}
	if (ctx->depthStencilImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(ctx->device, ctx->depthStencilImageMemory, NULL);
		ctx->depthStencilImageMemory = VK_NULL_HANDLE;
	}

	printf("[VULKAN] createSwapchain...\n");
	fflush(stdout);
	createSwapchain(ctx);

	printf("[VULKAN] createDepthStencilImage...\n");
	fflush(stdout);
	createDepthStencilImage(ctx);

	printf("[VULKAN] createFramebuffers...\n");
	fflush(stdout);
	createFramebuffers(ctx);

	// Recreate semaphores for new swapchain image count
	printf("[VULKAN] Recreating semaphores for %u swapchain images...\n", ctx->swapchainImageCount);
	fflush(stdout);

	ctx->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * ctx->swapchainImageCount);
	ctx->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * ctx->swapchainImageCount);
	ctx->imageInFlightFences = malloc(sizeof(VkFence) * ctx->swapchainImageCount);

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
		if (vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphores[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create semaphores for image %d\n", i);
		}
		ctx->imageInFlightFences[i] = VK_NULL_HANDLE;
	}

	ctx->framebufferResized = 0;
	printf("[VULKAN] recreateSwapchain done\n");
	fflush(stdout);
}

WindowVulkanContext* window_create_context(int width, int height, const char* title)
{
	printf("[VULKAN] Starting window context creation...\n");
	fflush(stdout);

	WindowVulkanContext* ctx = (WindowVulkanContext*)calloc(1, sizeof(WindowVulkanContext));
	if (!ctx) return NULL;

	ctx->width = width;
	ctx->height = height;
	ctx->maxFramesInFlight = 2;

	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		free(ctx);
		return NULL;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	ctx->window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!ctx->window) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	glfwSetWindowUserPointer(ctx->window, ctx);
	glfwSetFramebufferSizeCallback(ctx->window, framebufferResizeCallback);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	VkLayerProperties* availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	int validationLayerPresent = 0;
	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	for (uint32_t i = 0; i < layerCount; i++) {
		if (strcmp(availableLayers[i].layerName, validationLayerName) == 0) {
			validationLayerPresent = 1;
			break;
		}
	}
	free(availableLayers);

	const char** extensions = malloc(sizeof(char*) * (glfwExtensionCount + 1));
	for (uint32_t i = 0; i < glfwExtensionCount; i++) {
		extensions[i] = glfwExtensions[i];
	}
	extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	const char* layers[] = {validationLayerName};

	VkInstanceCreateInfo instanceInfo = {0};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.enabledExtensionCount = glfwExtensionCount + (validationLayerPresent ? 1 : 0);
	instanceInfo.ppEnabledExtensionNames = extensions;

	if (validationLayerPresent) {
		instanceInfo.enabledLayerCount = 1;
		instanceInfo.ppEnabledLayerNames = layers;
		printf("[VULKAN] Validation layers enabled\n");
	}

	VkResult result = vkCreateInstance(&instanceInfo, NULL, &ctx->instance);
	free(extensions);

	if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	if (validationLayerPresent) {
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {0};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = debugCallback;

		PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != NULL) {
			func(ctx->instance, &debugInfo, NULL, &ctx->debugMessenger);
		}
	}

	if (glfwCreateWindowSurface(ctx->instance, ctx->window, NULL, &ctx->surface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create window surface\n");
		vkDestroyInstance(ctx->instance, NULL);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		fprintf(stderr, "No Vulkan devices found\n");
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
		vkDestroyInstance(ctx->instance, NULL);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
	ctx->physicalDevice = devices[0];
	free(devices);

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
	VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

	ctx->graphicsQueueFamilyIndex = UINT32_MAX;
	ctx->presentQueueFamilyIndex = UINT32_MAX;

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 hasGraphics = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
		VkBool32 presentSupport = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physicalDevice, i, ctx->surface, &presentSupport);

		if (hasGraphics && presentSupport) {
			ctx->graphicsQueueFamilyIndex = i;
			ctx->presentQueueFamilyIndex = i;
			break;
		}

		if (hasGraphics && ctx->graphicsQueueFamilyIndex == UINT32_MAX) {
			ctx->graphicsQueueFamilyIndex = i;
		}

		if (presentSupport && ctx->presentQueueFamilyIndex == UINT32_MAX) {
			ctx->presentQueueFamilyIndex = i;
		}
	}
	free(queueFamilies);

	if (ctx->graphicsQueueFamilyIndex == UINT32_MAX || ctx->presentQueueFamilyIndex == UINT32_MAX) {
		fprintf(stderr, "Failed to find suitable queue families\n");
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
		vkDestroyInstance(ctx->instance, NULL);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	printf("[VULKAN] Selected queue families - Graphics: %u, Present: %u\n",
		ctx->graphicsQueueFamilyIndex, ctx->presentQueueFamilyIndex);
	fflush(stdout);

	// Detect additional queue families (compute, transfer) for NanoVG optimizations
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
	queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueInfos[8];  // Support up to 8 queue families
	uint32_t queueCreateInfoCount = 0;
	uint32_t familiesCreated[8] = {0};  // Track which families we've already added
	uint32_t familiesCreatedCount = 0;

	// Helper to check if family already added
	#define FAMILY_ALREADY_ADDED(idx) ({ \
		int found = 0; \
		for (uint32_t k = 0; k < familiesCreatedCount; k++) { \
			if (familiesCreated[k] == (idx)) { found = 1; break; } \
		} \
		found; \
	})

	// Add graphics queue family
	queueInfos[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfos[queueCreateInfoCount].pNext = NULL;
	queueInfos[queueCreateInfoCount].flags = 0;
	queueInfos[queueCreateInfoCount].queueFamilyIndex = ctx->graphicsQueueFamilyIndex;
	queueInfos[queueCreateInfoCount].queueCount = 1;
	queueInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
	familiesCreated[familiesCreatedCount++] = ctx->graphicsQueueFamilyIndex;
	queueCreateInfoCount++;

	// Add present queue family if different
	if (ctx->graphicsQueueFamilyIndex != ctx->presentQueueFamilyIndex) {
		queueInfos[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfos[queueCreateInfoCount].pNext = NULL;
		queueInfos[queueCreateInfoCount].flags = 0;
		queueInfos[queueCreateInfoCount].queueFamilyIndex = ctx->presentQueueFamilyIndex;
		queueInfos[queueCreateInfoCount].queueCount = 1;
		queueInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
		familiesCreated[familiesCreatedCount++] = ctx->presentQueueFamilyIndex;
		queueCreateInfoCount++;
	}

	// Add dedicated compute queue families for async compute support
	for (uint32_t i = 0; i < queueFamilyCount && queueCreateInfoCount < 8; i++) {
		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && !FAMILY_ALREADY_ADDED(i)) {
			queueInfos[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfos[queueCreateInfoCount].pNext = NULL;
			queueInfos[queueCreateInfoCount].flags = 0;
			queueInfos[queueCreateInfoCount].queueFamilyIndex = i;
			queueInfos[queueCreateInfoCount].queueCount = 1;
			queueInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
			familiesCreated[familiesCreatedCount++] = i;
			queueCreateInfoCount++;
		}
	}

	free(queueFamilies);

	const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo deviceInfo = {0};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceInfo.pQueueCreateInfos = queueInfos;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;

	if (vkCreateDevice(ctx->physicalDevice, &deviceInfo, NULL, &ctx->device) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create logical device\n");
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
		vkDestroyInstance(ctx->instance, NULL);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	printf("[VULKAN] Created device with %u queue families\n", queueCreateInfoCount);

	printf("[VULKAN] About to get graphics queue from family %u\n", ctx->graphicsQueueFamilyIndex);
	fflush(stdout);
	vkGetDeviceQueue(ctx->device, ctx->graphicsQueueFamilyIndex, 0, &ctx->graphicsQueue);

	printf("[VULKAN] About to get present queue from family %u\n", ctx->presentQueueFamilyIndex);
	fflush(stdout);
	vkGetDeviceQueue(ctx->device, ctx->presentQueueFamilyIndex, 0, &ctx->presentQueue);

	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = ctx->graphicsQueueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	printf("[VULKAN] ctx->graphicsQueueFamilyIndex = %u\n", ctx->graphicsQueueFamilyIndex);
	printf("[VULKAN] poolInfo.queueFamilyIndex = %u\n", poolInfo.queueFamilyIndex);
	printf("[VULKAN] About to call vkCreateCommandPool...\n");
	fflush(stdout);

	VkResult poolResult = vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool);

	printf("[VULKAN] vkCreateCommandPool returned: %d\n", poolResult);
	fflush(stdout);

	if (poolResult != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		vkDestroyDevice(ctx->device, NULL);
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
		vkDestroyInstance(ctx->instance, NULL);
		glfwDestroyWindow(ctx->window);
		glfwTerminate();
		free(ctx);
		return NULL;
	}

	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[0].descriptorCount = 100;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 100;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.maxSets = 100;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(ctx->device, &descriptorPoolInfo, NULL, &ctx->descriptorPool);

	createSwapchain(ctx);
	createDepthStencilImage(ctx);
	createRenderPass(ctx);
	createFramebuffers(ctx);
	createSyncObjects(ctx);

	printf("[VULKAN] window_create_context returning...\n");
	fflush(stdout);
	return ctx;
}

void window_destroy_context(WindowVulkanContext* ctx)
{
	if (!ctx) return;

	vkDeviceWaitIdle(ctx->device);

	if (ctx->imageAvailableSemaphores) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphores[i], NULL);
		}
		free(ctx->imageAvailableSemaphores);
	}

	if (ctx->renderFinishedSemaphores) {
		for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
			vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphores[i], NULL);
		}
		free(ctx->renderFinishedSemaphores);
	}

	if (ctx->imageInFlightFences) {
		free(ctx->imageInFlightFences);
	}

	if (ctx->inFlightFences) {
		for (uint32_t i = 0; i < ctx->maxFramesInFlight; i++) {
			vkDestroyFence(ctx->device, ctx->inFlightFences[i], NULL);
		}
		free(ctx->inFlightFences);
	}

	if (ctx->commandBuffers) {
		vkFreeCommandBuffers(ctx->device, ctx->commandPool, ctx->maxFramesInFlight, ctx->commandBuffers);
		free(ctx->commandBuffers);
	}

	cleanupSwapchain(ctx);

	if (ctx->depthStencilImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(ctx->device, ctx->depthStencilImageView, NULL);
	}
	if (ctx->depthStencilImage != VK_NULL_HANDLE) {
		vkDestroyImage(ctx->device, ctx->depthStencilImage, NULL);
	}
	if (ctx->depthStencilImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(ctx->device, ctx->depthStencilImageMemory, NULL);
	}

	if (ctx->renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
	}

	if (ctx->descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(ctx->device, ctx->descriptorPool, NULL);
	}

	if (ctx->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
	}

	if (ctx->device != VK_NULL_HANDLE) {
		vkDestroyDevice(ctx->device, NULL);
	}

	if (ctx->surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
	}

	if (ctx->debugMessenger != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != NULL) {
			func(ctx->instance, ctx->debugMessenger, NULL);
		}
	}

	if (ctx->instance != VK_NULL_HANDLE) {
		vkDestroyInstance(ctx->instance, NULL);
	}

	if (ctx->window) {
		glfwDestroyWindow(ctx->window);
	}

	glfwTerminate();

	free(ctx);
}

NVGcontext* window_create_nanovg_context(WindowVulkanContext* ctx, int flags)
{
	// NanoVG backend not implemented yet
	(void)ctx;
	(void)flags;
	return NULL;
}

int window_begin_frame(WindowVulkanContext* ctx, uint32_t* imageIndex, VkCommandBuffer* cmdBuf)
{
	printf("[VULKAN] window_begin_frame called\n");
	fflush(stdout);

	// Wait for current frame's fence
	vkWaitForFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame], VK_TRUE, UINT64_MAX);

	// Acquire next image - we don't know which image yet, so we can't use per-image semaphore here
	// Instead, use a temporary semaphore indexed by currentFrame for acquire
	VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
		ctx->imageAvailableSemaphores[ctx->currentFrame % ctx->swapchainImageCount], VK_NULL_HANDLE, imageIndex);

	printf("[VULKAN] vkAcquireNextImageKHR returned: %d\n", result);
	fflush(stdout);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		printf("[VULKAN] Swapchain out of date, recreating...\n");
		fflush(stdout);
		recreateSwapchain(ctx);
		return 0;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr, "Failed to acquire swapchain image\n");
		return 0;
	}

	// Wait for the image if it's still being used by a previous frame
	if (ctx->imageInFlightFences[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(ctx->device, 1, &ctx->imageInFlightFences[*imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark image as now being used by this frame
	ctx->imageInFlightFences[*imageIndex] = ctx->inFlightFences[ctx->currentFrame];

	// Reset fence for this frame
	vkResetFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame]);

	// Get and begin command buffer
	*cmdBuf = ctx->commandBuffers[ctx->currentFrame];

	vkResetCommandBuffer(*cmdBuf, 0);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(*cmdBuf, &beginInfo) != VK_SUCCESS) {
		fprintf(stderr, "Failed to begin command buffer\n");
		return 0;
	}

	return 1;
}

void window_end_frame(WindowVulkanContext* ctx, uint32_t imageIndex, VkCommandBuffer cmdBuf)
{
	// End command buffer
	if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
		fprintf(stderr, "Failed to end command buffer\n");
		return;
	}

	// Submit the command buffer with proper synchronization
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {ctx->imageAvailableSemaphores[ctx->currentFrame % ctx->swapchainImageCount]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	VkSemaphore signalSemaphores[] = {ctx->renderFinishedSemaphores[imageIndex]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFences[ctx->currentFrame]) != VK_SUCCESS) {
		fprintf(stderr, "Failed to submit command buffer\n");
	}

	// Present the result
	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = {ctx->swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	VkResult result = vkQueuePresentKHR(ctx->presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || ctx->framebufferResized) {
		recreateSwapchain(ctx);
	} else if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to present swapchain image\n");
	}

	ctx->currentFrame = (ctx->currentFrame + 1) % ctx->maxFramesInFlight;
}

int window_should_close(WindowVulkanContext* ctx)
{
	return glfwWindowShouldClose(ctx->window);
}

void window_poll_events(void)
{
	glfwPollEvents();
}

void window_get_framebuffer_size(WindowVulkanContext* ctx, int* width, int* height)
{
	glfwGetFramebufferSize(ctx->window, width, height);
}

VkImageView window_get_swapchain_image_view(WindowVulkanContext* ctx, uint32_t imageIndex)
{
	if (imageIndex < ctx->swapchainImageCount) {
		return ctx->swapchainImageViews[imageIndex];
	}
	return VK_NULL_HANDLE;
}

VkImageView window_get_depth_stencil_image_view(WindowVulkanContext* ctx)
{
	return ctx->depthStencilImageView;
}

int window_save_screenshot(WindowVulkanContext* ctx, uint32_t imageIndex, const char* filename)
{
	if (!ctx || imageIndex >= ctx->swapchainImageCount || !filename) {
		fprintf(stderr, "Invalid parameters for screenshot\n");
		return 0;
	}

	VkImage srcImage = ctx->swapchainImages[imageIndex];
	uint32_t width = ctx->swapchainExtent.width;
	uint32_t height = ctx->swapchainExtent.height;

	// Create destination image in linear tiling for CPU access
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = ctx->swapchainImageFormat;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.mipLevels = 1;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkImage dstImage;
	if (vkCreateImage(ctx->device, &imageInfo, NULL, &dstImage) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create destination image\n");
		return 0;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ctx->device, dstImage, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);

	uint32_t memoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
			(memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == UINT32_MAX) {
		fprintf(stderr, "Failed to find suitable memory type\n");
		vkDestroyImage(ctx->device, dstImage, NULL);
		return 0;
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory dstImageMemory;
	if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &dstImageMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate image memory\n");
		vkDestroyImage(ctx->device, dstImage, NULL);
		return 0;
	}

	vkBindImageMemory(ctx->device, dstImage, dstImageMemory, 0);

	// Create a one-time command buffer for the copy
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandPool = ctx->commandPool;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &cmdBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	// Transition dst image to TRANSFER_DST_OPTIMAL
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = dstImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	// Transition src image to TRANSFER_SRC_OPTIMAL
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.image = srcImage;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy image
	VkImageCopy copyRegion = {0};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent.width = width;
	copyRegion.extent.height = height;
	copyRegion.extent.depth = 1;

	vkCmdCopyImage(cmdBuffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion);

	// Transition dst image to GENERAL for host access
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.image = dstImage;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	// Transition src image back to PRESENT_SRC
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.image = srcImage;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(cmdBuffer);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx->graphicsQueue);

	vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &cmdBuffer);

	// Map memory and read pixels
	VkImageSubresource subresource = {0};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(ctx->device, dstImage, &subresource, &layout);

	void* data;
	if (vkMapMemory(ctx->device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, &data) != VK_SUCCESS) {
		fprintf(stderr, "Failed to map memory\n");
		vkDestroyImage(ctx->device, dstImage, NULL);
		vkFreeMemory(ctx->device, dstImageMemory, NULL);
		return 0;
	}

	// Determine output format and temp file
	int isPng = strstr(filename, ".png") != NULL;
	char tempPpm[512];
	const char* outputFile = filename;

	if (isPng) {
		snprintf(tempPpm, sizeof(tempPpm), "%.*s.ppm", (int)(strlen(filename) - 4), filename);
		outputFile = tempPpm;
	}

	// Write PPM file
	FILE* file = fopen(outputFile, "wb");
	if (!file) {
		fprintf(stderr, "Failed to open file for writing: %s\n", outputFile);
		vkUnmapMemory(ctx->device, dstImageMemory);
		vkDestroyImage(ctx->device, dstImage, NULL);
		vkFreeMemory(ctx->device, dstImageMemory, NULL);
		return 0;
	}

	fprintf(file, "P6\n%u %u\n255\n", width, height);

	const uint8_t* imageData = (const uint8_t*)data;
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t* row = imageData + layout.offset + y * layout.rowPitch;
		for (uint32_t x = 0; x < width; x++) {
			fputc(row[x * 4 + 2], file); // R
			fputc(row[x * 4 + 1], file); // G
			fputc(row[x * 4 + 0], file); // B
		}
	}

	fclose(file);
	vkUnmapMemory(ctx->device, dstImageMemory);
	vkDestroyImage(ctx->device, dstImage, NULL);
	vkFreeMemory(ctx->device, dstImageMemory, NULL);

	// Convert to PNG if needed
	if (isPng) {
		char cmd[1024];
		snprintf(cmd, sizeof(cmd), "convert %s %s && rm %s", tempPpm, filename, tempPpm);
		if (system(cmd) != 0) {
			fprintf(stderr, "Failed to convert to PNG: %s\n", filename);
			return 0;
		}
	}

	printf("Screenshot saved to %s\n", filename);
	return 1;
}
