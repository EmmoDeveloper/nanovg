// Test for Text Run Caching
// Verifies text caching data structures and lookup logic

#include "test_framework.h"
#include "../src/nanovg_vk_text_cache.h"
#include <stdio.h>
#include <string.h>

// Test: Create and destroy cache
TEST(create_destroy_cache)
{
	// Note: This test is currently incomplete as we need Vulkan device/renderpass
	// For now, test the key hashing and comparison functions

	VKNVGtextRunKey key1, key2;

	vknvg__buildTextRunKey(&key1, "Hello World", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	vknvg__buildTextRunKey(&key2, "Hello World", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	// Same keys should hash to same value
	uint32_t hash1 = vknvg__hashTextRunKey(&key1);
	uint32_t hash2 = vknvg__hashTextRunKey(&key2);

	printf("    Hash1: %u, Hash2: %u\n", hash1, hash2);
	ASSERT_TRUE(hash1 == hash2);
	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key2) == VK_TRUE);

	// Different text should produce different hash
	VKNVGtextRunKey key3;
	vknvg__buildTextRunKey(&key3, "Different", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	uint32_t hash3 = vknvg__hashTextRunKey(&key3);

	printf("    Hash3: %u (should differ from hash1)\n", hash3);
	ASSERT_TRUE(hash3 != hash1);
	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key3) == VK_FALSE);
}

// Test: Key building with different parameters
TEST(key_building)
{
	VKNVGtextRunKey key1, key2, key3;

	// Same text, different font size
	vknvg__buildTextRunKey(&key1, "Test", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	vknvg__buildTextRunKey(&key2, "Test", 1, 18.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key2) == VK_FALSE);
	printf("    Different font sizes produce different keys: OK\n");

	// Same text, different font ID
	vknvg__buildTextRunKey(&key3, "Test", 2, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key3) == VK_FALSE);
	printf("    Different fonts produce different keys: OK\n");

	// Same text, different color
	VKNVGtextRunKey key4;
	vknvg__buildTextRunKey(&key4, "Test", 1, 16.0f, 0.0f, 0.0f, 0xFF0000FF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key4) == VK_FALSE);
	printf("    Different colors produce different keys: OK\n");
}

// Test: Long text truncation
TEST(long_text_truncation)
{
	char longText[256];
	memset(longText, 'A', 255);
	longText[255] = '\0';

	VKNVGtextRunKey key;
	vknvg__buildTextRunKey(&key, longText, 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	// Should be truncated to VKNVG_TEXT_CACHE_MAX_STRING - 1
	size_t keyLen = strlen(key.text);
	printf("    Long text truncated from 255 to %zu chars\n", keyLen);
	ASSERT_TRUE(keyLen == VKNVG_TEXT_CACHE_MAX_STRING - 1);

	// Should end with null terminator
	ASSERT_TRUE(key.text[VKNVG_TEXT_CACHE_MAX_STRING - 1] == '\0');
}

// Test: Hash distribution
TEST(hash_distribution)
{
	// Generate many keys and check hash distribution
	uint32_t buckets[16] = {0};
	int numKeys = 1000;

	for (int i = 0; i < numKeys; i++) {
		char text[32];
		snprintf(text, sizeof(text), "Text %d", i);

		VKNVGtextRunKey key;
		vknvg__buildTextRunKey(&key, text, 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

		uint32_t hash = vknvg__hashTextRunKey(&key);
		buckets[hash % 16]++;
	}

	// Check that distribution is reasonably uniform
	printf("    Hash distribution across 16 buckets:\n");
	int minBucket = numKeys;
	int maxBucket = 0;

	for (int i = 0; i < 16; i++) {
		printf("      Bucket %2d: %u\n", i, buckets[i]);
		if (buckets[i] < minBucket) minBucket = buckets[i];
		if (buckets[i] > maxBucket) maxBucket = buckets[i];
	}

	// Distribution should be reasonable (within 2x of average)
	int avg = numKeys / 16;
	printf("    Average: %d, Min: %d, Max: %d\n", avg, minBucket, maxBucket);

	// Allow some variance but not too much
	ASSERT_TRUE(maxBucket < avg * 2);
	ASSERT_TRUE(minBucket > avg / 2);
}

// Test: Key comparison edge cases
TEST(key_comparison_edge_cases)
{
	VKNVGtextRunKey key1, key2;

	// Empty strings
	vknvg__buildTextRunKey(&key1, "", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	vknvg__buildTextRunKey(&key2, "", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key2) == VK_TRUE);
	printf("    Empty strings compare equal: OK\n");

	// Single character
	vknvg__buildTextRunKey(&key1, "A", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	vknvg__buildTextRunKey(&key2, "B", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key2) == VK_FALSE);
	printf("    Single character difference detected: OK\n");

	// Special characters
	vknvg__buildTextRunKey(&key1, "Hello\n\t\r", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);
	vknvg__buildTextRunKey(&key2, "Hello\n\t\r", 1, 16.0f, 0.0f, 0.0f, 0xFFFFFFFF, 0);

	ASSERT_TRUE(vknvg__compareTextRunKeys(&key1, &key2) == VK_TRUE);
	printf("    Special characters handled: OK\n");
}

int main(void)
{
	printf("\n");
	printf("==========================================\n");
	printf("  Text Cache Tests\n");
	printf("==========================================\n");
	printf("\n");

	RUN_TEST(test_create_destroy_cache);
	RUN_TEST(test_key_building);
	RUN_TEST(test_long_text_truncation);
	RUN_TEST(test_hash_distribution);
	RUN_TEST(test_key_comparison_edge_cases);

	print_test_summary();
	return test_exit_code();
}
