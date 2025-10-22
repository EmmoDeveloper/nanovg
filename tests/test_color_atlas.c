// test_color_atlas.c - RGBA Color Atlas Tests
//
// Tests for Phase 6.2: RGBA Atlas System

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_color_atlas.h"

#define TEST_ASSERT(cond, msg) do { \
	if (!(cond)) { \
		printf("  ✗ FAIL at line %d: %s\n", __LINE__, msg); \
		return 0; \
	} \
} while(0)

#define TEST_PASS(name) do { \
	printf("  ✓ PASS %s\n", name); \
	return 1; \
} while(0)

// Test 1: Hash function consistency
static int test_hash_function() {
	printf("Test: Hash function consistency\n");

	uint32_t hash1 = vknvg__hashColorGlyph(1, 100, 48);
	uint32_t hash2 = vknvg__hashColorGlyph(1, 100, 48);
	TEST_ASSERT(hash1 == hash2, "same input produces same hash");

	uint32_t hash3 = vknvg__hashColorGlyph(1, 101, 48);
	TEST_ASSERT(hash1 != hash3, "different codepoint produces different hash");

	uint32_t hash4 = vknvg__hashColorGlyph(2, 100, 48);
	TEST_ASSERT(hash1 != hash4, "different font produces different hash");

	uint32_t hash5 = vknvg__hashColorGlyph(1, 100, 64);
	TEST_ASSERT(hash1 != hash5, "different size produces different hash");

	TEST_PASS("test_hash_function");
}

// Test 2: Cache entry structure
static int test_cache_entry() {
	printf("Test: Cache entry structure\n");

	VKNVGcolorGlyphCacheEntry entry = {0};
	entry.fontID = 1;
	entry.codepoint = 0x1F600; // 😀
	entry.size = 48;
	entry.hash = vknvg__hashColorGlyph(1, 0x1F600, 48);
	entry.atlasIndex = 0;
	entry.atlasX = 100;
	entry.atlasY = 200;
	entry.width = 64;
	entry.height = 64;
	entry.state = VKNVG_COLOR_GLYPH_UPLOADED;

	TEST_ASSERT(entry.fontID == 1, "fontID");
	TEST_ASSERT(entry.codepoint == 0x1F600, "codepoint");
	TEST_ASSERT(entry.size == 48, "size");
	TEST_ASSERT(entry.width == 64, "width");
	TEST_ASSERT(entry.height == 64, "height");
	TEST_ASSERT(entry.state == VKNVG_COLOR_GLYPH_UPLOADED, "state");

	TEST_PASS("test_cache_entry");
}

// Test 3: Upload request structure
static int test_upload_request() {
	printf("Test: Upload request structure\n");

	VKNVGcolorUploadRequest req = {0};
	req.atlasIndex = 0;
	req.atlasX = 10;
	req.atlasY = 20;
	req.width = 32;
	req.height = 32;

	// Allocate RGBA data
	uint32_t dataSize = req.width * req.height * 4;
	req.rgbaData = (uint8_t*)malloc(dataSize);
	TEST_ASSERT(req.rgbaData != NULL, "rgbaData allocation");

	// Fill with test pattern
	for (uint32_t i = 0; i < dataSize; i += 4) {
		req.rgbaData[i + 0] = 255; // R
		req.rgbaData[i + 1] = 0;   // G
		req.rgbaData[i + 2] = 0;   // B
		req.rgbaData[i + 3] = 255; // A
	}

	TEST_ASSERT(req.rgbaData[0] == 255, "first pixel red");
	TEST_ASSERT(req.rgbaData[3] == 255, "first pixel alpha");

	free(req.rgbaData);
	TEST_PASS("test_upload_request");
}

// Test 4: Mock atlas creation (no Vulkan)
static int test_mock_atlas() {
	printf("Test: Mock atlas structure\n");

	// Create mock structure (no actual Vulkan calls)
	VKNVGcolorAtlas atlas = {0};
	atlas.glyphCacheSize = 64;
	atlas.glyphCache = (VKNVGcolorGlyphCacheEntry*)calloc(64, sizeof(VKNVGcolorGlyphCacheEntry));
	TEST_ASSERT(atlas.glyphCache != NULL, "cache allocation");

	atlas.currentFrame = 0;
	atlas.cacheHits = 0;
	atlas.cacheMisses = 0;
	atlas.evictions = 0;

	TEST_ASSERT(atlas.glyphCacheSize == 64, "cache size");
	TEST_ASSERT(atlas.currentFrame == 0, "initial frame");

	free(atlas.glyphCache);
	TEST_PASS("test_mock_atlas");
}

