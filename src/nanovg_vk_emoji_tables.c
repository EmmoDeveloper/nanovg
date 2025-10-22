// nanovg_vk_emoji_tables.c - Color Emoji Font Table Parsing Implementation

#include "nanovg_vk_emoji_tables.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// Font Table Utilities
// ============================================================================

// OpenType/TrueType font header
typedef struct {
	uint32_t sfntVersion;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
} VKNVGfontHeader;

// Table directory entry
typedef struct {
	uint8_t tag[4];
	uint32_t checksum;
	uint32_t offset;
	uint32_t length;
} VKNVGtableEntry;

// Find table in font
int vknvg__getFontTable(const uint8_t* fontData, uint32_t fontDataSize,
                        const char* tag, uint32_t* outOffset, uint32_t* outSize)
{
	if (!fontData || fontDataSize < 12) return 0;

	// Read font header
	uint16_t numTables = vknvg__readU16BE(fontData + 4);

	// Search table directory
	const uint8_t* tableDir = fontData + 12;
	for (uint16_t i = 0; i < numTables; i++) {
		const uint8_t* entry = tableDir + (i * 16);
		if (entry + 16 > fontData + fontDataSize) return 0;

		if (vknvg__tagEquals(entry, tag)) {
			uint32_t offset = vknvg__readU32BE(entry + 8);
			uint32_t size = vknvg__readU32BE(entry + 12);

			// Validate bounds
			if (offset + size > fontDataSize) return 0;

			*outOffset = offset;
			*outSize = size;
			return 1;
		}
	}

	return 0;
}

// Check if font has table
int vknvg__hasFontTable(const uint8_t* fontData, uint32_t fontDataSize, const char* tag)
{
	uint32_t offset, size;
	return vknvg__getFontTable(fontData, fontDataSize, tag, &offset, &size);
}

// Detect emoji format
VKNVGemojiFormat vknvg__detectEmojiFormat(const uint8_t* fontData, uint32_t fontDataSize)
{
	// Priority: COLR > SBIX > CBDT
	if (vknvg__hasFontTable(fontData, fontDataSize, "COLR") &&
	    vknvg__hasFontTable(fontData, fontDataSize, "CPAL")) {
		return VKNVG_EMOJI_COLR;
	}

	if (vknvg__hasFontTable(fontData, fontDataSize, "sbix")) {
		return VKNVG_EMOJI_SBIX;
	}

	if (vknvg__hasFontTable(fontData, fontDataSize, "CBDT") &&
	    vknvg__hasFontTable(fontData, fontDataSize, "CBLC")) {
		return VKNVG_EMOJI_CBDT;
	}

	if (vknvg__hasFontTable(fontData, fontDataSize, "SVG ")) {
		return VKNVG_EMOJI_SVG;
	}

	return VKNVG_EMOJI_NONE;
}

// ============================================================================
// SBIX Parser
// ============================================================================

