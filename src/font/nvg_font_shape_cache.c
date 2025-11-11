// Shaped text cache implementation
#include "nvg_font_shape_cache.h"
#include <stdlib.h>
#include <string.h>

// FNV-1a hash function for cache keys
uint32_t nvgShapeCache_hash(const NVGShapeKey* key) {
	uint32_t hash = 2166136261u;  // FNV-1a offset basis

	// Hash text content
	for (int i = 0; i < key->textLen; i++) {
		hash ^= (uint32_t)(unsigned char)key->text[i];
		hash *= 16777619u;  // FNV-1a prime
	}

	// Hash numeric fields
	hash ^= (uint32_t)key->fontId;
	hash *= 16777619u;

	// Hash size as integer (convert float to bits)
	uint32_t sizeHash = *(uint32_t*)&key->size;
	hash ^= sizeHash;
	hash *= 16777619u;

	hash ^= (uint32_t)key->hinting;
	hash *= 16777619u;

	hash ^= key->varStateId;
	hash *= 16777619u;

	// Hash features (already sorted by tag)
	for (int i = 0; i < key->nfeatures; i++) {
		// Hash 4-char tag
		for (int j = 0; j < 4; j++) {
			hash ^= (uint32_t)(unsigned char)key->featureTags[i][j];
			hash *= 16777619u;
		}
		hash ^= (uint32_t)key->featureValues[i];
		hash *= 16777619u;
	}

	hash ^= (uint32_t)key->kerningEnabled;
	hash *= 16777619u;

	hash ^= (uint32_t)key->bidiEnabled;
	hash *= 16777619u;

	hash ^= (uint32_t)key->baseDir;
	hash *= 16777619u;

	return hash;
}

// Compare two cache keys for equality
int nvgShapeCache_compareKeys(const NVGShapeKey* a, const NVGShapeKey* b) {
	if (a->hash != b->hash) return 0;
	if (a->fontId != b->fontId) return 0;
	if (a->size != b->size) return 0;  // Exact float comparison OK here
	if (a->hinting != b->hinting) return 0;
	if (a->varStateId != b->varStateId) return 0;
	if (a->kerningEnabled != b->kerningEnabled) return 0;
	if (a->bidiEnabled != b->bidiEnabled) return 0;
	if (a->baseDir != b->baseDir) return 0;
	if (a->textLen != b->textLen) return 0;
	if (a->nfeatures != b->nfeatures) return 0;

	// Compare text content
	if (memcmp(a->text, b->text, a->textLen) != 0) return 0;

	// Compare features
	for (int i = 0; i < a->nfeatures; i++) {
		if (strcmp(a->featureTags[i], b->featureTags[i]) != 0) return 0;
		if (a->featureValues[i] != b->featureValues[i]) return 0;
	}

	return 1;  // Keys match
}

// Sort features by tag for consistent hashing
void nvgShapeCache_sortFeatures(NVGShapeKey* key) {
	// Simple bubble sort (features array is small)
	for (int i = 0; i < key->nfeatures - 1; i++) {
		for (int j = 0; j < key->nfeatures - i - 1; j++) {
			if (strcmp(key->featureTags[j], key->featureTags[j + 1]) > 0) {
				// Swap tags
				char tempTag[5];
				memcpy(tempTag, key->featureTags[j], 5);
				memcpy(key->featureTags[j], key->featureTags[j + 1], 5);
				memcpy(key->featureTags[j + 1], tempTag, 5);

				// Swap values
				int tempVal = key->featureValues[j];
				key->featureValues[j] = key->featureValues[j + 1];
				key->featureValues[j + 1] = tempVal;
			}
		}
	}
}

// Create a new shaped text cache
NVGShapedTextCache* nvgShapeCache_create(void) {
	NVGShapedTextCache* cache = (NVGShapedTextCache*)calloc(1, sizeof(NVGShapedTextCache));
	return cache;
}

// Destroy shaped text cache and free all entries
void nvgShapeCache_destroy(NVGShapedTextCache* cache) {
	if (!cache) return;

	// Free all entries
	for (int i = 0; i < cache->count; i++) {
		if (cache->keys[i].text) {
			free(cache->keys[i].text);
			cache->keys[i].text = NULL;
		}
		if (cache->values[i].glyphInfo) {
			free(cache->values[i].glyphInfo);
			cache->values[i].glyphInfo = NULL;
		}
		if (cache->values[i].glyphPos) {
			free(cache->values[i].glyphPos);
			cache->values[i].glyphPos = NULL;
		}
	}

	free(cache);
}

// Look up a key in the cache
NVGShapedTextEntry* nvgShapeCache_lookup(NVGShapedTextCache* cache, const NVGShapeKey* key) {
	if (!cache || !key) return NULL;

	for (int i = 0; i < cache->count; i++) {
		if (cache->values[i].valid && nvgShapeCache_compareKeys(&cache->keys[i], key)) {
			// Cache hit - update LRU
			cache->values[i].lastUsed = cache->frameCounter++;
			return &cache->values[i];
		}
	}

	return NULL;  // Cache miss
}

