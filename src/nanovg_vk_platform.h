//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
// Copyright (c) 2025 NanoVG Vulkan Backend Contributors
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef NANOVG_VK_PLATFORM_H
#define NANOVG_VK_PLATFORM_H

#include <stdlib.h>
#include <string.h>
#include "nanovg_vk_internal.h"

static uint32_t vknvg__hashBlendState(VKNVGblend blend, int cacheSize)
{
	uint32_t hash = 0;
	hash = hash * 31 + (uint32_t)blend.srcRGB;
	hash = hash * 31 + (uint32_t)blend.dstRGB;
	hash = hash * 31 + (uint32_t)blend.srcAlpha;
	hash = hash * 31 + (uint32_t)blend.dstAlpha;
	return hash % cacheSize;
}

static int vknvg__compareBlendState(VKNVGblend a, VKNVGblend b)
{
	return a.srcRGB == b.srcRGB &&
	       a.dstRGB == b.dstRGB &&
	       a.srcAlpha == b.srcAlpha &&
	       a.dstAlpha == b.dstAlpha;
}

static VKNVGgpuTier vknvg__detectGpuTier(uint32_t vendorID, uint32_t deviceID, const char* deviceName, VkPhysicalDeviceType deviceType)
{
	if (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		return VKNVG_GPU_TIER_INTEGRATED;
	}

	if (vendorID == VKNVG_VENDOR_NVIDIA) {
		if ((deviceID >= 0x2684 && deviceID <= 0x2704) ||
		    strstr(deviceName, "RTX 4090") || strstr(deviceName, "RTX 4080")) {
			return VKNVG_GPU_TIER_ENTHUSIAST;
		}
		if ((deviceID >= 0x2204 && deviceID <= 0x2508) ||
		    strstr(deviceName, "RTX 3090") || strstr(deviceName, "RTX 3080") ||
		    strstr(deviceName, "RTX 3070")) {
			return VKNVG_GPU_TIER_HIGHEND;
		}
		if (strstr(deviceName, "RTX 3060") || strstr(deviceName, "RTX 2080") ||
		    strstr(deviceName, "RTX 2070") || strstr(deviceName, "RTX 2060")) {
			return VKNVG_GPU_TIER_MIDRANGE;
		}
		return VKNVG_GPU_TIER_ENTRY;
	}

	if (vendorID == VKNVG_VENDOR_AMD) {
		if (strstr(deviceName, "RX 7900") || strstr(deviceName, "RX 6950")) {
			return VKNVG_GPU_TIER_ENTHUSIAST;
		}
		if (strstr(deviceName, "RX 6900") || strstr(deviceName, "RX 6800")) {
			return VKNVG_GPU_TIER_HIGHEND;
		}
		if (strstr(deviceName, "RX 6700") || strstr(deviceName, "RX 6600")) {
			return VKNVG_GPU_TIER_MIDRANGE;
		}
		return VKNVG_GPU_TIER_ENTRY;
	}

	if (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		return VKNVG_GPU_TIER_MIDRANGE;
	}

	return VKNVG_GPU_TIER_UNKNOWN;
}