VKNVGsbixTable* vknvg__parseSbixTable(const uint8_t* fontData, uint32_t fontDataSize)
{
	uint32_t offset, size;
	if (!vknvg__getFontTable(fontData, fontDataSize, "sbix", &offset, &size)) {
		return NULL;
	}

	const uint8_t* table = fontData + offset;
	if (size < 8) return NULL;

	VKNVGsbixTable* sbix = (VKNVGsbixTable*)calloc(1, sizeof(VKNVGsbixTable));
	if (!sbix) return NULL;

	// Read header
	sbix->version = vknvg__readU16BE(table);
	sbix->flags = vknvg__readU16BE(table + 2);
	sbix->numStrikes = vknvg__readU32BE(table + 4);

	if (sbix->numStrikes == 0 || sbix->numStrikes > 100) {
		free(sbix);
		return NULL;
	}

	// Allocate strikes
	sbix->strikes = (VKNVGsbixStrike*)calloc(sbix->numStrikes, sizeof(VKNVGsbixStrike));
	if (!sbix->strikes) {
		free(sbix);
		return NULL;
	}

	// Read strike offsets (after header)
	const uint8_t* strikeOffsets = table + 8;
	for (uint32_t i = 0; i < sbix->numStrikes; i++) {
		uint32_t strikeOffset = vknvg__readU32BE(strikeOffsets + i * 4);
		if (strikeOffset + 8 > size) continue;

		const uint8_t* strike = table + strikeOffset;
		VKNVGsbixStrike* s = &sbix->strikes[i];

		// Read strike header
		s->ppem = vknvg__readU16BE(strike);
		s->resolution = vknvg__readU16BE(strike + 2);

		// Get 'maxp' table to know numGlyphs
		uint32_t maxpOffset, maxpSize;
		if (vknvg__getFontTable(fontData, fontDataSize, "maxp", &maxpOffset, &maxpSize)) {
			const uint8_t* maxp = fontData + maxpOffset;
			if (maxpSize >= 6) {
				s->numGlyphs = vknvg__readU16BE(maxp + 4);
			}
		}

		if (s->numGlyphs == 0) s->numGlyphs = 1000; // Reasonable default

		// Allocate glyph offsets
		s->glyphOffsets = (uint32_t*)malloc((s->numGlyphs + 1) * sizeof(uint32_t));
		if (!s->glyphOffsets) continue;

		// Read glyph data offsets
		const uint8_t* offsets = strike + 4;
		for (uint32_t g = 0; g <= s->numGlyphs; g++) {
			if (offsets + g * 4 >= table + size) {
				s->glyphOffsets[g] = 0;
			} else {
				s->glyphOffsets[g] = vknvg__readU32BE(offsets + g * 4);
			}
		}

		// Store strike data pointer
		s->strikeData = (uint8_t*)(strike + 4);
		s->strikeDataSize = size - (strikeOffset + 4);
	}

	return sbix;
}

void vknvg__freeSbixTable(VKNVGsbixTable* table)
{
	if (!table) return;

	if (table->strikes) {
		for (uint32_t i = 0; i < table->numStrikes; i++) {
			free(table->strikes[i].glyphOffsets);
		}
		free(table->strikes);
	}
	free(table);
}

VKNVGsbixStrike* vknvg__selectSbixStrike(VKNVGsbixTable* table, float size)
{
	if (!table || table->numStrikes == 0) return NULL;

	// Find closest ppem
	VKNVGsbixStrike* best = &table->strikes[0];
	int bestDiff = abs((int)best->ppem - (int)size);

	for (uint32_t i = 1; i < table->numStrikes; i++) {
		int diff = abs((int)table->strikes[i].ppem - (int)size);
		if (diff < bestDiff) {
			bestDiff = diff;
			best = &table->strikes[i];
		}
	}

	return best;
}

VKNVGsbixGlyphData* vknvg__getSbixGlyphData(VKNVGsbixStrike* strike, uint32_t glyphID)
{
	if (!strike || !strike->glyphOffsets || glyphID >= strike->numGlyphs) {
		return NULL;
	}

	uint32_t offset = strike->glyphOffsets[glyphID];
	uint32_t nextOffset = strike->glyphOffsets[glyphID + 1];

	if (nextOffset <= offset || offset + 8 > strike->strikeDataSize) {
		return NULL;
	}

	const uint8_t* glyphData = strike->strikeData + offset;
	uint32_t dataLength = nextOffset - offset;

	VKNVGsbixGlyphData* data = (VKNVGsbixGlyphData*)malloc(sizeof(VKNVGsbixGlyphData));
	if (!data) return NULL;

	// Read glyph data header
	data->originOffsetX = vknvg__readI16BE(glyphData);
	data->originOffsetY = vknvg__readI16BE(glyphData + 2);
	data->graphicType = vknvg__readU32BE(glyphData + 4);

	// Copy image data
	data->dataLength = dataLength - 8;
	data->data = (uint8_t*)malloc(data->dataLength);
	if (!data->data) {
		free(data);
		return NULL;
	}
	memcpy(data->data, glyphData + 8, data->dataLength);

	return data;
}

void vknvg__freeSbixGlyphData(VKNVGsbixGlyphData* data)
{
	if (!data) return;
	free(data->data);
	free(data);
}

// ============================================================================
// CBDT/CBLC Parser (Simplified)
// ============================================================================

