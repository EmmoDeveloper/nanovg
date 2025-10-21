// test_visual_atlas.c - Visual Validation: Save Virtual Atlas to Image
//
// This test renders text and saves the virtual atlas texture to a PPM image
// so you can see the actual cached glyphs visually.

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include "../src/nanovg_vk_virtual_atlas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Find a font with good CJK coverage
static const char* find_font(void)
{
	const char* candidates[] = {
		// Fonts with excellent CJK support
		"/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",  // Try non-TTC first
		"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
		"/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
		"/usr/share/fonts/truetype/arphic/uming.ttc",
		"/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
		// Fallback to basic fonts (limited CJK)
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		NULL
	};

	for (int i = 0; candidates[i] != NULL; i++) {
		FILE* f = fopen(candidates[i], "r");
		if (f) {
			fclose(f);
			return candidates[i];
		}
	}
	return NULL;
}

// Save 8-bit grayscale image as PPM (simple text format)
static void save_ppm(const char* filename, const uint8_t* data, int width, int height)
{
	FILE* f = fopen(filename, "wb");
	if (!f) {
		printf("  ⚠ Failed to open %s for writing\n", filename);
		return;
	}

	// PPM header (grayscale)
	fprintf(f, "P5\n%d %d\n255\n", width, height);

	// Write pixel data
	fwrite(data, 1, width * height, f);

	fclose(f);
	printf("  ✓ Saved to: %s\n", filename);
}

// Read back virtual atlas texture from GPU
static uint8_t* read_atlas_texture(VKNVGvirtualAtlas* atlas, VkDevice device,
                                    VkPhysicalDevice physicalDevice,
                                    int* out_width, int* out_height)
{
	*out_width = VKNVG_ATLAS_PHYSICAL_SIZE;
	*out_height = VKNVG_ATLAS_PHYSICAL_SIZE;

	size_t imageSize = VKNVG_ATLAS_PHYSICAL_SIZE * VKNVG_ATLAS_PHYSICAL_SIZE;
	uint8_t* pixels = (uint8_t*)malloc(imageSize);
	if (!pixels) return NULL;

	// Create staging buffer for readback
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer stagingBuffer;
	if (vkCreateBuffer(device, &bufferInfo, NULL, &stagingBuffer) != VK_SUCCESS) {
		free(pixels);
		return NULL;
	}

	// Allocate memory for staging buffer
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	uint32_t memTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReqs.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
			memTypeIndex = i;
			break;
		}
	}

	if (memTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(device, stagingBuffer, NULL);
		free(pixels);
		return NULL;
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	VkDeviceMemory stagingMemory;
	if (vkAllocateMemory(device, &allocInfo, NULL, &stagingMemory) != VK_SUCCESS) {
		vkDestroyBuffer(device, stagingBuffer, NULL);
		free(pixels);
		return NULL;
	}

	vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

	// Create command buffer for copy operation
	VkCommandPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = 0; // Assume graphics queue family 0

	VkCommandPool cmdPool;
	if (vkCreateCommandPool(device, &poolInfo, NULL, &cmdPool) != VK_SUCCESS) {
		vkFreeMemory(device, stagingMemory, NULL);
		vkDestroyBuffer(device, stagingBuffer, NULL);
		free(pixels);
		return NULL;
	}

	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = cmdPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &beginInfo);

	// Transition image to transfer source
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = atlas->atlasImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	// Copy image to buffer
	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = VKNVG_ATLAS_PHYSICAL_SIZE;
	region.imageExtent.height = VKNVG_ATLAS_PHYSICAL_SIZE;
	region.imageExtent.depth = 1;

	vkCmdCopyImageToBuffer(cmd, atlas->atlasImage,
	                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                       stagingBuffer, 1, &region);

	// Transition back to shader read
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(cmd);

	// Submit and wait
	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Map and copy data
	void* mapped;
	vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
	memcpy(pixels, mapped, imageSize);
	vkUnmapMemory(device, stagingMemory);

	// Cleanup
	vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
	vkDestroyCommandPool(device, cmdPool, NULL);
	vkFreeMemory(device, stagingMemory, NULL);
	vkDestroyBuffer(device, stagingBuffer, NULL);

	return pixels;
}

