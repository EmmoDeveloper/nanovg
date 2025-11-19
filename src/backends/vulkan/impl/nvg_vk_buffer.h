#ifndef NVG_VK_BUFFER_H
#define NVG_VK_BUFFER_H

#include "nvg_vk_types.h"

// Buffer management functions
int nvgvk_buffer_create(NVGVkContext* vk, NVGVkBuffer* buffer,
                        VkDeviceSize size, VkBufferUsageFlags usage);
void nvgvk_buffer_destroy(NVGVkContext* vk, NVGVkBuffer* buffer);
int nvgvk_buffer_upload(NVGVkContext* vk, NVGVkBuffer* buffer,
                        const void* data, VkDeviceSize size);
int nvgvk_buffer_reserve(NVGVkContext* vk, NVGVkBuffer* buffer,
                         VkDeviceSize size);
void nvgvk_buffer_reset(NVGVkBuffer* buffer);

#endif // NVG_VK_BUFFER_H
