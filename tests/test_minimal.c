#include "../src/nvg_vk.h"
#include "../src/nanovg.h"
#include "window_utils.h"
#include <stdio.h>

int main(void)
{
	printf("Creating window context...\n");
	fflush(stdout);
	WindowVulkanContext* winCtx = window_create_context(800, 600, "Minimal Test");
	printf("Returned from window_create_context: %p\n", (void*)winCtx);
	fflush(stdout);

	if (!winCtx) {
		fprintf(stderr, "Failed\n");
		return 1;
	}

	printf("Success!\n");
	fflush(stdout);

	window_destroy_context(winCtx);
	printf("Cleanup complete\n");
	return 0;
}
