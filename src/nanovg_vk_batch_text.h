// Batch Text Rendering Optimization
// Merges consecutive text rendering calls into single draw calls
//
// This optimization reduces draw call overhead by combining multiple nvgText() calls
// that share compatible rendering state (same font atlas, blend mode, pipeline).

#ifndef NANOVG_VK_BATCH_TEXT_H
#define NANOVG_VK_BATCH_TEXT_H

// Check if two calls can be batched together
static VkBool32 vknvg__canBatchCalls(VKNVGcall* call1, VKNVGcall* call2, VKNVGcontext* vk)
{
	// Only batch TRIANGLE type calls (type == 2)
	if (call1->type != 2 || call2->type != 2) {
		return VK_FALSE;
	}

	// Must have same image (font atlas)
	if (call1->image != call2->image) {
		return VK_FALSE;
	}

	// Must have same blend mode
	if (call1->blend.srcRGB != call2->blend.srcRGB ||
		call1->blend.dstRGB != call2->blend.dstRGB ||
		call1->blend.srcAlpha != call2->blend.srcAlpha ||
		call1->blend.dstAlpha != call2->blend.dstAlpha) {
		return VK_FALSE;
	}

	// Must both use instancing or both not use instancing
	if (call1->useInstancing != call2->useInstancing) {
		return VK_FALSE;
	}

	// If instanced, verify instances are consecutive in the buffer
	if (call1->useInstancing) {
		if (call1->instanceOffset + call1->instanceCount != call2->instanceOffset) {
			return VK_FALSE;
		}
	} else {
		// If not instanced, verify vertices are consecutive
		if (call1->triangleOffset + call1->triangleCount != call2->triangleOffset) {
			return VK_FALSE;
		}
	}

	return VK_TRUE;
}

// Batch consecutive compatible text rendering calls
// Returns the new number of calls after batching
static int vknvg__batchTextCalls(VKNVGcontext* vk)
{
	if (vk->ncalls <= 1) {
		return vk->ncalls;
	}

	int writeIdx = 0;	// Where to write the next batched call
	int readIdx = 0;	// Current call being processed
	int batchedCount = 0;

	while (readIdx < vk->ncalls) {
		VKNVGcall* currentCall = &vk->calls[readIdx];

		// Copy current call to write position
		if (writeIdx != readIdx) {
			vk->calls[writeIdx] = *currentCall;
		}

		// Try to batch with subsequent calls
		int nextIdx = readIdx + 1;
		while (nextIdx < vk->ncalls && vknvg__canBatchCalls(&vk->calls[writeIdx], &vk->calls[nextIdx], vk)) {
			VKNVGcall* nextCall = &vk->calls[nextIdx];

			// Merge nextCall into the call at writeIdx
			if (vk->calls[writeIdx].useInstancing) {
				// Extend instance count
				vk->calls[writeIdx].instanceCount += nextCall->instanceCount;
			} else {
				// Extend triangle count
				vk->calls[writeIdx].triangleCount += nextCall->triangleCount;
			}

			batchedCount++;
			nextIdx++;
		}

		// Move to next write position and next unprocessed call
		writeIdx++;
		readIdx = nextIdx;
	}

	return writeIdx;  // New call count after batching
}

#endif // NANOVG_VK_BATCH_TEXT_H
