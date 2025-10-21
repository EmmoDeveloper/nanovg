#include <stdio.h>
#include <stdlib.h>

#include "nanovg.h"

#define NANOVG_VK_IMPLEMENTATION
#include "nanovg_vk.h"

int main(void) {
	printf("Test 1: Header compilation\n");
	printf("✓ Headers compiled successfully\n\n");

	printf("Test 2: Structure sizes\n");
	printf("sizeof(VKNVGcontext) = %zu bytes\n", sizeof(VKNVGcontext));
	printf("sizeof(VKNVGtexture) = %zu bytes\n", sizeof(VKNVGtexture));
	printf("sizeof(VKNVGfragUniforms) = %zu bytes\n", sizeof(VKNVGfragUniforms));
	printf("✓ All structures defined\n\n");

	printf("Test 3: Function availability\n");
	NVGVkCreateInfo info = {0};
	printf("nvgCreateVk function: %p\n", (void*)nvgCreateVk);
	printf("✓ API functions available\n\n");

	printf("===========================================\n");
	printf("✓ Basic compilation test passed\n");
	printf("===========================================\n");

	return 0;
}
