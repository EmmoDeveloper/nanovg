// Async Glyph Upload System
// Non-blocking atlas updates using async compute or transfer queue
//
// This optimization moves glyph uploads off the main graphics queue,
// allowing rendering to proceed while uploads happen in the background.

#ifndef NANOVG_VK_ASYNC_UPLOAD_H
#define NANOVG_VK_ASYNC_UPLOAD_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define VKNVG_MAX_UPLOAD_FRAMES 3		// Triple buffering for uploads
#define VKNVG_ASYNC_UPLOAD_BATCH_SIZE 128		// Max pending uploads per frame

// Upload command
typedef struct VKNVGuploadCommand {
	VkBuffer stagingBuffer;
	VkImage dstImage;
	uint32_t x, y;
	uint32_t width, height;
	VkDeviceSize stagingOffset;
} VKNVGuploadCommand;

// Per-frame upload resources
typedef struct VKNVGuploadFrame {
	VkCommandBuffer commandBuffer;
	VkFence fence;
	VkSemaphore completeSemaphore;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	void* stagingMapped;
	VkDeviceSize stagingSize;
	VkDeviceSize stagingUsed;

	VKNVGuploadCommand commands[VKNVG_ASYNC_UPLOAD_BATCH_SIZE];
	uint32_t commandCount;

	VkBool32 inUse;
	uint64_t frameNumber;
} VKNVGuploadFrame;

// Async upload context
typedef struct VKNVGasyncUpload {
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue transferQueue;		// Dedicated transfer or async compute queue
	uint32_t transferQueueFamily;

	VkCommandPool commandPool;

	VKNVGuploadFrame frames[VKNVG_MAX_UPLOAD_FRAMES];
	uint32_t currentFrame;
	uint64_t frameCounter;

	pthread_mutex_t submitMutex;

	// Statistics
	uint32_t uploadsSubmitted;
	uint32_t uploadsCompleted;
	uint32_t stallsAvoided;
} VKNVGasyncUpload;

// Function declarations
uint32_t vknvg__findMemoryTypeAsync(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);
VkResult vknvg__createUploadStagingBuffer(VKNVGasyncUpload* ctx,
                                          VKNVGuploadFrame* frame,
                                          VkDeviceSize size);
VKNVGasyncUpload* vknvg__createAsyncUpload(VkDevice device,
                                           VkPhysicalDevice physicalDevice,
                                           VkQueue transferQueue,
                                           uint32_t transferQueueFamily,
                                           VkDeviceSize stagingBufferSize);
void vknvg__destroyAsyncUpload(VKNVGasyncUpload* ctx);
VkResult vknvg__queueAsyncUpload(VKNVGasyncUpload* ctx,
                                VkImage dstImage,
                                const void* data,
                                VkDeviceSize dataSize,
                                uint32_t x, uint32_t y,
                                uint32_t width, uint32_t height);
VkResult vknvg__submitAsyncUploads(VKNVGasyncUpload* ctx);
VkSemaphore vknvg__getUploadCompleteSemaphore(VKNVGasyncUpload* ctx);
void vknvg__getAsyncUploadStats(VKNVGasyncUpload* ctx,
                               uint32_t* submitted,
                               uint32_t* completed,
                               uint32_t* stallsAvoided);

#endif // NANOVG_VK_ASYNC_UPLOAD_H