VKNVGcbdtTable* vknvg__parseCbdtTable(const uint8_t* fontData, uint32_t fontDataSize)
{
	uint32_t cbdtOffset, cbdtSize, cblcOffset, cblcSize;

	if (!vknvg__getFontTable(fontData, fontDataSize, "CBDT", &cbdtOffset, &cbdtSize) ||
	    !vknvg__getFontTable(fontData, fontDataSize, "CBLC", &cblcOffset, &cblcSize)) {
		return NULL;
	}

	VKNVGcbdtTable* cbdt = (VKNVGcbdtTable*)calloc(1, sizeof(VKNVGcbdtTable));
	if (!cbdt) return NULL;

	// Read CBDT header
	const uint8_t* cbdtTable = fontData + cbdtOffset;
	cbdt->majorVersion = vknvg__readU16BE(cbdtTable);
	cbdt->minorVersion = vknvg__readU16BE(cbdtTable + 2);
	cbdt->tableData = (uint8_t*)(cbdtTable);
	cbdt->tableSize = cbdtSize;

	// Parse CBLC
	const uint8_t* cblcTable = fontData + cblcOffset;
	if (cblcSize < 8) {
		free(cbdt);
		return NULL;
	}

	cbdt->cblc = (VKNVGcblcTable*)calloc(1, sizeof(VKNVGcblcTable));
	if (!cbdt->cblc) {
		free(cbdt);
		return NULL;
	}

	cbdt->cblc->majorVersion = vknvg__readU16BE(cblcTable);
	cbdt->cblc->minorVersion = vknvg__readU16BE(cblcTable + 2);
	cbdt->cblc->numSizes = vknvg__readU32BE(cblcTable + 4);

	if (cbdt->cblc->numSizes == 0 || cbdt->cblc->numSizes > 50) {
		vknvg__freeCbdtTable(cbdt);
		return NULL;
	}

	// Allocate bitmap sizes
	cbdt->cblc->bitmapSizes = (VKNVGcblcBitmapSize*)calloc(cbdt->cblc->numSizes, sizeof(VKNVGcblcBitmapSize));
	if (!cbdt->cblc->bitmapSizes) {
		vknvg__freeCbdtTable(cbdt);
		return NULL;
	}

	// Parse bitmap size array (simplified - just read metadata)
	const uint8_t* sizesArray = cblcTable + 8;
	for (uint32_t i = 0; i < cbdt->cblc->numSizes; i++) {
		if (sizesArray + 48 > cblcTable + cblcSize) break;

		VKNVGcblcBitmapSize* size = &cbdt->cblc->bitmapSizes[i];
		size->indexSubTableArrayOffset = vknvg__readU32BE(sizesArray);
		size->indexTablesSize = vknvg__readU32BE(sizesArray + 4);
		size->numberOfIndexSubTables = vknvg__readU32BE(sizesArray + 8);

		// Skip to ppem info (at offset 44-47 in BitmapSize record)
		size->ppemX = sizesArray[44];
		size->ppemY = sizesArray[45];
		size->bitDepth = sizesArray[46];
		size->flags = sizesArray[47];

		sizesArray += 48;
	}

	return cbdt;
}

void vknvg__freeCbdtTable(VKNVGcbdtTable* table)
{
	if (!table) return;

	if (table->cblc) {
		if (table->cblc->bitmapSizes) {
			for (uint32_t i = 0; i < table->cblc->numSizes; i++) {
				free(table->cblc->bitmapSizes[i].subtables);
			}
			free(table->cblc->bitmapSizes);
		}
		free(table->cblc);
	}
	free(table);
}

VKNVGcblcBitmapSize* vknvg__selectCbdtSize(VKNVGcbdtTable* table, float size)
{
	if (!table || !table->cblc || table->cblc->numSizes == 0) return NULL;

	// Find closest ppem
	VKNVGcblcBitmapSize* best = &table->cblc->bitmapSizes[0];
	int bestDiff = abs((int)best->ppemY - (int)size);

	for (uint32_t i = 1; i < table->cblc->numSizes; i++) {
		int diff = abs((int)table->cblc->bitmapSizes[i].ppemY - (int)size);
		if (diff < bestDiff) {
			bestDiff = diff;
			best = &table->cblc->bitmapSizes[i];
		}
	}

	return best;
}

