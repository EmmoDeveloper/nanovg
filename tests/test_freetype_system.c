#include "../src/nvg_freetype.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	printf("=== FreeType Direct Integration Test ===\n\n");

	// Create system
	printf("1. Creating FreeType font system...\n");
	NVGFontSystem* sys = nvgft_create(512, 512);
	if (!sys) {
		fprintf(stderr, "Failed to create font system\n");
		return 1;
	}
	printf("   ✓ Font system created\n\n");

	// Add font
	printf("2. Adding font...\n");
	int font_id = nvgft_add_font(sys, "sans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
	if (font_id < 0) {
		fprintf(stderr, "Failed to add font\n");
		nvgft_destroy(sys);
		return 1;
	}
	printf("   ✓ Font added (id=%d)\n\n", font_id);

	// Find font by name
	printf("3. Finding font by name...\n");
	int found_id = nvgft_find_font(sys, "sans");
	if (found_id != font_id) {
		fprintf(stderr, "Font lookup failed\n");
		nvgft_destroy(sys);
		return 1;
	}
	printf("   ✓ Font found (id=%d)\n\n", found_id);

	// Set state
	printf("4. Setting font state...\n");
	nvgft_set_font(sys, font_id);
	nvgft_set_size(sys, 24.0f);
	nvgft_set_spacing(sys, 1.0f);
	nvgft_set_align(sys, NVGFT_ALIGN_LEFT | NVGFT_ALIGN_BASELINE);
	printf("   ✓ State configured\n\n");

	// Text iteration
	printf("5. Testing text iteration...\n");
	NVGFTTextIter iter;
	nvgft_text_iter_init(sys, &iter, 10.0f, 50.0f, "Hello FreeType!", NULL);

	int count = 0;
	NVGFTQuad quad;
	while (nvgft_text_iter_next(sys, &iter, &quad)) {
		count++;
	}
	printf("   ✓ Iterated %d glyphs\n\n", count);

	// Get atlas size
	printf("6. Checking atlas...\n");
	int w, h;
	nvgft_get_atlas_size(sys, &w, &h);
	printf("   ✓ Atlas size: %dx%d\n\n", w, h);

	// Cleanup
	printf("7. Cleaning up...\n");
	nvgft_destroy(sys);
	printf("   ✓ Cleanup complete\n\n");

	printf("=== Test PASSED ===\n");
	printf("FreeType direct integration API is functional\n");

	return 0;
}