// Test 5: Cache statistics
static int test_statistics() {
	printf("Test: Cache statistics\n");

	VKNVGcolorAtlas atlas = {0};
	atlas.cacheHits = 100;
	atlas.cacheMisses = 20;
	atlas.evictions = 5;
	atlas.glyphCount = 50;

	uint32_t hits, misses, evictions, count;
	vknvg__getColorAtlasStats(&atlas, &hits, &misses, &evictions, &count);

	TEST_ASSERT(hits == 100, "cache hits");
	TEST_ASSERT(misses == 20, "cache misses");
	TEST_ASSERT(evictions == 5, "evictions");
	TEST_ASSERT(count == 50, "glyph count");

	// Reset
	vknvg__resetColorAtlasStats(&atlas);
	TEST_ASSERT(atlas.cacheHits == 0, "hits reset");
	TEST_ASSERT(atlas.cacheMisses == 0, "misses reset");
	TEST_ASSERT(atlas.evictions == 0, "evictions reset");

	TEST_PASS("test_statistics");
}

// Test 6: LRU list operations (mock)
static int test_lru_list() {
	printf("Test: LRU list operations\n");

	VKNVGcolorAtlas atlas = {0};
	atlas.glyphCacheSize = 4;
	atlas.glyphCache = (VKNVGcolorGlyphCacheEntry*)calloc(4, sizeof(VKNVGcolorGlyphCacheEntry));
	atlas.currentFrame = 10;

	// Initialize entries
	for (int i = 0; i < 4; i++) {
		atlas.glyphCache[i].fontID = 1;
		atlas.glyphCache[i].codepoint = 100 + i;
		atlas.glyphCache[i].size = 48;
		atlas.glyphCache[i].state = VKNVG_COLOR_GLYPH_UPLOADED;
	}

	// Touch entries to build LRU list
	vknvg__touchColorGlyph(&atlas, &atlas.glyphCache[0]);
	vknvg__touchColorGlyph(&atlas, &atlas.glyphCache[1]);
	vknvg__touchColorGlyph(&atlas, &atlas.glyphCache[2]);
	vknvg__touchColorGlyph(&atlas, &atlas.glyphCache[3]);

	// Head should be most recently touched (entry 3)
	TEST_ASSERT(atlas.lruHead == &atlas.glyphCache[3], "head is entry 3");
	// Tail should be least recently touched (entry 0)
	TEST_ASSERT(atlas.lruTail == &atlas.glyphCache[0], "tail is entry 0");

	// Touch entry 0 again
	vknvg__touchColorGlyph(&atlas, &atlas.glyphCache[0]);
	// Now entry 0 should be at head
	TEST_ASSERT(atlas.lruHead == &atlas.glyphCache[0], "entry 0 moved to head");
	// Entry 1 should now be tail
	TEST_ASSERT(atlas.lruTail == &atlas.glyphCache[1], "entry 1 is now tail");

	free(atlas.glyphCache);
	TEST_PASS("test_lru_list");
}

// Test 7: Upload queue management
static int test_upload_queue() {
	printf("Test: Upload queue management\n");

	VKNVGcolorAtlas atlas = {0};
	atlas.uploadQueueHead = 0;
	atlas.uploadQueueTail = 0;
	atlas.uploadQueueCount = 0;

	// Simulate adding to queue
	atlas.uploadQueue[atlas.uploadQueueTail].atlasIndex = 0;
	atlas.uploadQueue[atlas.uploadQueueTail].width = 32;
	atlas.uploadQueue[atlas.uploadQueueTail].height = 32;
	atlas.uploadQueueTail = (atlas.uploadQueueTail + 1) % VKNVG_COLOR_UPLOAD_QUEUE_SIZE;
	atlas.uploadQueueCount++;

	TEST_ASSERT(atlas.uploadQueueCount == 1, "one item queued");

	// Add another
	atlas.uploadQueue[atlas.uploadQueueTail].atlasIndex = 1;
	atlas.uploadQueue[atlas.uploadQueueTail].width = 64;
	atlas.uploadQueue[atlas.uploadQueueTail].height = 64;
	atlas.uploadQueueTail = (atlas.uploadQueueTail + 1) % VKNVG_COLOR_UPLOAD_QUEUE_SIZE;
	atlas.uploadQueueCount++;

	TEST_ASSERT(atlas.uploadQueueCount == 2, "two items queued");

	// Dequeue one
	VKNVGcolorUploadRequest* req = &atlas.uploadQueue[atlas.uploadQueueHead];
	TEST_ASSERT(req->width == 32, "first item width");
	atlas.uploadQueueHead = (atlas.uploadQueueHead + 1) % VKNVG_COLOR_UPLOAD_QUEUE_SIZE;
	atlas.uploadQueueCount--;

	TEST_ASSERT(atlas.uploadQueueCount == 1, "one item remains");

	// Dequeue second
	req = &atlas.uploadQueue[atlas.uploadQueueHead];
	TEST_ASSERT(req->width == 64, "second item width");

	TEST_PASS("test_upload_queue");
}

