// Async Glyph Upload System Implementation
// Non-blocking atlas updates using async compute or transfer queue

#include "nanovg_vk_async_upload.h"

// Find memory type
uint32_t vknvg__findMemoryTypeAsync(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return UINT32_MAX;
}

// Create staging buffer for uploads
VkResult vknvg__createUploadStagingBuffer(VKNVGasyncUpload* ctx,
                                          VKNVGuploadFrame* frame,
                                          VkDeviceSize size)
{
	VkResult result;

	// Create staging buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(ctx->device, &bufferInfo, NULL, &frame->stagingBuffer);
	if (result != VK_SUCCESS) return result;

	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(ctx->device, frame->stagingBuffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = vknvg__findMemoryTypeAsync(
		ctx->physicalDevice,
		memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(ctx->device, frame->stagingBuffer, NULL);
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;
	}

	result = vkAllocateMemory(ctx->device, &allocInfo, NULL, &frame->stagingMemory);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(ctx->device, frame->stagingBuffer, NULL);
		return result;
	}

	vkBindBufferMemory(ctx->device, frame->stagingBuffer, frame->stagingMemory, 0);

	// Map memory
	result = vkMapMemory(ctx->device, frame->stagingMemory, 0, size, 0, &frame->stagingMapped);
	if (result != VK_SUCCESS) {
		vkFreeMemory(ctx->device, frame->stagingMemory, NULL);
		vkDestroyBuffer(ctx->device, frame->stagingBuffer, NULL);
		return result;
	}

	frame->stagingSize = size;
	frame->stagingUsed = 0;

	return VK_SUCCESS;
}

// Create async upload context
VKNVGasyncUpload* vknvg__createAsyncUpload(VkDevice device,
                                           VkPhysicalDevice physicalDevice,
                                           VkQueue transferQueue,
                                           uint32_t transferQueueFamily,
                                           VkDeviceSize stagingBufferSize)
{
	VKNVGasyncUpload* ctx = (VKNVGasyncUpload*)malloc(sizeof(VKNVGasyncUpload));
	if (!ctx) return NULL;

	memset(ctx, 0, sizeof(VKNVGasyncUpload));
	ctx->device = device;
	ctx->physicalDevice = physicalDevice;
	ctx->transferQueue = transferQueue;
	ctx->transferQueueFamily = transferQueueFamily;

	pthread_mutex_init(&ctx->submitMutex, NULL);

	// Create command pool
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = transferQueueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
		free(ctx);
		return NULL;
	}

	// Create per-frame resources
	for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
		VKNVGuploadFrame* frame = &ctx->frames[i];

		// Allocate command buffer
		VkCommandBufferAllocateInfo allocInfo = {0};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = ctx->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device, &allocInfo, &frame->commandBuffer) != VK_SUCCESS) {
			goto cleanup;
		}

		// Create fence
		VkFenceCreateInfo fenceInfo = {0};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

		if (vkCreateFence(device, &fenceInfo, NULL, &frame->fence) != VK_SUCCESS) {
			goto cleanup;
		}

		// Create semaphore
		VkSemaphoreCreateInfo semaphoreInfo = {0};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &frame->completeSemaphore) != VK_SUCCESS) {
			goto cleanup;
		}

		// Create staging buffer
		if (vknvg__createUploadStagingBuffer(ctx, frame, stagingBufferSize) != VK_SUCCESS) {
			goto cleanup;
		}

		frame->inUse = VK_FALSE;
	}

	return ctx;

cleanup:
	// Cleanup on failure
	for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
		VKNVGuploadFrame* frame = &ctx->frames[i];
		if (frame->stagingMapped) {
			vkUnmapMemory(device, frame->stagingMemory);
		}
		if (frame->stagingMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, frame->stagingMemory, NULL);
		}
		if (frame->stagingBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, frame->stagingBuffer, NULL);
		}
		if (frame->completeSemaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, frame->completeSemaphore, NULL);
		}
		if (frame->fence != VK_NULL_HANDLE) {
			vkDestroyFence(device, frame->fence, NULL);
		}
	}
	if (ctx->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, ctx->commandPool, NULL);
	}
	pthread_mutex_destroy(&ctx->submitMutex);
	free(ctx);
	return NULL;
}

// Destroy async upload context
void vknvg__destroyAsyncUpload(VKNVGasyncUpload* ctx)
{
	if (!ctx) return;

	// Wait for all uploads to complete
	for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
		VKNVGuploadFrame* frame = &ctx->frames[i];
		if (frame->fence != VK_NULL_HANDLE) {
			vkWaitForFences(ctx->device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
		}
	}

	// Destroy resources
	for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
		VKNVGuploadFrame* frame = &ctx->frames[i];
		if (frame->stagingMapped) {
			vkUnmapMemory(ctx->device, frame->stagingMemory);
		}
		if (frame->stagingMemory != VK_NULL_HANDLE) {
			vkFreeMemory(ctx->device, frame->stagingMemory, NULL);
		}
		if (frame->stagingBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(ctx->device, frame->stagingBuffer, NULL);
		}
		if (frame->completeSemaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(ctx->device, frame->completeSemaphore, NULL);
		}
		if (frame->fence != VK_NULL_HANDLE) {
			vkDestroyFence(ctx->device, frame->fence, NULL);
		}
	}

	if (ctx->commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
	}

	pthread_mutex_destroy(&ctx->submitMutex);
	free(ctx);
}

