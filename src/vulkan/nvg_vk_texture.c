#include "nvg_vk_texture.h"
#include "nvg_vk_buffer.h"
#include "../nanovg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Helper: Get Vulkan format from texture type
VkFormat nvgvk__get_vk_format(int type)
{
	if (type == NVG_TEXTURE_LCD_SUBPIXEL) {  // LCD subpixel - use RGB order
		return VK_FORMAT_R8G8B8A8_UNORM;
	}
	if (type == NVG_TEXTURE_RGBA || type == NVG_TEXTURE_MSDF) {  // RGBA or MSDF
		return VK_FORMAT_R8G8B8A8_UNORM;  // Data is in RGBA format
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

// Initialize texture descriptor system (descriptor set layout and pool)
int nvgvk__init_texture_descriptors(NVGVkContext* vk)
{
	// Create descriptor set layout for textures (uniform buffer + sampler)
	VkDescriptorSetLayoutBinding bindings[2] = {0};

	// Binding 0: Uniform buffer (viewSize)
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Binding 1: Combined image sampler
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, NULL, &vk->textureDescriptorSetLayout) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create texture descriptor set layout\n");
		return 0;
	}

	// Create descriptor pool for all texture descriptor sets
	VkDescriptorPoolSize poolSizes[2] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = NVGVK_MAX_TEXTURES;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = NVGVK_MAX_TEXTURES;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = NVGVK_MAX_TEXTURES;

	if (vkCreateDescriptorPool(vk->device, &poolInfo, NULL, &vk->textureDescriptorPool) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to create texture descriptor pool\n");
		vkDestroyDescriptorSetLayout(vk->device, vk->textureDescriptorSetLayout, NULL);
		vk->textureDescriptorSetLayout = VK_NULL_HANDLE;
		return 0;
	}

	return 1;
}

// Destroy texture descriptor system
void nvgvk__destroy_texture_descriptors(NVGVkContext* vk)
{
	if (vk->textureDescriptorPool) {
		vkDestroyDescriptorPool(vk->device, vk->textureDescriptorPool, NULL);
		vk->textureDescriptorPool = VK_NULL_HANDLE;
	}
	if (vk->textureDescriptorSetLayout) {
		vkDestroyDescriptorSetLayout(vk->device, vk->textureDescriptorSetLayout, NULL);
		vk->textureDescriptorSetLayout = VK_NULL_HANDLE;
	}
}

// Allocate and initialize descriptor set for a texture
int nvgvk__allocate_texture_descriptor_set(NVGVkContext* vk, NVGVkTexture* tex)
{
	// Allocate descriptor set
	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk->textureDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &vk->textureDescriptorSetLayout;

	if (vkAllocateDescriptorSets(vk->device, &allocInfo, &tex->descriptorSet) != VK_SUCCESS) {
		fprintf(stderr, "NanoVG Vulkan: Failed to allocate texture descriptor set\n");
		return 0;
	}

	// Update descriptor set with uniform buffer and texture
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = vk->uniformBuffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(float) * 2; // viewSize

	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = tex->imageView;
	imageInfo.sampler = tex->sampler;

	VkWriteDescriptorSet writes[2] = {0};

	// Uniform buffer
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = tex->descriptorSet;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &bufferInfo;

	// Texture sampler
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = tex->descriptorSet;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(vk->device, 2, writes, 0, NULL);

	return 1;
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
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;  // Only 1 mipmap level (0), clamp to prevent sampling non-existent levels

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
	int bytesPerPixel = (type == NVG_TEXTURE_RGBA || type == 3) ? 4 : 1;  // RGBA/MSDF=4, ALPHA=1

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
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

	// Upload data if provided, or clear texture if NULL (prevents garbage/undefined content)
	if (data != NULL) {
		// Note: id is 0-based here, but nvgvk_update_texture expects 1-based
		if (!nvgvk_update_texture(userPtr, id + 1, 0, 0, w, h, data)) {
			nvgvk_delete_texture(userPtr, id + 1);
			return -1;
		}
	} else {
		// Clear texture to prevent undefined/garbage content
		printf("[nvgvk_create_texture] Clearing %dx%d atlas texture to zeros\n", w, h);
		unsigned char* zeros = (unsigned char*)calloc(w * h * bytesPerPixel, 1);
		if (zeros) {
			nvgvk_update_texture(userPtr, id + 1, 0, 0, w, h, zeros);
			free(zeros);
		}
	}

	// Allocate and initialize descriptor set for this texture
	if (!nvgvk__allocate_texture_descriptor_set(vk, tex)) {
		nvgvk_delete_texture(userPtr, id + 1);
		return -1;
	}

	// NanoVG uses 1-based texture IDs (0 = failure)
	return id + 1;
}

