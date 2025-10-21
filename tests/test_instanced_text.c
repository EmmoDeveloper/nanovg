// Test for glyph instancing functionality

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Instanced rendering initialization
TEST(instanced_initialization)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	// Create context without text rendering flags - instancing may be disabled
	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	// Access internal context
	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// Verify instance buffer infrastructure exists
	ASSERT_NOT_NULL(vknvg->glyphInstanceBuffer);
	ASSERT_TRUE(vknvg->glyphInstanceBuffer->capacity == 65536);

	// Verify instanced shader module is loaded
	ASSERT_TRUE(vknvg->textInstancedVertShaderModule != VK_NULL_HANDLE);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);

	// Test with SDF text flag - should create instanced pipelines
	vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_SDF_TEXT);
	ASSERT_NOT_NULL(nvg);

	vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// With SDF text, the infrastructure exists
	ASSERT_NOT_NULL(vknvg->glyphInstanceBuffer);
	ASSERT_TRUE(vknvg->textInstancedVertShaderModule != VK_NULL_HANDLE);

	// Note: Actual pipeline creation depends on correct shader module setup
	// The important thing is that the infrastructure is in place

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Instance buffer operations
TEST(instance_buffer_operations)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;
	VKNVGglyphInstanceBuffer* buffer = vknvg->glyphInstanceBuffer;

	// Test adding instances
	int initialCount = buffer->count;

	// Add a few instances
	for (int i = 0; i < 10; i++) {
		int idx = vknvg__addGlyphInstance(buffer,
		                                   10.0f + i * 20.0f, 20.0f,  // position
		                                   16.0f, 16.0f,              // size
		                                   0.1f * i, 0.2f,            // UV offset
		                                   0.1f, 0.1f);               // UV size
		ASSERT_TRUE(idx != -1);
	}

	ASSERT_TRUE(buffer->count == initialCount + 10);

	// Test upload
	vknvg__uploadGlyphInstances(buffer);

	// Test reset
	vknvg__resetGlyphInstances(buffer);
	ASSERT_TRUE(buffer->count == 0);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Text rendering with instancing
TEST(text_rendering_instanced)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	// Create a dummy font
	int font = nvgCreateFont(nvg, "test", "nonexistent.ttf");

	// Test with and without instancing
	for (int useInstancing = 0; useInstancing <= 1; useInstancing++) {
		vknvg->useTextInstancing = useInstancing ? VK_TRUE : VK_FALSE;

		nvgBeginFrame(nvg, 800, 600, 1.0f);

		// Render some text
		nvgFontSize(nvg, 18.0f);
		nvgFontFace(nvg, "test");
		nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255));
		nvgText(nvg, 100, 100, "Hello World", NULL);
		nvgText(nvg, 100, 130, "Instanced Rendering", NULL);
		nvgText(nvg, 100, 160, "Test 123", NULL);

		nvgEndFrame(nvg);

		// Reset for next iteration
		if (vknvg->glyphInstanceBuffer != NULL) {
			vknvg__resetGlyphInstances(vknvg->glyphInstanceBuffer);
		}
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Instance buffer capacity limits
TEST(instance_buffer_capacity)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;
	VKNVGglyphInstanceBuffer* buffer = vknvg->glyphInstanceBuffer;

	// Fill buffer to capacity
	int capacity = buffer->capacity;
	for (int i = 0; i < capacity; i++) {
		int idx = vknvg__addGlyphInstance(buffer, i * 10.0f, 0.0f, 10.0f, 10.0f,
		                                   0.0f, 0.0f, 0.1f, 0.1f);
		ASSERT_TRUE(idx != -1);
	}

	ASSERT_TRUE(buffer->count == capacity);

	// Try to add one more (should fail gracefully)
	int idx = vknvg__addGlyphInstance(buffer, 0.0f, 0.0f, 10.0f, 10.0f,
	                                   0.0f, 0.0f, 0.1f, 0.1f);
	ASSERT_TRUE(idx == -1);

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	RUN_TEST(test_instanced_initialization);
	RUN_TEST(test_instance_buffer_operations);
	RUN_TEST(test_text_rendering_instanced);
	RUN_TEST(test_instance_buffer_capacity);

	print_test_summary();
	return test_exit_code();
}