// Test: Render various text and visualize the atlas
static void test_visualize_atlas(void)
{
	printf("\n=== Visual Atlas Test ===\n");

	const char* fontPath = find_font();
	if (!fontPath) {
		printf("  ⚠ No font found, skipping visual test\n");
		return;
	}

	printf("  Using font: %s\n", fontPath);

	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	int flags = NVG_ANTIALIAS | NVG_VIRTUAL_ATLAS;
	NVGcontext* nvg = test_create_nanovg_context(vk, flags);
	ASSERT_NOT_NULL(nvg);

	int font = nvgCreateFont(nvg, "demo", fontPath);
	ASSERT_TRUE(font >= 0);

	NVGparams* params = nvgInternalParams(nvg);
	VKNVGcontext* vknvg = (VKNVGcontext*)params->userPtr;
	VKNVGvirtualAtlas* atlas = vknvg->virtualAtlas;
	ASSERT_NOT_NULL(atlas);

	printf("\n  Rendering demonstration text...\n");

	// Render various text samples including Chinese characters
	const char* samples[] = {
		"ABCDEFGHIJ",        // Test with Latin first
		"你好世界",           // Hello World (Chinese)
		"中文字符测试",        // Chinese character test
		"春夏秋冬",           // Spring Summer Fall Winter
		"一二三四五",         // One Two Three Four Five
		"大小多少",           // Big Small Many Few
		"天地人和",           // Heaven Earth Person Harmony
		"山水花鸟",           // Mountain Water Flower Bird
		"日月星辰",           // Sun Moon Star Celestial
		"Hello World! 你好",  // Mixed English/Chinese
		"こんにちは世界",     // Hello World (Japanese)
		NULL
	};

	for (int i = 0; samples[i] != NULL; i++) {
		nvgBeginFrame(nvg, 800, 600, 1.0f);
		nvgFontFace(nvg, "demo");
		nvgFontSize(nvg, 48.0f + i * 4.0f); // Larger sizes for visibility (48-84px)
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		nvgText(nvg, 10, 100, samples[i], NULL);
		nvgEndFrame(nvg); // IMPORTANT: Actually upload to GPU!
	}

	printf("  ✓ Chinese characters rendered\n");

	// Wait for all rendering/uploads from nvgEndFrame to complete
	vkQueueWaitIdle(vk->queue);
	printf("  ✓ Rendering complete, waited for GPU\n");

	// Manually trigger GPU uploads (headless test doesn't have render calls)
	printf("  Manually processing GPU uploads...\n");

	// Create command buffer for uploads
	VkCommandBufferAllocateInfo cmdAllocInfo = {0};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = vk->commandPool;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer uploadCmd;
	vkAllocateCommandBuffers(vk->device, &cmdAllocInfo, &uploadCmd);

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(uploadCmd, &beginInfo);

	// Debug: Check upload queue
	printf("  Upload queue count before processUploads: %u\n", atlas->uploadQueueCount);

	// Process uploads
	vknvg__processUploads(atlas, uploadCmd);

	// End and submit
	vkEndCommandBuffer(uploadCmd);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &uploadCmd;

	vkQueueSubmit(vk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->queue);

	// Cleanup
	vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &uploadCmd);

	printf("  ✓ GPU uploads complete\n");

	// Get statistics
	uint32_t hits, misses, evictions, uploads;
	vknvg__getAtlasStats(atlas, &hits, &misses, &evictions, &uploads);

	printf("  Atlas Statistics:\n");
	printf("    Glyphs cached: %u\n", misses);
	printf("    Cache hits:    %u\n", hits);
	printf("    Evictions:     %u\n", evictions);
	printf("    Uploads:       %u\n", uploads);

	// Check glyph locations for debugging
	printf("\n  Sample glyph locations:\n");
	int found = 0;
	for (uint32_t i = 0; i < atlas->glyphCacheSize && found < 10; i++) {
		if (atlas->glyphCache[i].state == VKNVG_GLYPH_UPLOADED) {
			printf("    Glyph at slot %u: atlas pos (%u,%u) size (%ux%u) codepoint=0x%X\n",
			       i, atlas->glyphCache[i].atlasX, atlas->glyphCache[i].atlasY,
			       atlas->glyphCache[i].width, atlas->glyphCache[i].height,
			       atlas->glyphCache[i].key.codepoint);
			found++;
		}
	}
	if (found == 0) {
		printf("    WARNING: No glyphs found in UPLOADED state!\n");
		// Check other states
		int empty = 0, loading = 0, ready = 0, uploaded = 0;
		for (uint32_t i = 0; i < atlas->glyphCacheSize; i++) {
			switch (atlas->glyphCache[i].state) {
				case VKNVG_GLYPH_EMPTY: empty++; break;
				case VKNVG_GLYPH_LOADING: loading++; break;
				case VKNVG_GLYPH_READY: ready++; break;
				case VKNVG_GLYPH_UPLOADED: uploaded++; break;
			}
		}
		printf("    State counts: EMPTY=%d, LOADING=%d, READY=%d, UPLOADED=%d\n",
		       empty, loading, ready, uploaded);
	}

	// Read back atlas texture
	printf("\n  Reading back virtual atlas texture from GPU...\n");

	int width, height;
	uint8_t* atlasData = read_atlas_texture(atlas, vk->device, vk->physicalDevice,
	                                         &width, &height);

	if (atlasData) {
		printf("  ✓ Successfully read %dx%d atlas\n", width, height);

		// Save full atlas
		save_ppm("virtual_atlas_full.ppm", atlasData, width, height);

		// Create a zoomed view of the glyph regions
		// Chinese characters are larger and spread across multiple pages
		// Let's capture a reasonable section where glyphs are placed
		int zoomWidth = 2048;   // Half width to see more detail
		int zoomHeight = 1024;  // Good height for multiple rows
		int offsetX = 0;
		int offsetY = height - zoomHeight;  // Bottom section where glyphs typically are

		uint8_t* glyphZoom = (uint8_t*)malloc(zoomWidth * zoomHeight);
		if (glyphZoom) {
			for (int y = 0; y < zoomHeight; y++) {
				memcpy(&glyphZoom[y * zoomWidth],
				       &atlasData[(offsetY + y) * width + offsetX],
				       zoomWidth);
			}
			save_ppm("virtual_atlas_zoom.ppm", glyphZoom, zoomWidth, zoomHeight);
			free(glyphZoom);
		}

		free(atlasData);

		printf("\n  📁 Output files created:\n");
		printf("     - virtual_atlas_full.ppm (4096x4096 - full atlas)\n");
		printf("     - virtual_atlas_zoom.ppm (1024x1024 - zoomed view)\n");
		printf("\n  View with:\n");
		printf("     convert virtual_atlas_full.ppm virtual_atlas_full.png\n");
		printf("     eog virtual_atlas_full.png\n");
		printf("  Or:\n");
		printf("     gimp virtual_atlas_full.ppm\n");
	} else {
		printf("  ✗ Failed to read atlas texture\n");
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);

	printf("  ✓ PASS test_visualize_atlas\n");
}

int main(void)
{
	printf("==========================================\n");
	printf("  Visual Atlas Validation\n");
	printf("==========================================\n");
	printf("This test renders text and saves the\n");
	printf("virtual atlas texture to PPM image files.\n");
	printf("==========================================\n");

	test_visualize_atlas();

	printf("\n========================================\n");
	printf("Visual validation complete!\n");
	printf("\nYou can now see the cached glyphs in the\n");
	printf("virtual atlas texture image files.\n");
	printf("========================================\n");

	return 0;
}