// Queue an upload
VkResult vknvg__queueAsyncUpload(VKNVGasyncUpload* ctx,
                                VkImage dstImage,
                                const void* data,
                                VkDeviceSize dataSize,
                                uint32_t x, uint32_t y,
                                uint32_t width, uint32_t height)
{
	if (!ctx) return VK_ERROR_INITIALIZATION_FAILED;

	VKNVGuploadFrame* frame = &ctx->frames[ctx->currentFrame];

	// Check if we have space in staging buffer
	if (frame->stagingUsed + dataSize > frame->stagingSize) {
		return VK_ERROR_OUT_OF_DEVICE_MEMORY;  // Staging buffer full
	}

	// Check if we have space for more commands
	if (frame->commandCount >= VKNVG_ASYNC_UPLOAD_BATCH_SIZE) {
		return VK_ERROR_TOO_MANY_OBJECTS;
	}

	// Copy data to staging buffer
	memcpy((uint8_t*)frame->stagingMapped + frame->stagingUsed, data, dataSize);

	// Record upload command
	VKNVGuploadCommand* cmd = &frame->commands[frame->commandCount++];
	cmd->stagingBuffer = frame->stagingBuffer;
	cmd->dstImage = dstImage;
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	cmd->stagingOffset = frame->stagingUsed;

	frame->stagingUsed += dataSize;

	return VK_SUCCESS;
}

// Submit uploads for current frame
VkResult vknvg__submitAsyncUploads(VKNVGasyncUpload* ctx)
{
	if (!ctx) return VK_ERROR_INITIALIZATION_FAILED;

	VKNVGuploadFrame* frame = &ctx->frames[ctx->currentFrame];

	if (frame->commandCount == 0) {
		return VK_SUCCESS;  // Nothing to upload
	}

	// Wait for fence from previous use of this frame
	vkWaitForFences(ctx->device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
	vkResetFences(ctx->device, 1, &frame->fence);

	// Reset command buffer
	vkResetCommandBuffer(frame->commandBuffer, 0);

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult result = vkBeginCommandBuffer(frame->commandBuffer, &beginInfo);
	if (result != VK_SUCCESS) return result;

	// Record upload commands
	for (uint32_t i = 0; i < frame->commandCount; i++) {
		VKNVGuploadCommand* cmd = &frame->commands[i];

		// Transition image to transfer dst
		VkImageMemoryBarrier barrier = {0};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = cmd->dstImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(frame->commandBuffer,
		                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);

		// Copy buffer to image
		VkBufferImageCopy region = {0};
		region.bufferOffset = cmd->stagingOffset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = cmd->x;
		region.imageOffset.y = cmd->y;
		region.imageOffset.z = 0;
		region.imageExtent.width = cmd->width;
		region.imageExtent.height = cmd->height;
		region.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(frame->commandBuffer,
		                       cmd->stagingBuffer,
		                       cmd->dstImage,
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                       1, &region);

		// Transition back to shader read
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(frame->commandBuffer,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT,
		                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);
	}

	vkEndCommandBuffer(frame->commandBuffer);

	// Submit to transfer queue
	pthread_mutex_lock(&ctx->submitMutex);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame->commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame->completeSemaphore;

	result = vkQueueSubmit(ctx->transferQueue, 1, &submitInfo, frame->fence);

	pthread_mutex_unlock(&ctx->submitMutex);

	if (result != VK_SUCCESS) return result;

	ctx->uploadsSubmitted += frame->commandCount;

	// Mark frame as in use
	frame->inUse = VK_TRUE;
	frame->frameNumber = ctx->frameCounter++;

	// Reset for next use
	frame->commandCount = 0;
	frame->stagingUsed = 0;

	// Move to next frame
	ctx->currentFrame = (ctx->currentFrame + 1) % VKNVG_MAX_UPLOAD_FRAMES;

	return VK_SUCCESS;
}

// Get semaphore to wait on for latest uploads
VkSemaphore vknvg__getUploadCompleteSemaphore(VKNVGasyncUpload* ctx)
{
	if (!ctx) return VK_NULL_HANDLE;

	// Find most recent submitted frame
	uint64_t latestFrame = 0;
	VkSemaphore latestSemaphore = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
		VKNVGuploadFrame* frame = &ctx->frames[i];
		if (frame->inUse && frame->frameNumber > latestFrame) {
			latestFrame = frame->frameNumber;
			latestSemaphore = frame->completeSemaphore;
		}
	}

	return latestSemaphore;
}

// Get statistics
void vknvg__getAsyncUploadStats(VKNVGasyncUpload* ctx,
                               uint32_t* submitted,
                               uint32_t* completed,
                               uint32_t* stallsAvoided)
{
	if (!ctx) return;

	if (submitted) *submitted = ctx->uploadsSubmitted;
	if (stallsAvoided) *stallsAvoided = ctx->stallsAvoided;

	if (completed) {
		// Count completed uploads by checking fences
		uint32_t count = 0;
		for (uint32_t i = 0; i < VKNVG_MAX_UPLOAD_FRAMES; i++) {
			VKNVGuploadFrame* frame = &ctx->frames[i];
			if (frame->inUse) {
				VkResult status = vkGetFenceStatus(ctx->device, frame->fence);
				if (status == VK_SUCCESS) {
					frame->inUse = VK_FALSE;
					count++;
				}
			}
		}
		ctx->uploadsCompleted += count;
		*completed = ctx->uploadsCompleted;
	}
}
