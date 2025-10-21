// Test for instanced pipeline creation

#include "test_framework.h"

#define NANOVG_VK_IMPLEMENTATION
#include "test_utils.h"
#include <stdio.h>

// Test: Pipeline creation with different flags
TEST(pipeline_creation_basic)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	printf("  Basic context (NVG_ANTIALIAS only):\n");
	printf("    useTextInstancing: %s\n", vknvg->useTextInstancing ? "TRUE" : "FALSE");
	printf("    textInstancedVertShaderModule: %s\n",
	       vknvg->textInstancedVertShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textInstancedPipeline: %s\n",
	       vknvg->textInstancedPipeline != VK_NULL_HANDLE ? "created" : "NULL");
	printf("    fragShaderModule: %s\n",
	       vknvg->fragShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pipeline creation with SDF text
TEST(pipeline_creation_sdf)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_SDF_TEXT);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	printf("  SDF Text context (NVG_ANTIALIAS | NVG_SDF_TEXT):\n");
	printf("    useTextInstancing: %s\n", vknvg->useTextInstancing ? "TRUE" : "FALSE");
	printf("    textInstancedVertShaderModule: %s\n",
	       vknvg->textInstancedVertShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textSDFFragShaderModule: %s\n",
	       vknvg->textSDFFragShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textSDFPipeline: %s\n",
	       vknvg->textSDFPipeline != VK_NULL_HANDLE ? "created" : "NULL");
	printf("    textInstancedSDFPipeline: %s\n",
	       vknvg->textInstancedSDFPipeline != VK_NULL_HANDLE ? "created" : "NULL");

	// The basic text pipelines should be created
	ASSERT_TRUE(vknvg->textSDFPipeline != VK_NULL_HANDLE);

	// If instancing is enabled, the instanced pipeline should also be created
	if (vknvg->useTextInstancing) {
		printf("    ✓ Instancing is enabled\n");
		if (vknvg->textInstancedSDFPipeline == VK_NULL_HANDLE) {
			printf("    ✗ ERROR: Instanced SDF pipeline is NULL despite instancing being enabled!\n");
			ASSERT_TRUE(0); // Force failure
		} else {
			printf("    ✓ Instanced SDF pipeline created successfully\n");
		}
	} else {
		printf("    ! Instancing was disabled (pipeline creation likely failed)\n");
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pipeline creation with MSDF text
TEST(pipeline_creation_msdf)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_MSDF_TEXT);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	printf("  MSDF Text context (NVG_ANTIALIAS | NVG_MSDF_TEXT):\n");
	printf("    useTextInstancing: %s\n", vknvg->useTextInstancing ? "TRUE" : "FALSE");
	printf("    textMSDFFragShaderModule: %s\n",
	       vknvg->textMSDFFragShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textMSDFPipeline: %s\n",
	       vknvg->textMSDFPipeline != VK_NULL_HANDLE ? "created" : "NULL");
	printf("    textInstancedMSDFPipeline: %s\n",
	       vknvg->textInstancedMSDFPipeline != VK_NULL_HANDLE ? "created" : "NULL");

	ASSERT_TRUE(vknvg->textMSDFPipeline != VK_NULL_HANDLE);

	if (vknvg->useTextInstancing) {
		printf("    ✓ Instancing is enabled\n");
		if (vknvg->textInstancedMSDFPipeline == VK_NULL_HANDLE) {
			printf("    ✗ ERROR: Instanced MSDF pipeline is NULL despite instancing being enabled!\n");
			ASSERT_TRUE(0);
		} else {
			printf("    ✓ Instanced MSDF pipeline created successfully\n");
		}
	} else {
		printf("    ! Instancing was disabled\n");
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pipeline creation with subpixel text
TEST(pipeline_creation_subpixel)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_SUBPIXEL_TEXT);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	printf("  Subpixel Text context (NVG_ANTIALIAS | NVG_SUBPIXEL_TEXT):\n");
	printf("    useTextInstancing: %s\n", vknvg->useTextInstancing ? "TRUE" : "FALSE");
	printf("    textSubpixelFragShaderModule: %s\n",
	       vknvg->textSubpixelFragShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textSubpixelPipeline: %s\n",
	       vknvg->textSubpixelPipeline != VK_NULL_HANDLE ? "created" : "NULL");
	printf("    textInstancedSubpixelPipeline: %s\n",
	       vknvg->textInstancedSubpixelPipeline != VK_NULL_HANDLE ? "created" : "NULL");

	ASSERT_TRUE(vknvg->textSubpixelPipeline != VK_NULL_HANDLE);

	if (vknvg->useTextInstancing) {
		printf("    ✓ Instancing is enabled\n");
		if (vknvg->textInstancedSubpixelPipeline == VK_NULL_HANDLE) {
			printf("    ✗ ERROR: Instanced subpixel pipeline is NULL despite instancing being enabled!\n");
			ASSERT_TRUE(0);
		} else {
			printf("    ✓ Instanced subpixel pipeline created successfully\n");
		}
	} else {
		printf("    ! Instancing was disabled\n");
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

// Test: Pipeline creation with color text
TEST(pipeline_creation_color)
{
	TestVulkanContext* vk = test_create_vulkan_context();
	ASSERT_NOT_NULL(vk);

	NVGcontext* nvg = test_create_nanovg_context(vk, NVG_ANTIALIAS | NVG_COLOR_TEXT);
	ASSERT_NOT_NULL(nvg);

	VKNVGcontext* vknvg = (VKNVGcontext*)nvgInternalParams(nvg)->userPtr;

	printf("  Color Text context (NVG_ANTIALIAS | NVG_COLOR_TEXT):\n");
	printf("    useTextInstancing: %s\n", vknvg->useTextInstancing ? "TRUE" : "FALSE");
	printf("    textColorFragShaderModule: %s\n",
	       vknvg->textColorFragShaderModule != VK_NULL_HANDLE ? "loaded" : "NULL");
	printf("    textColorPipeline: %s\n",
	       vknvg->textColorPipeline != VK_NULL_HANDLE ? "created" : "NULL");
	printf("    textInstancedColorPipeline: %s\n",
	       vknvg->textInstancedColorPipeline != VK_NULL_HANDLE ? "created" : "NULL");

	ASSERT_TRUE(vknvg->textColorPipeline != VK_NULL_HANDLE);

	if (vknvg->useTextInstancing) {
		printf("    ✓ Instancing is enabled\n");
		if (vknvg->textInstancedColorPipeline == VK_NULL_HANDLE) {
			printf("    ✗ ERROR: Instanced color pipeline is NULL despite instancing being enabled!\n");
			ASSERT_TRUE(0);
		} else {
			printf("    ✓ Instanced color pipeline created successfully\n");
		}
	} else {
		printf("    ! Instancing was disabled\n");
	}

	nvgDeleteVk(nvg);
	test_destroy_vulkan_context(vk);
}

int main(void)
{
	printf("==========================================\n");
	printf("  Pipeline Creation Diagnostic Tests\n");
	printf("==========================================\n\n");

	RUN_TEST(test_pipeline_creation_basic);
	printf("\n");
	RUN_TEST(test_pipeline_creation_sdf);
	printf("\n");
	RUN_TEST(test_pipeline_creation_msdf);
	printf("\n");
	RUN_TEST(test_pipeline_creation_subpixel);
	printf("\n");
	RUN_TEST(test_pipeline_creation_color);
	printf("\n");

	print_test_summary();
	return test_exit_code();
}