VKNVGcbdtGlyphData* vknvg__getCbdtGlyphData(VKNVGcbdtTable* table,
                                             VKNVGcblcBitmapSize* bitmapSize,
                                             uint32_t glyphID)
{
	// Simplified: This would require parsing index subtables
	// For now, return NULL (full implementation needed)
	(void)table;
	(void)bitmapSize;
	(void)glyphID;
	return NULL;
}

void vknvg__freeCbdtGlyphData(VKNVGcbdtGlyphData* data)
{
	if (!data) return;
	free(data->data);
	free(data);
}

// ============================================================================
// COLR/CPAL Parser
// ============================================================================

VKNVGcolrTable* vknvg__parseColrTable(const uint8_t* fontData, uint32_t fontDataSize)
{
	uint32_t offset, size;
	if (!vknvg__getFontTable(fontData, fontDataSize, "COLR", &offset, &size)) {
		return NULL;
	}

	const uint8_t* table = fontData + offset;
	if (size < 14) return NULL;

	VKNVGcolrTable* colr = (VKNVGcolrTable*)calloc(1, sizeof(VKNVGcolrTable));
	if (!colr) return NULL;

	// Read header
	colr->version = vknvg__readU16BE(table);
	colr->numBaseGlyphs = vknvg__readU16BE(table + 2);
	uint32_t baseGlyphsOffset = vknvg__readU32BE(table + 4);
	uint32_t layersOffset = vknvg__readU32BE(table + 8);
	uint16_t numLayers = vknvg__readU16BE(table + 12);

	if (colr->numBaseGlyphs == 0 || baseGlyphsOffset + colr->numBaseGlyphs * 6 > size) {
		free(colr);
		return NULL;
	}

	// Allocate glyphs
	colr->glyphs = (VKNVGcolrGlyph*)calloc(colr->numBaseGlyphs, sizeof(VKNVGcolrGlyph));
	if (!colr->glyphs) {
		free(colr);
		return NULL;
	}

	// Parse base glyph records
	const uint8_t* baseGlyphs = table + baseGlyphsOffset;
	const uint8_t* layerRecords = table + layersOffset;

	for (uint16_t i = 0; i < colr->numBaseGlyphs; i++) {
		const uint8_t* record = baseGlyphs + i * 6;
		VKNVGcolrGlyph* glyph = &colr->glyphs[i];

		glyph->glyphID = vknvg__readU16BE(record);
		uint16_t firstLayerIndex = vknvg__readU16BE(record + 2);
		glyph->numLayers = vknvg__readU16BE(record + 4);

		if (glyph->numLayers == 0 || firstLayerIndex + glyph->numLayers > numLayers) {
			continue;
		}

		// Allocate layers
		glyph->layers = (VKNVGcolrLayer*)malloc(glyph->numLayers * sizeof(VKNVGcolrLayer));
		if (!glyph->layers) continue;

		// Read layers
		for (uint16_t l = 0; l < glyph->numLayers; l++) {
			const uint8_t* layer = layerRecords + (firstLayerIndex + l) * 4;
			glyph->layers[l].glyphID = vknvg__readU16BE(layer);
			glyph->layers[l].paletteIndex = vknvg__readU16BE(layer + 2);
		}
	}

	// Build hash table for lookup
	colr->hashTableSize = colr->numBaseGlyphs * 2; // 50% load factor
	colr->glyphHashTable = (uint32_t*)malloc(colr->hashTableSize * sizeof(uint32_t));
	if (colr->glyphHashTable) {
		memset(colr->glyphHashTable, 0xFF, colr->hashTableSize * sizeof(uint32_t));
		for (uint16_t i = 0; i < colr->numBaseGlyphs; i++) {
			uint32_t hash = colr->glyphs[i].glyphID % colr->hashTableSize;
			colr->glyphHashTable[hash] = i;
		}
	}

	return colr;
}

