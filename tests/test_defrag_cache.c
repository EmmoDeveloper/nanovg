#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Simple test to verify defragmentation and cache update logic
// This tests the core algorithms without requiring Vulkan setup

typedef struct {
	uint16_t srcX, srcY;
	uint16_t dstX, dstY;
	uint16_t width, height;
	uint32_t glyphId;
} TestGlyphMove;

typedef struct {
	uint32_t fontID;
	uint32_t codepoint;
	uint16_t atlasX, atlasY;
	uint16_t width, height;
	int isValid;
} TestCacheEntry;

// Simulate glyph cache update after defragmentation
void updateCacheAfterDefrag(TestCacheEntry* cache, int cacheSize,
                             const TestGlyphMove* moves, int moveCount,
                             int* updatedCount) {
	*updatedCount = 0;

	for (int i = 0; i < cacheSize; i++) {
		TestCacheEntry* entry = &cache[i];

		if (!entry->isValid) {
			continue;
		}

		// Check if this entry was moved
		for (int m = 0; m < moveCount; m++) {
			const TestGlyphMove* move = &moves[m];

			// Match by source position and dimensions
			if (entry->atlasX == move->srcX && entry->atlasY == move->srcY &&
			    entry->width == move->width && entry->height == move->height) {
				// Update to new position
				entry->atlasX = move->dstX;
				entry->atlasY = move->dstY;
				(*updatedCount)++;
				break;
			}
		}
	}
}

int main() {
	printf("=== Defragmentation Cache Update Test ===\n\n");

	// Create test cache with some glyphs
	printf("1. Creating test cache with 10 glyphs...\n");
	const int CACHE_SIZE = 10;
	TestCacheEntry cache[CACHE_SIZE];

	for (int i = 0; i < CACHE_SIZE; i++) {
		cache[i].fontID = 1;
		cache[i].codepoint = 0x4E00 + i;
		cache[i].atlasX = i * 64;
		cache[i].atlasY = 0;
		cache[i].width = 64;
		cache[i].height = 64;
		cache[i].isValid = 1;

		printf("   Glyph %d: pos=(%u, %u) size=%ux%u\n",
		       i, cache[i].atlasX, cache[i].atlasY,
		       cache[i].width, cache[i].height);
	}

	// Simulate defragmentation moves
	printf("\n2. Simulating defragmentation moves...\n");
	const int MOVE_COUNT = 5;
	TestGlyphMove moves[MOVE_COUNT];

	// Move glyphs 0-4 to compact them
	for (int i = 0; i < MOVE_COUNT; i++) {
		moves[i].srcX = i * 64;
		moves[i].srcY = 0;
		moves[i].dstX = i * 32;  // Compact to 32px spacing
		moves[i].dstY = 0;
		moves[i].width = 64;
		moves[i].height = 64;
		moves[i].glyphId = i;

		printf("   Move %d: (%u, %u) -> (%u, %u) [%ux%u]\n",
		       i, moves[i].srcX, moves[i].srcY,
		       moves[i].dstX, moves[i].dstY,
		       moves[i].width, moves[i].height);
	}

	// Update cache after defragmentation
	printf("\n3. Updating cache after defragmentation...\n");
	int updatedCount = 0;
	updateCacheAfterDefrag(cache, CACHE_SIZE, moves, MOVE_COUNT, &updatedCount);
	printf("   Updated %d cache entries\n", updatedCount);

	// Verify results
	printf("\n4. Verifying updated cache...\n");
	int passed = 1;

	for (int i = 0; i < CACHE_SIZE; i++) {
		printf("   Glyph %d: pos=(%u, %u) size=%ux%u",
		       i, cache[i].atlasX, cache[i].atlasY,
		       cache[i].width, cache[i].height);

		if (i < MOVE_COUNT) {
			// These should have been moved
			uint16_t expectedX = i * 32;
			if (cache[i].atlasX != expectedX) {
				printf(" - ERROR: Expected X=%u\n", expectedX);
				passed = 0;
			} else {
				printf(" - OK (moved)\n");
			}
		} else {
			// These should be unchanged
			uint16_t expectedX = i * 64;
			if (cache[i].atlasX != expectedX) {
				printf(" - ERROR: Expected X=%u\n", expectedX);
				passed = 0;
			} else {
				printf(" - OK (unchanged)\n");
			}
		}
	}

	if (updatedCount != MOVE_COUNT) {
		printf("\nERROR: Expected %d updates, got %d\n", MOVE_COUNT, updatedCount);
		passed = 0;
	}

	if (passed) {
		printf("\n=== Defragmentation Cache Test PASSED ===\n");
		return 0;
	} else {
		printf("\n=== Defragmentation Cache Test FAILED ===\n");
		return 1;
	}
}
