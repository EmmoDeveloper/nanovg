#ifndef NVG_VK_TEXTURE_H
#define NVG_VK_TEXTURE_H

#include "nvg_vk_types.h"

// Texture type enum (from NanoVG)
enum NVGtextureType {
	NVG_TEXTURE_ALPHA = 0x01,
	NVG_TEXTURE_RGBA = 0x02,
};

// Texture management functions
int nvgvk_create_texture(void* userPtr, int type, int w, int h,
                         int imageFlags, const unsigned char* data);
void nvgvk_delete_texture(void* userPtr, int image);
int nvgvk_update_texture(void* userPtr, int image, int x, int y,
                         int w, int h, const unsigned char* data);
int nvgvk_get_texture_size(void* userPtr, int image, int* w, int* h);

// Internal helper functions
int nvgvk__allocate_texture(NVGVkContext* vk);
VkFormat nvgvk__get_vk_format(int type);
int nvgvk__create_sampler(NVGVkContext* vk, NVGVkTexture* tex, int imageFlags);

#endif // NVG_VK_TEXTURE_H
