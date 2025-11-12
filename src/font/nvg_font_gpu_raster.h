#ifndef NVG_FONT_GPU_RASTER_H
#define NVG_FONT_GPU_RASTER_H

#include "nvg_font_types.h"
#include "nvg_font_gpu_types.h"
#include "nvg_font.h"  // For NVGRasterMode enum
#include <ft2build.h>
#include FT_FREETYPE_H

// Forward declaration
typedef struct NVGFontGpuRasterizer NVGFontGpuRasterizer;

// GPU rasterization parameters
typedef struct NVGGpuRasterParams {
	float pxRange;          // Anti-aliasing range in pixels (default: 1.5)
	int useWinding;         // 0=analytical coverage, 1=winding number (default: 1)
	int maxCurvesPerGlyph;  // Buffer sizing hint (default: 256)
	int maxContoursPerGlyph; // Buffer sizing hint (default: 32)
} NVGGpuRasterParams;

// Initialize GPU rasterizer
// vkContext should be NVGVkContext* (opaque to font system)
int nvgFont_InitGpuRasterizer(NVGFontSystem* fs, void* vkContext,
                               const NVGGpuRasterParams* params);

// Destroy GPU rasterizer
void nvgFont_DestroyGpuRasterizer(NVGFontSystem* fs);

// Check if glyph should be rasterized on GPU (based on size, complexity)
int nvgFont_ShouldUseGPU(NVGFontSystem* fs, FT_Face face,
                         unsigned int glyphIndex, int width, int height);

// Rasterize glyph directly to atlas using GPU
// Returns 1 on success, 0 on failure (caller should fall back to CPU)
int nvgFont_RasterizeGlyphGPU(NVGFontSystem* fs, FT_Face face,
                               unsigned int glyphIndex,
                               int width, int height,
                               int atlasX, int atlasY,
                               int atlasIndex);

// Extract FreeType outline to GPU format
// Returns 1 on success, 0 if outline is too complex for GPU
int nvgFont_ExtractOutline(FT_Face face, unsigned int glyphIndex,
                            NVGGpuGlyphData* glyphData,
                            int width, int height);

// Flush all queued GPU rasterization jobs
// Records compute commands into the provided command buffer
// Should be called BEFORE the render pass begins
// cmdBuffer: VkCommandBuffer to record commands into
// Returns number of jobs flushed
int nvgFont_FlushGpuRasterJobs(NVGFontSystem* fs, void* cmdBuffer);

// Set rasterization mode (internal use)
void nvgFont_SetGpuRasterizerMode(NVGFontSystem* fs, NVGRasterMode mode);

#endif // NVG_FONT_GPU_RASTER_H