// Insert shaped text result into cache
void nvgShapeCache_insert(NVGShapedTextCache* cache, NVGShapeKey* key, hb_buffer_t* hb_buffer) {
	if (!cache || !key || !hb_buffer) return;

	int insertIdx = -1;

	// Find slot: empty or LRU eviction
	if (cache->count < NVG_SHAPED_TEXT_CACHE_SIZE) {
		// Use next empty slot
		insertIdx = cache->count++;
	} else {
		// Cache is full - evict LRU entry
		unsigned int minUsed = cache->values[0].lastUsed;
		insertIdx = 0;
		for (int i = 1; i < NVG_SHAPED_TEXT_CACHE_SIZE; i++) {
			if (cache->values[i].lastUsed < minUsed) {
				minUsed = cache->values[i].lastUsed;
				insertIdx = i;
			}
		}

		// Free evicted entry
		if (cache->keys[insertIdx].text) {
			free(cache->keys[insertIdx].text);
			cache->keys[insertIdx].text = NULL;
		}
		if (cache->values[insertIdx].glyphInfo) {
			free(cache->values[insertIdx].glyphInfo);
			cache->values[insertIdx].glyphInfo = NULL;
		}
		if (cache->values[insertIdx].glyphPos) {
			free(cache->values[insertIdx].glyphPos);
			cache->values[insertIdx].glyphPos = NULL;
		}
	}

	// Copy key (transfer ownership of key->text)
	cache->keys[insertIdx] = *key;

	// Copy HarfBuzz output
	unsigned int glyphCount;
	hb_glyph_info_t* info = hb_buffer_get_glyph_infos(hb_buffer, &glyphCount);
	hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(hb_buffer, &glyphCount);

	cache->values[insertIdx].glyphCount = glyphCount;
	cache->values[insertIdx].glyphInfo = (hb_glyph_info_t*)malloc(
		sizeof(hb_glyph_info_t) * glyphCount);
	cache->values[insertIdx].glyphPos = (hb_glyph_position_t*)malloc(
		sizeof(hb_glyph_position_t) * glyphCount);

	if (cache->values[insertIdx].glyphInfo && cache->values[insertIdx].glyphPos) {
		memcpy(cache->values[insertIdx].glyphInfo, info,
		       sizeof(hb_glyph_info_t) * glyphCount);
		memcpy(cache->values[insertIdx].glyphPos, pos,
		       sizeof(hb_glyph_position_t) * glyphCount);

		cache->values[insertIdx].direction = hb_buffer_get_direction(hb_buffer);
		cache->values[insertIdx].lastUsed = cache->frameCounter++;
		cache->values[insertIdx].valid = 1;
	} else {
		// Allocation failed - invalidate entry
		if (cache->values[insertIdx].glyphInfo) free(cache->values[insertIdx].glyphInfo);
		if (cache->values[insertIdx].glyphPos) free(cache->values[insertIdx].glyphPos);
		cache->values[insertIdx].glyphInfo = NULL;
		cache->values[insertIdx].glyphPos = NULL;
		cache->values[insertIdx].valid = 0;
		if (cache->keys[insertIdx].text) {
			free(cache->keys[insertIdx].text);
			cache->keys[insertIdx].text = NULL;
		}
	}
}

// Clear entire cache
void nvgShapeCache_clear(NVGShapedTextCache* cache) {
	if (!cache) return;

	for (int i = 0; i < cache->count; i++) {
		if (cache->keys[i].text) {
			free(cache->keys[i].text);
			cache->keys[i].text = NULL;
		}
		if (cache->values[i].glyphInfo) {
			free(cache->values[i].glyphInfo);
			cache->values[i].glyphInfo = NULL;
		}
		if (cache->values[i].glyphPos) {
			free(cache->values[i].glyphPos);
			cache->values[i].glyphPos = NULL;
		}
		cache->values[i].valid = 0;
	}

	memset(cache, 0, sizeof(NVGShapedTextCache));
}

// Invalidate all cache entries for a specific font
void nvgShapeCache_invalidateFont(NVGShapedTextCache* cache, int fontId) {
	if (!cache) return;

	for (int i = 0; i < cache->count; i++) {
		if (cache->keys[i].fontId == fontId) {
			// Free this entry
			if (cache->keys[i].text) {
				free(cache->keys[i].text);
				cache->keys[i].text = NULL;
			}
			if (cache->values[i].glyphInfo) {
				free(cache->values[i].glyphInfo);
				cache->values[i].glyphInfo = NULL;
			}
			if (cache->values[i].glyphPos) {
				free(cache->values[i].glyphPos);
				cache->values[i].glyphPos = NULL;
			}
			cache->values[i].valid = 0;
		}
	}
}
