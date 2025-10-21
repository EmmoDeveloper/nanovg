// Integration test: Virtual atlas with NanoVG backend
// Tests that the virtual atlas can be enabled via the NVG_VIRTUAL_ATLAS flag

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include "../src/nanovg_vk_virtual_atlas.h"
#include <stdio.h>

// Test: Virtual atlas enabled via flag
TEST(nvg_virtual_atlas_flag)
{
	TestVulkanContext* vkctx = test_create_vulkan_context();
	ASSERT_NOT_NULL(vkctx);

	// Create NanoVG context WITH virtual atlas flag
	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = vkctx->instance;
	createInfo.physicalDevice = vkctx->physicalDevice;
	createInfo.device = vkctx->device;
	createInfo.queue = vkctx->queue;
	createInfo.queueFamilyIndex = vkctx->queueFamilyIndex;
	createInfo.commandPool = vkctx->commandPool;
	createInfo.descriptorPool = vkctx->descriptorPool;
	createInfo.renderPass = vkctx->renderPass;
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	createInfo.maxFrames = 3;

	NVGcontext* nvg = nvgCreateVk(&createInfo, NVG_ANTIALIAS | NVG_VIRTUAL_ATLAS);
	ASSERT_NOT_NULL(nvg);

	// Access the VKNVGcontext to check virtual atlas was created
	NVGparams* params = nvgInternalParams(nvg);
	VKNVGcontext* vk = (VKNVGcontext*)params->userPtr;

	ASSERT_NOT_NULL(vk);
	ASSERT_TRUE(vk->useVirtualAtlas == VK_TRUE);
	ASSERT_NOT_NULL(vk->virtualAtlas);

	printf("  ✓ Virtual atlas enabled via NVG_VIRTUAL_ATLAS flag\n");
	printf("    Atlas: %p\n", (void*)vk->virtualAtlas);
	printf("    useVirtualAtlas: %d\n", vk->useVirtualAtlas);

	// Get atlas statistics
	uint32_t hits, misses, evictions, uploads;
	vknvg__getAtlasStats(vk->virtualAtlas, &hits, &misses, &evictions, &uploads);
	printf("    Stats: hits=%d misses=%d evictions=%d uploads=%d\n",
	       hits, misses, evictions, uploads);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vkctx);
}

// Test: Virtual atlas NOT enabled without flag
TEST(nvg_virtual_atlas_disabled)
{
	TestVulkanContext* vkctx = test_create_vulkan_context();
	ASSERT_NOT_NULL(vkctx);

	// Create NanoVG context WITHOUT virtual atlas flag
	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = vkctx->instance;
	createInfo.physicalDevice = vkctx->physicalDevice;
	createInfo.device = vkctx->device;
	createInfo.queue = vkctx->queue;
	createInfo.queueFamilyIndex = vkctx->queueFamilyIndex;
	createInfo.commandPool = vkctx->commandPool;
	createInfo.descriptorPool = vkctx->descriptorPool;
	createInfo.renderPass = vkctx->renderPass;
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	createInfo.maxFrames = 3;

	NVGcontext* nvg = nvgCreateVk(&createInfo, NVG_ANTIALIAS);  // No NVG_VIRTUAL_ATLAS
	ASSERT_NOT_NULL(nvg);

	NVGparams* params = nvgInternalParams(nvg);
	VKNVGcontext* vk = (VKNVGcontext*)params->userPtr;

	ASSERT_NOT_NULL(vk);
	ASSERT_TRUE(vk->useVirtualAtlas == VK_FALSE);
	ASSERT_TRUE(vk->virtualAtlas == NULL);

	printf("  ✓ Virtual atlas disabled by default\n");
	printf("    useVirtualAtlas: %d\n", vk->useVirtualAtlas);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vkctx);
}

// Test: Virtual atlas with CJK flag combination
TEST(nvg_virtual_atlas_with_text_flags)
{
	TestVulkanContext* vkctx = test_create_vulkan_context();
	ASSERT_NOT_NULL(vkctx);

	// Create NanoVG context with virtual atlas + SDF text
	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = vkctx->instance;
	createInfo.physicalDevice = vkctx->physicalDevice;
	createInfo.device = vkctx->device;
	createInfo.queue = vkctx->queue;
	createInfo.queueFamilyIndex = vkctx->queueFamilyIndex;
	createInfo.commandPool = vkctx->commandPool;
	createInfo.descriptorPool = vkctx->descriptorPool;
	createInfo.renderPass = vkctx->renderPass;
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	createInfo.maxFrames = 3;

	// Typical flags for CJK rendering
	int flags = NVG_ANTIALIAS | NVG_VIRTUAL_ATLAS | NVG_SDF_TEXT;
	NVGcontext* nvg = nvgCreateVk(&createInfo, flags);
	ASSERT_NOT_NULL(nvg);

	NVGparams* params = nvgInternalParams(nvg);
	VKNVGcontext* vk = (VKNVGcontext*)params->userPtr;

	ASSERT_NOT_NULL(vk);
	ASSERT_TRUE(vk->useVirtualAtlas == VK_TRUE);
	ASSERT_NOT_NULL(vk->virtualAtlas);
	ASSERT_TRUE(vk->flags & NVG_SDF_TEXT);

	printf("  ✓ Virtual atlas works with text rendering flags\n");
	printf("    Flags: NVG_VIRTUAL_ATLAS | NVG_SDF_TEXT\n");
	printf("    useVirtualAtlas: %d\n", vk->useVirtualAtlas);
	printf("    SDF text enabled: %d\n", !!(vk->flags & NVG_SDF_TEXT));

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vkctx);
}

int main(void)
{
	printf("==========================================\n");
	printf("  NanoVG Virtual Atlas Integration Tests\n");
	printf("==========================================\n\n");

	RUN_TEST(test_nvg_virtual_atlas_flag);
	printf("\n");
	RUN_TEST(test_nvg_virtual_atlas_disabled);
	printf("\n");
	RUN_TEST(test_nvg_virtual_atlas_with_text_flags);
	printf("\n");

	print_test_summary();
	return test_exit_code();
}
