#ifndef NVG_VK_CONTEXT_H
#define NVG_VK_CONTEXT_H

#include "nvg_vk_types.h"

// Context creation parameters
typedef struct NVGVkCreateInfo {
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue queue;
	VkCommandPool commandPool;
	int flags;
} NVGVkCreateInfo;

// Context lifecycle functions
int nvgvk_create(void* userPtr, const NVGVkCreateInfo* createInfo);
void nvgvk_delete(void* userPtr);

// Frame management
void nvgvk_viewport(void* userPtr, float width, float height, float devicePixelRatio);
void nvgvk_cancel(void* userPtr);
void nvgvk_flush(void* userPtr);

#endif // NVG_VK_CONTEXT_H
