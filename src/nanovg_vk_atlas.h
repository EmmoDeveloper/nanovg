// nanovg_vk_atlas.h - Font Atlas Management and Optimization
// Pre-warming atlas with common glyphs to eliminate first-frame stutters

#ifndef NANOVG_VK_ATLAS_H
#define NANOVG_VK_ATLAS_H

// Common glyphs for pre-warming the font atlas
// Covers ASCII printable range and common punctuation
static const char* NANOVG_VK_PREWARM_GLYPHS =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789"
	" .,!?;:'\"-()[]{}/@#$%&*+=<>|\\~`_";

// Common font sizes to pre-warm (in pixels)
static const float NANOVG_VK_PREWARM_SIZES[] = {
	12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 24.0f
};

#define NANOVG_VK_PREWARM_SIZE_COUNT 6

// Pre-warm font atlas with common glyphs at common sizes
// This eliminates first-frame stutters from glyph rasterization and upload
//
// Usage:
//   NVGcontext* vg = nvgCreateVk(...);
//   int font = nvgCreateFont(vg, "sans", "Roboto-Regular.ttf");
//   nvgPrewarmFont(vg, font);  // Pre-load common glyphs
//
// Parameters:
//   ctx    - NanoVG context
//   font   - Font handle returned from nvgCreateFont
//
// Returns: Number of glyphs pre-warmed
static int nvgPrewarmFont(NVGcontext* ctx, int font)
{
	if (ctx == NULL || font < 0) {
		return 0;
	}

	int glyphCount = 0;

	// Save current state
	nvgSave(ctx);

	// Set the font
	nvgFontFaceId(ctx, font);

	// Pre-warm at each common size
	for (int i = 0; i < NANOVG_VK_PREWARM_SIZE_COUNT; i++) {
		float size = NANOVG_VK_PREWARM_SIZES[i];
		nvgFontSize(ctx, size);

		// Render glyphs off-screen to force atlas upload
		// Position far off-screen so they don't appear
		nvgText(ctx, -10000.0f, -10000.0f, NANOVG_VK_PREWARM_GLYPHS, NULL);

		// Count glyphs (rough estimate)
		glyphCount += (int)strlen(NANOVG_VK_PREWARM_GLYPHS);
	}

	// Restore state
	nvgRestore(ctx);

	return glyphCount;
}

// Pre-warm all loaded fonts
// Convenience function to pre-warm all fonts in the context
//
// Parameters:
//   ctx       - NanoVG context
//   fontIDs   - Array of font handles
//   fontCount - Number of fonts in array
//
// Returns: Total number of glyphs pre-warmed across all fonts
static int nvgPrewarmFonts(NVGcontext* ctx, const int* fontIDs, int fontCount)
{
	if (ctx == NULL || fontIDs == NULL || fontCount <= 0) {
		return 0;
	}

	int totalGlyphs = 0;
	for (int i = 0; i < fontCount; i++) {
		totalGlyphs += nvgPrewarmFont(ctx, fontIDs[i]);
	}

	return totalGlyphs;
}

// Advanced pre-warming with custom glyph set
//
// Parameters:
//   ctx    - NanoVG context
//   font   - Font handle
//   glyphs - Custom string of glyphs to pre-warm
//   sizes  - Array of font sizes to pre-warm
//   sizeCount - Number of sizes in array
//
// Returns: Number of glyphs pre-warmed
static int nvgPrewarmFontCustom(NVGcontext* ctx, int font, const char* glyphs,
                                 const float* sizes, int sizeCount)
{
	if (ctx == NULL || font < 0 || glyphs == NULL || sizes == NULL || sizeCount <= 0) {
		return 0;
	}

	int glyphCount = 0;

	nvgSave(ctx);
	nvgFontFaceId(ctx, font);

	for (int i = 0; i < sizeCount; i++) {
		nvgFontSize(ctx, sizes[i]);
		nvgText(ctx, -10000.0f, -10000.0f, glyphs, NULL);
		glyphCount += (int)strlen(glyphs);
	}

	nvgRestore(ctx);

	return glyphCount;
}

#endif // NANOVG_VK_ATLAS_H
