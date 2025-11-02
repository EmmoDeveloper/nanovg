// Atlas Rectangle Packing - Guillotine Algorithm
// Implements efficient bin-packing for variable-sized glyphs
//
// The Guillotine algorithm maintains a list of free rectangles and splits
// them using "guillotine cuts" when allocating space. This provides better
// space utilization than fixed-size pages.

#ifndef NANOVG_VK_ATLAS_PACKING_H
#define NANOVG_VK_ATLAS_PACKING_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VKNVG_MAX_FREE_RECTS 1024	// Maximum number of free rectangles

// Rectangle structure
typedef struct VKNVGrect {
	uint16_t x, y;		// Position
	uint16_t width, height;	// Dimensions
} VKNVGrect;

// Atlas packer state
typedef struct VKNVGatlasPacker {
	uint16_t atlasWidth;
	uint16_t atlasHeight;

	// Free rectangles list
	VKNVGrect freeRects[VKNVG_MAX_FREE_RECTS];
	uint32_t freeRectCount;

	// Statistics
	uint32_t allocatedArea;
	uint32_t totalArea;
	uint32_t allocationCount;
	uint32_t failedAllocations;
} VKNVGatlasPacker;

// Heuristics for choosing where to place a rectangle
typedef enum VKNVGpackingHeuristic {
	VKNVG_PACK_BEST_SHORT_SIDE_FIT,	// Minimize leftover horizontal space
	VKNVG_PACK_BEST_LONG_SIDE_FIT,	// Minimize leftover vertical space
	VKNVG_PACK_BEST_AREA_FIT,		// Minimize leftover area
	VKNVG_PACK_BOTTOM_LEFT,			// Place in bottom-left corner
} VKNVGpackingHeuristic;

// Split rule for guillotine cuts
typedef enum VKNVGsplitRule {
	VKNVG_SPLIT_SHORTER_AXIS,		// Split along shorter axis
	VKNVG_SPLIT_LONGER_AXIS,		// Split along longer axis
	VKNVG_SPLIT_MINIMIZE_AREA,		// Minimize wasted area
	VKNVG_SPLIT_MAXIMIZE_AREA,		// Maximize larger remaining rect
} VKNVGsplitRule;

// Function declarations
void vknvg__initAtlasPacker(VKNVGatlasPacker* packer, uint16_t width, uint16_t height);
int vknvg__packRect(VKNVGatlasPacker* packer, uint16_t width, uint16_t height,
                    VKNVGrect* outRect, VKNVGpackingHeuristic heuristic, VKNVGsplitRule splitRule);
float vknvg__getPackingEfficiency(const VKNVGatlasPacker* packer);
void vknvg__resetAtlasPacker(VKNVGatlasPacker* packer);

#endif // NANOVG_VK_ATLAS_PACKING_H