void nvgvk_delete_texture(void* userPtr, int image)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	// Convert from 1-based to 0-based
	int id = image - 1;
	if (!vk || id < 0 || id >= NVGVK_MAX_TEXTURES) {
		return;
	}

	NVGVkTexture* tex = &vk->textures[id];
	if (tex->image == VK_NULL_HANDLE) {
		return;
	}

	vkDeviceWaitIdle(vk->device);

	// Note: descriptor sets are automatically freed when pool is destroyed or reset
	// No need to explicitly free individual descriptor sets

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
	// Convert from 1-based to 0-based
	int id = image - 1;
	if (!vk || id < 0 || id >= NVGVK_MAX_TEXTURES || !data) {
		return 0;
	}

	NVGVkTexture* tex = &vk->textures[id];
	if (tex->image == VK_NULL_HANDLE) {
		return 0;
	}

	int bytesPerPixel = (tex->type == NVG_TEXTURE_RGBA || tex->type == NVG_TEXTURE_MSDF || tex->type == NVG_TEXTURE_LCD_SUBPIXEL) ? 4 : 1;
	VkDeviceSize dataSize = w * h * bytesPerPixel;

	// Create staging buffer
	NVGVkBuffer stagingBuffer;
	if (!nvgvk_buffer_create(vk, &stagingBuffer, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
		return 0;
	}

	// Copy data to staging buffer
	memcpy(stagingBuffer.mapped, data, dataSize);

	// If we're in a render pass, end it temporarily and flush
	int wasInRenderPass = vk->inRenderPass;
	VkRenderPass savedRenderPass = VK_NULL_HANDLE;
	VkFramebuffer savedFramebuffer = VK_NULL_HANDLE;

	if (wasInRenderPass) {
		savedRenderPass = vk->activeRenderPass;
		savedFramebuffer = vk->activeFramebuffer;
		vkCmdEndRenderPass(vk->commandBuffer);
		vkEndCommandBuffer(vk->commandBuffer);

		// Submit rendering commands and wait for completion
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vk->commandBuffer;

		// Submit without fence and use QueueWaitIdle to avoid fence state conflicts
		vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(vk->queue);

		vk->inRenderPass = 0;
	}

	// Allocate temporary command buffer for upload
	VkCommandBuffer uploadCmd;
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vk->device, &allocInfo, &uploadCmd) != VK_SUCCESS) {
		nvgvk_buffer_destroy(vk, &stagingBuffer);
		return 0;
	}

	// Record commands for image upload
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(uploadCmd, &beginInfo);

	// Transition to transfer dst
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	// If texture was already used, it's in SHADER_READ_ONLY_OPTIMAL, otherwise UNDEFINED
	barrier.oldLayout = (tex->flags & 0x8000) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	printf("[LAYOUT] Texture %d transition: %s -> TRANSFER_DST\n",
		id, (tex->flags & 0x8000) ? "SHADER_READ" : "UNDEFINED");
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = tex->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	// Synchronize previous shader reads before starting transfer write
	barrier.srcAccessMask = (tex->flags & 0x8000) ? VK_ACCESS_SHADER_READ_BIT : 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(uploadCmd,
	                     (tex->flags & 0x8000) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
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

	vkCmdCopyBufferToImage(uploadCmd, stagingBuffer.buffer, tex->image,
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition to shader read
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	printf("[LAYOUT] Texture %d transition: TRANSFER_DST -> SHADER_READ\n", id);

	vkCmdPipelineBarrier(uploadCmd,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(uploadCmd);

	// Mark texture as initialized
	tex->flags |= 0x8000;

	// Submit upload commands with fence for synchronization
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &uploadCmd;

	// Reset fence before submitting (fence was signaled by render submit above)
	vkResetFences(vk->device, 1, &vk->uploadFence);
	vkQueueSubmit(vk->queue, 1, &submitInfo, vk->uploadFence);

	// Wait for upload to complete before cleanup
	vkWaitForFences(vk->device, 1, &vk->uploadFence, VK_TRUE, UINT64_MAX);

	// Cleanup
	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &uploadCmd);
	nvgvk_buffer_destroy(vk, &stagingBuffer);

	// If we were in a render pass, restart the command buffer and render pass
	if (wasInRenderPass) {
		static int restart_count = 0;
		printf("[RENDER PASS RESTART #%d]\n", ++restart_count);

		VkCommandBufferBeginInfo restartBegin = {0};
		restartBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		restartBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(vk->commandBuffer, &restartBegin);

		// Restart render pass with saved info
		VkRenderPassBeginInfo rpBeginInfo = {0};
		rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpBeginInfo.renderPass = savedRenderPass;
		rpBeginInfo.framebuffer = savedFramebuffer;
		rpBeginInfo.renderArea = vk->renderArea;
		rpBeginInfo.clearValueCount = vk->clearValueCount;
		rpBeginInfo.pClearValues = vk->clearValues;

		vkCmdBeginRenderPass(vk->commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vk->inRenderPass = 1;

		// Restore viewport and scissor state
		vkCmdSetViewport(vk->commandBuffer, 0, 1, &vk->viewport);
		vkCmdSetScissor(vk->commandBuffer, 0, 1, &vk->scissor);

		// DO NOT rebind vertex buffer here - it will be bound in flush after vertices are uploaded
		printf("[RESTART] Skipping vertex buffer rebind (%d vertices accumulated so far)\n",
		       vk->vertexCount);

		// Invalidate pipeline state - command buffer was restarted so bindings are lost
		vk->currentPipeline = -1;
	}

	return 1;
}

int nvgvk_get_texture_size(void* userPtr, int image, int* w, int* h)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	// Convert from 1-based to 0-based
	int id = image - 1;
	if (!vk || id < 0 || id >= NVGVK_MAX_TEXTURES) {
		return 0;
	}

	NVGVkTexture* tex = &vk->textures[id];
	if (tex->image == VK_NULL_HANDLE) {
		return 0;
	}

	if (w) *w = tex->width;
	if (h) *h = tex->height;
	return 1;
}

int nvgvk_copy_texture(void* userPtr, int srcImage, int dstImage,
                       int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	NVGVkContext* vk = (NVGVkContext*)userPtr;
	// Convert from 1-based to 0-based
	int srcId = srcImage - 1;
	int dstId = dstImage - 1;

	if (!vk || srcId < 0 || srcId >= NVGVK_MAX_TEXTURES ||
	    dstId < 0 || dstId >= NVGVK_MAX_TEXTURES) {
		return 0;
	}

	NVGVkTexture* srcTex = &vk->textures[srcId];
	NVGVkTexture* dstTex = &vk->textures[dstId];

	if (srcTex->image == VK_NULL_HANDLE || dstTex->image == VK_NULL_HANDLE) {
		return 0;
	}

	// Save render pass state if active
	int wasInRenderPass = vk->inRenderPass;
	VkRenderPass savedRenderPass = VK_NULL_HANDLE;
	VkFramebuffer savedFramebuffer = VK_NULL_HANDLE;

	if (wasInRenderPass) {
		savedRenderPass = vk->activeRenderPass;
		savedFramebuffer = vk->activeFramebuffer;

		// End render pass
		vkCmdEndRenderPass(vk->commandBuffer);
		vkEndCommandBuffer(vk->commandBuffer);

		// Submit and wait
		VkSubmitInfo submitInfo = {0};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vk->commandBuffer;

		vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(vk->queue);

		vk->inRenderPass = 0;
	}

	// Allocate temporary command buffer for copy
	VkCommandBuffer copyCmd;
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vk->device, &allocInfo, &copyCmd) != VK_SUCCESS) {
		return 0;
	}

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(copyCmd, &beginInfo);

	// Transition source to transfer src
	VkImageMemoryBarrier srcBarrier = {0};
	srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	srcBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	srcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	srcBarrier.image = srcTex->image;
	srcBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	srcBarrier.subresourceRange.levelCount = 1;
	srcBarrier.subresourceRange.layerCount = 1;
	srcBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(copyCmd,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &srcBarrier);

	// Transition dest to transfer dst
	VkImageMemoryBarrier dstBarrier = {0};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.oldLayout = (dstTex->flags & 0x8000) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	dstBarrier.image = dstTex->image;
	dstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dstBarrier.subresourceRange.levelCount = 1;
	dstBarrier.subresourceRange.layerCount = 1;
	dstBarrier.srcAccessMask = (dstTex->flags & 0x8000) ? VK_ACCESS_SHADER_READ_BIT : 0;
	dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(copyCmd,
	                     (dstTex->flags & 0x8000) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &dstBarrier);

	// Copy image region
	VkImageCopy region = {0};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.srcOffset.x = srcX;
	region.srcOffset.y = srcY;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.dstOffset.x = dstX;
	region.dstOffset.y = dstY;
	region.extent.width = w;
	region.extent.height = h;
	region.extent.depth = 1;

	vkCmdCopyImage(copyCmd,
	               srcTex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               dstTex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &region);

	// Transition source back to shader read
	srcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	srcBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	srcBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(copyCmd,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &srcBarrier);

	// Transition dest to shader read
	dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dstBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(copyCmd,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &dstBarrier);

	vkEndCommandBuffer(copyCmd);

	// Mark dest texture as initialized
	dstTex->flags |= 0x8000;

	// Submit copy commands
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	vkResetFences(vk->device, 1, &vk->uploadFence);
	vkQueueSubmit(vk->queue, 1, &submitInfo, vk->uploadFence);

	// Wait for copy to complete
	vkWaitForFences(vk->device, 1, &vk->uploadFence, VK_TRUE, UINT64_MAX);

	// Free temporary command buffer
	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &copyCmd);

	// Restart render pass if needed
	if (wasInRenderPass) {
		VkCommandBufferBeginInfo restartBegin = {0};
		restartBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		restartBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(vk->commandBuffer, &restartBegin);

		VkRenderPassBeginInfo rpBeginInfo = {0};
		rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpBeginInfo.renderPass = savedRenderPass;
		rpBeginInfo.framebuffer = savedFramebuffer;
		rpBeginInfo.renderArea = vk->renderArea;
		rpBeginInfo.clearValueCount = vk->clearValueCount;
		rpBeginInfo.pClearValues = vk->clearValues;

		vkCmdBeginRenderPass(vk->commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vk->inRenderPass = 1;

		vkCmdSetViewport(vk->commandBuffer, 0, 1, &vk->viewport);
		vkCmdSetScissor(vk->commandBuffer, 0, 1, &vk->scissor);

		vk->currentPipeline = -1;
	}

	return 1;
}