void vknvg__freeColrTable(VKNVGcolrTable* table)
{
	if (!table) return;

	if (table->glyphs) {
		for (uint16_t i = 0; i < table->numBaseGlyphs; i++) {
			free(table->glyphs[i].layers);
		}
		free(table->glyphs);
	}
	free(table->glyphHashTable);
	free(table);
}

VKNVGcolrGlyph* vknvg__getColrGlyph(VKNVGcolrTable* table, uint32_t glyphID)
{
	if (!table || !table->glyphHashTable) return NULL;

	// Hash table lookup
	uint32_t hash = glyphID % table->hashTableSize;
	uint32_t index = table->glyphHashTable[hash];

	if (index != 0xFFFFFFFF && index < table->numBaseGlyphs) {
		if (table->glyphs[index].glyphID == glyphID) {
			return &table->glyphs[index];
		}
	}

	// Linear search fallback
	for (uint16_t i = 0; i < table->numBaseGlyphs; i++) {
		if (table->glyphs[i].glyphID == glyphID) {
			return &table->glyphs[i];
		}
	}

	return NULL;
}

VKNVGcpalTable* vknvg__parseCpalTable(const uint8_t* fontData, uint32_t fontDataSize)
{
	uint32_t offset, size;
	if (!vknvg__getFontTable(fontData, fontDataSize, "CPAL", &offset, &size)) {
		return NULL;
	}

	const uint8_t* table = fontData + offset;
	if (size < 12) return NULL;

	VKNVGcpalTable* cpal = (VKNVGcpalTable*)calloc(1, sizeof(VKNVGcpalTable));
	if (!cpal) return NULL;

	// Read header
	cpal->version = vknvg__readU16BE(table);
	cpal->numPaletteEntries = vknvg__readU16BE(table + 2);
	cpal->numPalettes = vknvg__readU16BE(table + 4);
	cpal->numColorRecords = vknvg__readU16BE(table + 6);
	uint32_t colorRecordsOffset = vknvg__readU32BE(table + 8);

	if (cpal->numPalettes == 0 || cpal->numColorRecords == 0 ||
	    colorRecordsOffset + cpal->numColorRecords * 4 > size) {
		free(cpal);
		return NULL;
	}

	// Read color records
	cpal->colorRecords = (VKNVGcpalColor*)malloc(cpal->numColorRecords * sizeof(VKNVGcpalColor));
	if (!cpal->colorRecords) {
		free(cpal);
		return NULL;
	}

	const uint8_t* colors = table + colorRecordsOffset;
	for (uint16_t i = 0; i < cpal->numColorRecords; i++) {
		cpal->colorRecords[i].blue = colors[i * 4 + 0];
		cpal->colorRecords[i].green = colors[i * 4 + 1];
		cpal->colorRecords[i].red = colors[i * 4 + 2];
		cpal->colorRecords[i].alpha = colors[i * 4 + 3];
	}

	// Build palettes
	cpal->palettes = (VKNVGcpalPalette*)malloc(cpal->numPalettes * sizeof(VKNVGcpalPalette));
	if (!cpal->palettes) {
		vknvg__freeCpalTable(cpal);
		return NULL;
	}

	// Read palette offsets (after header at offset 12)
	const uint8_t* paletteOffsets = table + 12;
	for (uint16_t p = 0; p < cpal->numPalettes; p++) {
		uint16_t firstColorIndex = vknvg__readU16BE(paletteOffsets + p * 2);
		cpal->palettes[p].numColors = cpal->numPaletteEntries;
		cpal->palettes[p].colors = &cpal->colorRecords[firstColorIndex];
	}

	return cpal;
}

void vknvg__freeCpalTable(VKNVGcpalTable* table)
{
	if (!table) return;
	free(table->colorRecords);
	free(table->palettes);
	free(table);
}

VKNVGcpalColor vknvg__getCpalColor(VKNVGcpalTable* table, uint16_t paletteIndex, uint16_t colorIndex)
{
	VKNVGcpalColor black = {0, 0, 0, 255};

	if (!table || paletteIndex >= table->numPalettes) return black;
	if (colorIndex >= table->palettes[paletteIndex].numColors) return black;

	return table->palettes[paletteIndex].colors[colorIndex];
}
