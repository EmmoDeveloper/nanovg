#include "window_utils.h"
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include "nanovg.h"
#include "nanovg_vk.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../example/stb_image_write.h"

void saveScreenshot(WindowVulkanContext* ctx, uint32_t imageIndex, const char* filename)
{
	VkDevice device = ctx->device;
	VkImage srcImage = ctx->swapchainImages[imageIndex];
	VkExtent2D extent = ctx->swapchainExtent;

	// Create staging image (CPU-accessible)
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = ctx->swapchainImageFormat;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage dstImage;
	vkCreateImage(device, &imageInfo, NULL, &dstImage);

	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, dstImage, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProps);

	uint32_t memTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReqs.memoryTypeBits & (1 << i)) &&
			(memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
			memTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	VkDeviceMemory dstMemory;
	vkAllocateMemory(device, &allocInfo, NULL, &dstMemory);
	vkBindImageMemory(device, dstImage, dstMemory, 0);

	// Create command buffer for copy
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = ctx->commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuf;
	vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmdBuf);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuf, &beginInfo);

	// Transition dst image to TRANSFER_DST_OPTIMAL
	VkImageMemoryBarrier barrier1 = {0};
	barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier1.image = dstImage;
	barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier1.subresourceRange.levelCount = 1;
	barrier1.subresourceRange.layerCount = 1;
	barrier1.srcAccessMask = 0;
	barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier1);

	// Transition src image to TRANSFER_SRC_OPTIMAL
	VkImageMemoryBarrier barrier2 = {0};
	barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier2.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.image = srcImage;
	barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier2.subresourceRange.levelCount = 1;
	barrier2.subresourceRange.layerCount = 1;
	barrier2.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier2);

	// Copy image
	VkImageCopy region = {0};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.extent.width = extent.width;
	region.extent.height = extent.height;
	region.extent.depth = 1;

	vkCmdCopyImage(cmdBuf, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition dst to GENERAL for reading
	VkImageMemoryBarrier barrier3 = {0};
	barrier3.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier3.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier3.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier3.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier3.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier3.image = dstImage;
	barrier3.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier3.subresourceRange.levelCount = 1;
	barrier3.subresourceRange.layerCount = 1;
	barrier3.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier3.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

	vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier3);

	vkEndCommandBuffer(cmdBuf);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ctx->graphicsQueue);

	// Map memory and read pixels
	VkImageSubresource subresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(device, dstImage, &subresource, &layout);

	void* data;
	vkMapMemory(device, dstMemory, 0, VK_WHOLE_SIZE, 0, &data);

	// Convert BGRA to RGBA
	unsigned char* pixels = (unsigned char*)malloc(extent.width * extent.height * 4);
	for (uint32_t y = 0; y < extent.height; y++) {
		unsigned char* row = (unsigned char*)data + y * layout.rowPitch;
		for (uint32_t x = 0; x < extent.width; x++) {
			uint32_t offset = x * 4;
			pixels[(y * extent.width + x) * 4 + 0] = row[offset + 2];  // R
			pixels[(y * extent.width + x) * 4 + 1] = row[offset + 1];  // G
			pixels[(y * extent.width + x) * 4 + 2] = row[offset + 0];  // B
			pixels[(y * extent.width + x) * 4 + 3] = row[offset + 3];  // A
		}
	}

	stbi_write_png(filename, extent.width, extent.height, 4, pixels, extent.width * 4);
	printf("Screenshot saved to %s\n", filename);

	free(pixels);
	vkUnmapMemory(device, dstMemory);
	vkFreeCommandBuffers(device, ctx->commandPool, 1, &cmdBuf);
	vkDestroyImage(device, dstImage, NULL);
	vkFreeMemory(device, dstMemory, NULL);
}

int main(void)
{
	printf("Creating Vulkan window context...\n");
	fflush(stdout);
	WindowVulkanContext* ctx = window_create_context(800, 600, "NanoVG Vulkan Test");
	if (!ctx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	printf("Window context created successfully!\n");
	fflush(stdout);

	printf("Creating NanoVG context...\n");
	fflush(stdout);
	// Don't use INTERNAL_SYNC - we'll submit NanoVG's command buffer ourselves
	// Use INTERNAL_RENDER_PASS so NanoVG creates its own render pass
	// Enable STENCIL_STROKES for proper fill rendering
	NVGcontext* vg = window_create_nanovg_context(ctx, NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_INTERNAL_RENDER_PASS);
	if (!vg) {
		fprintf(stderr, "Failed to create NanoVG context\n");
		window_destroy_context(ctx);
		return 1;
	}

	printf("NanoVG context created successfully!\n");
	fflush(stdout);

	printf("Entering render loop...\n");
	fflush(stdout);
	float time = 0.0f;
	int frameCount = 0;
	int snapshotTaken = 0;

	uint32_t frameIndex = 0;
	while (!window_should_close(ctx)) {
		window_poll_events();

		uint32_t imageIndex;
		VkCommandBuffer cmdBuf;

		if (!window_begin_frame(ctx, &imageIndex, &cmdBuf)) {
			continue;
		}

		// Tell NanoVG which frame we're on so it uses the correct command buffer
		nvgVkSetCurrentFrame(vg, frameIndex);

		VkImageView imageView = window_get_swapchain_image_view(ctx, imageIndex);
		VkImageView depthStencilView = window_get_depth_stencil_image_view(ctx);
		nvgVkSetRenderTargets(vg, imageView, depthStencilView);

		int width, height;
		window_get_framebuffer_size(ctx, &width, &height);

		nvgBeginFrame(vg, width, height, 1.0f);

		nvgBeginPath(vg);
		nvgRect(vg, 100, 100, 200, 200);
		nvgFillColor(vg, nvgRGBA(255, 100, 100, 200));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, 400, 200, 80);
		nvgFillColor(vg, nvgRGBA(100, 200, 255, 200));
		nvgFill(vg);

		float x = 400 + cosf(time) * 150;
		float y = 400 + sinf(time) * 150;
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, 40);
		nvgFillColor(vg, nvgRGBA(100, 255, 100, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, 500, 400, 180, 120, 10);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);

		nvgEndFrame(vg);

		// Get NanoVG's command buffer after rendering
		VkCommandBuffer nvgCmdBuf = nvgVkGetCommandBuffer(vg);

		window_end_frame(ctx, imageIndex, nvgCmdBuf);

		time += 0.016f;
		frameIndex = (frameIndex + 1) % 2;  // Match maxFramesInFlight
		frameCount++;

		// Take snapshot after 10 frames and exit
		if (frameCount == 10 && !snapshotTaken) {
			printf("Taking snapshot...\n");
			fflush(stdout);
			snapshotTaken = 1;
			saveScreenshot(ctx, imageIndex, "/tmp/test-vulkan.png");
			break;  // Exit after snapshot
		}
	}

	printf("Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(ctx);

	printf("Test completed successfully\n");
	return 0;
}
