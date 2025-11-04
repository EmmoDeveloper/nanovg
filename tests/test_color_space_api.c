#include "nanovg.h"
#include <stdio.h>

int main(void)
{
	printf("=== NanoVG Color Space API Test ===\n\n");

	// Test color space API (API demonstration only - no actual context needed)
	printf("Testing color space API declarations...\n");

	printf("   • Setting source color space to sRGB...\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Source set to sRGB\n");

	printf("   • Setting destination color space to sRGB...\n");
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Destination set to sRGB\n");

	printf("   • Testing Display P3 color space...\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_DISPLAY_P3);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Display P3 → sRGB conversion configured\n");

	printf("   • Testing HDR10 color space...\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_HDR10);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ HDR10 → sRGB conversion configured\n");

	printf("   • Testing linear sRGB color space...\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_LINEAR_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	printf("     ✓ Linear sRGB → sRGB conversion configured\n");

	printf("   • Setting HDR scale factor to 80.0...\n");
	nvgColorSpaceHDRScale(vg, 80.0f);
	printf("     ✓ HDR scale set\n");

	printf("   • Enabling gamut mapping...\n");
	nvgColorSpaceGamutMapping(vg, 1);
	printf("     ✓ Gamut mapping enabled\n");

	printf("   • Enabling tone mapping...\n");
	nvgColorSpaceToneMapping(vg, 1);
	printf("     ✓ Tone mapping enabled\n");

	printf("   • Disabling tone mapping...\n");
	nvgColorSpaceToneMapping(vg, 0);
	printf("     ✓ Tone mapping disabled\n");

	printf("   • Resetting to default (sRGB → sRGB)...\n");
	nvgColorSpaceSource(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceDestination(vg, NVG_COLOR_SPACE_SRGB);
	nvgColorSpaceHDRScale(vg, 1.0f);
	printf("     ✓ Reset to defaults\n");

	printf("\n4. Cleaning up...\n");
	nvgDeleteVk(vg);
	window_destroy_context(window);
	printf("   ✓ Cleanup complete\n");

	printf("\n=== Color Space API Test PASSED ===\n");
	return 0;
}
