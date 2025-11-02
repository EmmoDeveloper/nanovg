// Atlas Rectangle Packing - Guillotine Algorithm Implementation
// Implements efficient bin-packing for variable-sized glyphs

#include "nanovg_vk_atlas_packing.h"
#include <limits.h>

// Initialize atlas packer
void vknvg__initAtlasPacker(VKNVGatlasPacker* packer,
                            uint16_t width,
                            uint16_t height)
{
	if (!packer) return;

	memset(packer, 0, sizeof(VKNVGatlasPacker));
	packer->atlasWidth = width;
	packer->atlasHeight = height;
	packer->totalArea = (uint32_t)width * height;

	// Initial free rectangle is the entire atlas
	packer->freeRects[0].x = 0;
	packer->freeRects[0].y = 0;
	packer->freeRects[0].width = width;
	packer->freeRects[0].height = height;
	packer->freeRectCount = 1;
}

// Score a free rectangle for a given width/height
static int32_t vknvg__scoreRect(const VKNVGrect* freeRect,
                                uint16_t width,
                                uint16_t height,
                                VKNVGpackingHeuristic heuristic)
{
	int32_t leftoverHoriz = freeRect->width - width;
	int32_t leftoverVert = freeRect->height - height;

	// Doesn't fit
	if (leftoverHoriz < 0 || leftoverVert < 0) {
		return INT32_MAX;
	}

	switch (heuristic) {
		case VKNVG_PACK_BEST_SHORT_SIDE_FIT:
			return leftoverHoriz < leftoverVert ? leftoverHoriz : leftoverVert;

		case VKNVG_PACK_BEST_LONG_SIDE_FIT:
			return leftoverHoriz > leftoverVert ? leftoverHoriz : leftoverVert;

		case VKNVG_PACK_BEST_AREA_FIT:
			return leftoverHoriz * leftoverVert;

		case VKNVG_PACK_BOTTOM_LEFT:
			return freeRect->y * 10000 + freeRect->x;

		default:
			return INT32_MAX;
	}
}

// Find best free rectangle for allocation
static int32_t vknvg__findBestRect(VKNVGatlasPacker* packer,
                                   uint16_t width,
                                   uint16_t height,
                                   VKNVGpackingHeuristic heuristic)
{
	int32_t bestScore = INT32_MAX;
	int32_t bestIndex = -1;

	for (uint32_t i = 0; i < packer->freeRectCount; i++) {
		VKNVGrect* rect = &packer->freeRects[i];

		// Early termination: perfect fit
		if (rect->width == width && rect->height == height) {
			return (int32_t)i;
		}

		int32_t score = vknvg__scoreRect(rect, width, height, heuristic);
		if (score < bestScore) {
			bestScore = score;
			bestIndex = (int32_t)i;

			// Early termination: very good fit (waste < 5%)
			if (rect->width >= width && rect->height >= height) {
				uint32_t rectArea = rect->width * rect->height;
				uint32_t usedArea = width * height;
				uint32_t wastedArea = rectArea - usedArea;
				if (wastedArea * 20 < rectArea) {  // Less than 5% waste
					break;
				}
			}
		}
	}

	return bestIndex;
}

// Split free rectangle using guillotine cut
static void vknvg__splitFreeRect(VKNVGatlasPacker* packer,
                                 uint32_t freeRectIndex,
                                 const VKNVGrect* usedRect,
                                 VKNVGsplitRule splitRule)
{
	if (packer->freeRectCount >= VKNVG_MAX_FREE_RECTS - 1) {
		return;	// No room for more splits
	}

	VKNVGrect freeRect = packer->freeRects[freeRectIndex];

	// Calculate remaining space
	uint16_t leftoverHoriz = freeRect.width - usedRect->width;
	uint16_t leftoverVert = freeRect.height - usedRect->height;

	// Determine split axis
	int splitHorizontal = 0;
	switch (splitRule) {
		case VKNVG_SPLIT_SHORTER_AXIS:
			splitHorizontal = (leftoverHoriz < leftoverVert) ? 1 : 0;
			break;

		case VKNVG_SPLIT_LONGER_AXIS:
			splitHorizontal = (leftoverHoriz > leftoverVert) ? 1 : 0;
			break;

		case VKNVG_SPLIT_MINIMIZE_AREA:
		case VKNVG_SPLIT_MAXIMIZE_AREA:
			// For simplicity, use shorter axis
			splitHorizontal = (leftoverHoriz < leftoverVert) ? 1 : 0;
			break;
	}

	// Create split rectangles
	VKNVGrect bottom, right;

	if (splitHorizontal) {
		// Horizontal split: bottom rect spans full width
		bottom.x = freeRect.x;
		bottom.y = freeRect.y + usedRect->height;
		bottom.width = freeRect.width;
		bottom.height = leftoverVert;

		// Right rect is only to the right of used rect
		right.x = freeRect.x + usedRect->width;
		right.y = freeRect.y;
		right.width = leftoverHoriz;
		right.height = usedRect->height;
	} else {
		// Vertical split: right rect spans full height
		right.x = freeRect.x + usedRect->width;
		right.y = freeRect.y;
		right.width = leftoverHoriz;
		right.height = freeRect.height;

		// Bottom rect is only below used rect
		bottom.x = freeRect.x;
		bottom.y = freeRect.y + usedRect->height;
		bottom.width = usedRect->width;
		bottom.height = leftoverVert;
	}

	// Remove the used free rectangle
	if (freeRectIndex < packer->freeRectCount - 1) {
		packer->freeRects[freeRectIndex] = packer->freeRects[packer->freeRectCount - 1];
	}
	packer->freeRectCount--;

	// Add new free rectangles if they have non-zero area
	if (bottom.width > 0 && bottom.height > 0) {
		packer->freeRects[packer->freeRectCount++] = bottom;
	}
	if (right.width > 0 && right.height > 0 && packer->freeRectCount < VKNVG_MAX_FREE_RECTS) {
		packer->freeRects[packer->freeRectCount++] = right;
	}
}

// Allocate rectangle in atlas
int vknvg__packRect(VKNVGatlasPacker* packer,
                    uint16_t width,
                    uint16_t height,
                    VKNVGrect* outRect,
                    VKNVGpackingHeuristic heuristic,
                    VKNVGsplitRule splitRule)
{
	if (!packer || !outRect || width == 0 || height == 0) {
		return 0;
	}

	// Find best fit
	int32_t bestIndex = vknvg__findBestRect(packer, width, height, heuristic);
	if (bestIndex < 0) {
		packer->failedAllocations++;
		return 0;	// No space
	}

	// Allocate from this rectangle
	VKNVGrect* freeRect = &packer->freeRects[bestIndex];
	outRect->x = freeRect->x;
	outRect->y = freeRect->y;
	outRect->width = width;
	outRect->height = height;

	// Split the free rectangle
	vknvg__splitFreeRect(packer, (uint32_t)bestIndex, outRect, splitRule);

	// Update statistics
	packer->allocatedArea += (uint32_t)width * height;
	packer->allocationCount++;

	return 1;
}

// Get packing efficiency (0.0 to 1.0)
float vknvg__getPackingEfficiency(const VKNVGatlasPacker* packer)
{
	if (!packer || packer->totalArea == 0) return 0.0f;
	return (float)packer->allocatedArea / (float)packer->totalArea;
}

// Reset packer (clear all allocations)
void vknvg__resetAtlasPacker(VKNVGatlasPacker* packer)
{
	if (!packer) return;

	uint16_t width = packer->atlasWidth;
	uint16_t height = packer->atlasHeight;
	vknvg__initAtlasPacker(packer, width, height);
}