// Test 8: RGBA data format
static int test_rgba_format() {
	printf("Test: RGBA data format\n");

	// Create test RGBA data (8x8 red square)
	uint32_t width = 8;
	uint32_t height = 8;
	uint32_t dataSize = width * height * 4;
	uint8_t* rgbaData = (uint8_t*)malloc(dataSize);
	TEST_ASSERT(rgbaData != NULL, "rgba allocation");

	// Fill with red
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint32_t offset = (y * width + x) * 4;
			rgbaData[offset + 0] = 255; // R
			rgbaData[offset + 1] = 0;   // G
			rgbaData[offset + 2] = 0;   // B
			rgbaData[offset + 3] = 255; // A
		}
	}

	// Verify corners
	TEST_ASSERT(rgbaData[0] == 255, "top-left red");
	TEST_ASSERT(rgbaData[1] == 0, "top-left green");
	TEST_ASSERT(rgbaData[2] == 0, "top-left blue");
	TEST_ASSERT(rgbaData[3] == 255, "top-left alpha");

	uint32_t bottomRightOffset = (7 * width + 7) * 4;
	TEST_ASSERT(rgbaData[bottomRightOffset + 0] == 255, "bottom-right red");
	TEST_ASSERT(rgbaData[bottomRightOffset + 3] == 255, "bottom-right alpha");

	// Test premultiplied alpha
	uint8_t* semiTransparent = (uint8_t*)malloc(dataSize);
	for (uint32_t i = 0; i < width * height; i++) {
		uint8_t alpha = 128;
		semiTransparent[i * 4 + 0] = (255 * alpha) / 255; // R premultiplied
		semiTransparent[i * 4 + 1] = 0;
		semiTransparent[i * 4 + 2] = 0;
		semiTransparent[i * 4 + 3] = alpha;
	}

	TEST_ASSERT(semiTransparent[0] == 128, "premultiplied red");
	TEST_ASSERT(semiTransparent[3] == 128, "alpha 128");

	free(rgbaData);
	free(semiTransparent);
	TEST_PASS("test_rgba_format");
}

// Test 9: Glyph state machine
static int test_state_machine() {
	printf("Test: Glyph state machine\n");

	VKNVGcolorGlyphCacheEntry entry = {0};

	// Initial state
	entry.state = VKNVG_COLOR_GLYPH_EMPTY;
	TEST_ASSERT(entry.state == VKNVG_COLOR_GLYPH_EMPTY, "initial empty");

	// Allocate
	entry.state = VKNVG_COLOR_GLYPH_LOADING;
	TEST_ASSERT(entry.state == VKNVG_COLOR_GLYPH_LOADING, "loading state");

	// Ready for upload
	entry.state = VKNVG_COLOR_GLYPH_READY;
	TEST_ASSERT(entry.state == VKNVG_COLOR_GLYPH_READY, "ready state");

	// Uploaded
	entry.state = VKNVG_COLOR_GLYPH_UPLOADED;
	TEST_ASSERT(entry.state == VKNVG_COLOR_GLYPH_UPLOADED, "uploaded state");

	TEST_PASS("test_state_machine");
}

// Test 10: Frame management
static int test_frame_management() {
	printf("Test: Frame management\n");

	VKNVGcolorAtlas atlas = {0};
	atlas.currentFrame = 0;

	// Begin frame
	vknvg__colorAtlasBeginFrame(&atlas);
	TEST_ASSERT(atlas.currentFrame == 1, "frame incremented");

	vknvg__colorAtlasBeginFrame(&atlas);
	TEST_ASSERT(atlas.currentFrame == 2, "frame incremented again");

	// Simulate end frame (no-op without Vulkan)
	vknvg__colorAtlasEndFrame(&atlas);
	TEST_ASSERT(atlas.currentFrame == 2, "frame unchanged after end");

	TEST_PASS("test_frame_management");
}

int main() {
	printf("==========================================\n");
	printf("  RGBA Color Atlas Tests (Phase 6.2)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_hash_function()) passed++;
	total++; if (test_cache_entry()) passed++;
	total++; if (test_upload_request()) passed++;
	total++; if (test_mock_atlas()) passed++;
	total++; if (test_statistics()) passed++;
	total++; if (test_lru_list()) passed++;
	total++; if (test_upload_queue()) passed++;
	total++; if (test_rgba_format()) passed++;
	total++; if (test_state_machine()) passed++;
	total++; if (test_frame_management()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