static void vknvg__detectPlatformOptimizations(VKNVGcontext* vk)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(vk->physicalDevice, &deviceProperties);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(vk->physicalDevice, &memProperties);

	memset(&vk->platformOpt, 0, sizeof(VKNVGplatformOptimizations));
	vk->platformOpt.vendorID = deviceProperties.vendorID;
	vk->platformOpt.deviceID = deviceProperties.deviceID;
	strncpy(vk->platformOpt.deviceName, deviceProperties.deviceName, 255);
	vk->platformOpt.deviceName[255] = '\0';

	vk->platformOpt.tier = vknvg__detectGpuTier(deviceProperties.vendorID, deviceProperties.deviceID,
	                                             deviceProperties.deviceName, deviceProperties.deviceType);

	switch (vk->platformOpt.tier) {
		case VKNVG_GPU_TIER_ENTHUSIAST:
			vk->platformOpt.optimalVertexBufferSize = 8 * 1024 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 256 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 16 * 1024 * 1024;
			break;
		case VKNVG_GPU_TIER_HIGHEND:
			vk->platformOpt.optimalVertexBufferSize = 4 * 1024 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 128 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 8 * 1024 * 1024;
			break;
		case VKNVG_GPU_TIER_MIDRANGE:
			vk->platformOpt.optimalVertexBufferSize = 2 * 1024 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 64 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 4 * 1024 * 1024;
			break;
		case VKNVG_GPU_TIER_ENTRY:
			vk->platformOpt.optimalVertexBufferSize = 1024 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 64 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 2 * 1024 * 1024;
			break;
		case VKNVG_GPU_TIER_INTEGRATED:
			vk->platformOpt.optimalVertexBufferSize = 512 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 32 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 1024 * 1024;
			break;
		default:
			vk->platformOpt.optimalVertexBufferSize = 1024 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 64 * 1024;
			vk->platformOpt.optimalStagingBufferSize = 2 * 1024 * 1024;
			break;
	}

	switch (deviceProperties.vendorID) {
		case VKNVG_VENDOR_NVIDIA:
			vk->platformOpt.vendor = VKNVG_VENDOR_NVIDIA;
			vk->platformOpt.usePinnedMemory = VK_TRUE;
			vk->platformOpt.useDeviceLocalHostVisible = VK_FALSE;
			vk->platformOpt.useCoherentMemory = VK_TRUE;

			if (vk->platformOpt.tier >= VKNVG_GPU_TIER_HIGHEND) {
				vk->platformOpt.useAsyncCompute = VK_TRUE;
				vk->platformOpt.usePushDescriptors = VK_TRUE;
				vk->platformOpt.useLargePages = VK_TRUE;
				vk->platformOpt.useMemoryBudget = VK_TRUE;
			}
			break;

		case VKNVG_VENDOR_AMD:
			vk->platformOpt.vendor = VKNVG_VENDOR_AMD;
			vk->platformOpt.usePinnedMemory = VK_FALSE;
			vk->platformOpt.useDeviceLocalHostVisible = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
			vk->platformOpt.useCoherentMemory = VK_TRUE;

			if (vk->platformOpt.tier >= VKNVG_GPU_TIER_HIGHEND) {
				vk->platformOpt.useAsyncCompute = VK_TRUE;
				vk->platformOpt.usePushDescriptors = VK_TRUE;
			}
			break;

		case VKNVG_VENDOR_INTEL:
			vk->platformOpt.vendor = VKNVG_VENDOR_INTEL;
			vk->platformOpt.usePinnedMemory = VK_FALSE;
			vk->platformOpt.useDeviceLocalHostVisible = VK_TRUE;
			vk->platformOpt.useCoherentMemory = VK_TRUE;
			break;

		case VKNVG_VENDOR_ARM:
		case VKNVG_VENDOR_QUALCOMM:
		case VKNVG_VENDOR_IMGTECH:
			vk->platformOpt.vendor = (VKNVGvendor)deviceProperties.vendorID;
			vk->platformOpt.optimalVertexBufferSize = 256 * 1024;
			vk->platformOpt.optimalUniformBufferSize = 16 * 1024;
			vk->platformOpt.usePinnedMemory = VK_FALSE;
			vk->platformOpt.useDeviceLocalHostVisible = VK_TRUE;
			vk->platformOpt.useCoherentMemory = VK_TRUE;
			break;

		default:
			vk->platformOpt.vendor = VKNVG_VENDOR_UNKNOWN;
			vk->platformOpt.usePinnedMemory = VK_FALSE;
			vk->platformOpt.useDeviceLocalHostVisible = VK_FALSE;
			vk->platformOpt.useCoherentMemory = VK_TRUE;
			break;
	}

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		VkMemoryPropertyFlags flags = memProperties.memoryTypes[i].propertyFlags;

		if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
		    vk->platformOpt.preferredDeviceLocalMemoryType == 0) {
			vk->platformOpt.preferredDeviceLocalMemoryType = i;
		}

		if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
		    (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) &&
		    vk->platformOpt.preferredHostVisibleMemoryType == 0) {
			vk->platformOpt.preferredHostVisibleMemoryType = i;
		}

		if (vk->platformOpt.useDeviceLocalHostVisible) {
			if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
			    (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
			    (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				vk->platformOpt.preferredDeviceLocalMemoryType = i;
				vk->platformOpt.preferredHostVisibleMemoryType = i;
			}
		}
	}

	if (vk->platformOpt.useAsyncCompute) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vk->physicalDevice, &queueFamilyCount, NULL);

		VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vk->physicalDevice, &queueFamilyCount, queueFamilies);

		vk->platformOpt.computeQueueFamilyIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
			    !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			    i != vk->queueFamilyIndex) {
				vk->platformOpt.computeQueueFamilyIndex = i;
				break;
			}
		}

		if (vk->platformOpt.computeQueueFamilyIndex == UINT32_MAX) {
			for (uint32_t i = 0; i < queueFamilyCount; i++) {
				if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
				    queueFamilies[i].queueCount > 1) {
					vk->platformOpt.computeQueueFamilyIndex = i;
					break;
				}
			}
		}

		if (vk->platformOpt.computeQueueFamilyIndex == UINT32_MAX) {
			vk->platformOpt.useAsyncCompute = VK_FALSE;
		}

		free(queueFamilies);
	}
}

static VkResult vknvg__createRenderPass(VKNVGcontext* vk)
{
	VkAttachmentDescription colorAttachment = {0};
	colorAttachment.format = vk->colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthStencilAttachment = {0};
	depthStencilAttachment.format = vk->depthStencilFormat;
	depthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
	subpass.pDepthStencilAttachment = (vk->depthStencilFormat != VK_FORMAT_UNDEFINED) ? &depthStencilAttachmentRef : NULL;

	VkAttachmentDescription attachments[2];
	uint32_t attachmentCount = 1;
	attachments[0] = colorAttachment;
	if (vk->depthStencilFormat != VK_FORMAT_UNDEFINED) {
		attachments[1] = depthStencilAttachment;
		attachmentCount = 2;
	}

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachmentCount;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	return vkCreateRenderPass(vk->device, &renderPassInfo, NULL, &vk->renderPass);
}

#endif // NANOVG_VK_PLATFORM_H
