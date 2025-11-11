#ifndef NVG_FONT_GPU_TYPES_H
#define NVG_FONT_GPU_TYPES_H

#include <stdint.h>

// GPU glyph rasterization data structures
// These match the GLSL shader layout (std430 alignment)

// Maximum limits for GPU buffers
#define NVG_GPU_MAX_CURVES 256
#define NVG_GPU_MAX_CONTOURS 32

// Curve types
#define NVG_GPU_CURVE_LINEAR 1
#define NVG_GPU_CURVE_QUADRATIC 2
#define NVG_GPU_CURVE_CUBIC 3

// GPU-side curve representation (48 bytes, 16-byte aligned)
typedef struct NVGGpuCurve {
	float p0[2];        // Start point
	float p1[2];        // Control point 1
	float p2[2];        // Control point 2 (or end for quadratic)
	float p3[2];        // End point
	uint32_t type;      // Curve type (linear, quadratic, cubic)
	uint32_t contourId; // Which contour this belongs to
	uint32_t padding[2]; // Explicit padding for 16-byte alignment
} NVGGpuCurve;

// Contour information (16 bytes)
typedef struct NVGGpuContour {
	uint32_t firstCurve;  // Index into curves array
	uint32_t curveCount;  // Number of curves in this contour
	int32_t winding;      // Winding direction (+1 CCW, -1 CW)
	uint32_t padding;     // Explicit padding
} NVGGpuContour;

// Complete glyph data for GPU rasterization (~13KB per glyph)
typedef struct NVGGpuGlyphData {
	// Header (32 bytes)
	uint32_t curveCount;
	uint32_t contourCount;
	float bbox[4];        // xMin, yMin, xMax, yMax (FreeType 26.6 fixed point units)
	uint32_t outputWidth;
	uint32_t outputHeight;
	float scale;          // Units per pixel
	uint32_t padding;

	// Variable-length arrays (curves then contours)
	NVGGpuCurve curves[NVG_GPU_MAX_CURVES];      // 12KB
	NVGGpuContour contours[NVG_GPU_MAX_CONTOURS]; // 512 bytes
} NVGGpuGlyphData;

// Push constants for compute shader (16 bytes)
typedef struct NVGGpuRasterPushConstants {
	uint32_t curveCount;
	uint32_t contourCount;
	float pxRange;        // Anti-aliasing range in pixels (e.g., 1.5)
	uint32_t useWinding;  // 0=analytical distance, 1=winding number
} NVGGpuRasterPushConstants;

// Atlas write parameters (16 bytes, matches shader layout)
typedef struct NVGGpuAtlasParams {
	int32_t offset[2];      // Where to write in atlas (x, y)
	int32_t glyphSize[2];   // Glyph dimensions (width, height)
} NVGGpuAtlasParams;

#endif // NVG_FONT_GPU_TYPES_H
