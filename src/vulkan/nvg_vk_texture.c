#include "nvg_vk_texture.h"
#include "nvg_vk_buffer.h"
#include <stdio.h>
#include <string.h>

// Helper: Get Vulkan format from texture type
VkFormat nvgvk__get_vk_format(int type)
{
	if (type == NVG_TEXTURE_RGBA) {
		return VK_FORMAT_R8G8B8A8_UNORM;
	}
	return VK_FORMAT_R8_UNORM;  // ALPHA
}

// Helper: Find memory type
static uint32_t nvgvk__find_memory_type(NVGVkContext* vk, uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return UINT32_MAX;
}

// Allocate a texture slot
int nvgvk__allocate_texture(NVGVkContext* vk)
{
	for (int i = 0; i < NVGVK_MAX_TEXTURES; i++) {
		if (vk->textures[i].image == VK_NULL_HANDLE) {
			memset(&vk->textures[i], 0, sizeof(NVGVkTexture));
			vk->textureCount++;
			return i;
		}
	}
	fprintf(stderr, "NanoVG Vulkan: No free texture slots\n");
	return -1;
}

// Create sampler for texture
int nvgvk__create_sampler(NVGVkContext* vk, NVGVkTexture* tex, int imageFlags)
{
	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Use nearest if requested, otherwise linear
	if (imageFlags & (1 << 5)) {  // NVG_IMAGE_NEAREST
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
	} else {
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
	}

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	// Handle repeat flags
	VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	if ((imageFlags & (1 << 1)) || (imageFlags & (1 << 2))) {  // REPEATX or REPEATY
		addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (vkCreateSampler(vk->device, &samplerInfo, NULL, &tex->sampler) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create sampler\n");
		return 0;
	}

	return 1;
}

int nvgvk_create_texture(void* userPtr, int type, int w, int h,
                         int imageFlags, const unsigned char* data)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || w <= 0 || h <= 0) {
		return -1;
	}

	// Allocate texture slot
	int id = nvgvk__allocate_texture(vk);
	if (id < 0) {
		return -1;
	}

	NVGVkTexture* tex = &vk->textures[id];
	tex->width = w;
	tex->height = h;
	tex->type = type;
	tex->flags = imageFlags;

	VkFormat format = nvgvk__get_vk_format(type);
	int bytesPerPixel = (type == NVG_TEXTURE_RGBA) ? 4 : 1;

	// Create image
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent.width = w;
	imageInfo.extent.height = h;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(vk->device, &imageInfo, NULL, &tex->image) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create image\n");
		return -1;
	}

	// Allocate memory
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(vk->device, tex->image, &memReq);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = nvgvk__find_memory_type(vk, memReq.memoryTypeBits,
	                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (allocInfo.memoryTypeIndex == UINT32_MAX ||
	    vkAllocateMemory(vk->device, &allocInfo, NULL, &tex->memory) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate image memory\n");
		vkDestroyImage(vk->device, tex->image, NULL);
		tex->image = VK_NULL_HANDLE;
		return -1;
	}

	vkBindImageMemory(vk->device, tex->image, tex->memory, 0);

	// Create image view
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = tex->image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(vk->device, &viewInfo, NULL, &tex->imageView) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create image view\n");
		vkFreeMemory(vk->device, tex->memory, NULL);
		vkDestroyImage(vk->device, tex->image, NULL);
		tex->image = VK_NULL_HANDLE;
		return -1;
	}

	// Create sampler
	if (!nvgvk__create_sampler(vk, tex, imageFlags)) {
		vkDestroyImageView(vk->device, tex->imageView, NULL);
		vkFreeMemory(vk->device, tex->memory, NULL);
		vkDestroyImage(vk->device, tex->image, NULL);
		tex->image = VK_NULL_HANDLE;
		return -1;
	}

	// Upload data if provided
	if (data != NULL) {
		if (!nvgvk_update_texture(userPtr, id, 0, 0, w, h, data)) {
			nvgvk_delete_texture(userPtr, id);
			return -1;
		}
	}

	return id;
}

void nvgvk_delete_texture(void* userPtr, int image)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || image < 0 || image >= NVGVK_MAX_TEXTURES) {
		return;
	}

	NVGVkTexture* tex = &vk->textures[image];
	if (tex->image == VK_NULL_HANDLE) {
		return;
	}

	vkDeviceWaitIdle(vk->device);

	if (tex->sampler) {
		vkDestroySampler(vk->device, tex->sampler, NULL);
	}
	if (tex->imageView) {
		vkDestroyImageView(vk->device, tex->imageView, NULL);
	}
	if (tex->memory) {
		vkFreeMemory(vk->device, tex->memory, NULL);
	}
	if (tex->image) {
		vkDestroyImage(vk->device, tex->image, NULL);
	}

	memset(tex, 0, sizeof(NVGVkTexture));
	vk->textureCount--;
}

int nvgvk_update_texture(void* userPtr, int image, int x, int y,
                         int w, int h, const unsigned char* data)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || image < 0 || image >= NVGVK_MAX_TEXTURES || !data) {
		return 0;
	}

	NVGVkTexture* tex = &vk->textures[image];
	if (tex->image == VK_NULL_HANDLE) {
		return 0;
	}

	int bytesPerPixel = (tex->type == NVG_TEXTURE_RGBA) ? 4 : 1;
	VkDeviceSize dataSize = w * h * bytesPerPixel;

	// Create staging buffer
	NVGVkBuffer stagingBuffer;
	if (!nvgvk_buffer_create(vk, &stagingBuffer, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
		return 0;
	}

	// Copy data to staging buffer
	memcpy(stagingBuffer.mapped, data, dataSize);

	// Record commands for image upload
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(vk->commandBuffer, &beginInfo);

	// Transition to transfer dst
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = tex->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(vk->commandBuffer,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy buffer to image
	VkBufferImageCopy region = {0};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = x;
	region.imageOffset.y = y;
	region.imageExtent.width = w;
	region.imageExtent.height = h;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(vk->commandBuffer, stagingBuffer.buffer, tex->image,
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition to shader read
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(vk->commandBuffer,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(vk->commandBuffer);

	// Submit and wait
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vk->commandBuffer;

	vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->queue);

	// Cleanup staging buffer
	nvgvk_buffer_destroy(vk, &stagingBuffer);

	return 1;
}

int nvgvk_get_texture_size(void* userPtr, int image, int* w, int* h)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	if (!vk || image < 0 || image >= NVGVK_MAX_TEXTURES) {
		return 0;
	}

	NVGVkTexture* tex = &vk->textures[image];
	if (tex->image == VK_NULL_HANDLE) {
		return 0;
	}

	if (w) *w = tex->width;
	if (h) *h = tex->height;
	return 1;
}
