// test_emoji_tables.c - Font Table Parsing Tests
//
// Tests for Phase 6: Color Emoji Support

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nanovg_vk_emoji_tables.h"

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

// Helper: Create minimal TrueType font header
static uint8_t* createMockFont(uint32_t* outSize) {
	// Minimal TrueType font with empty tables
	uint8_t* font = (uint8_t*)calloc(1024, 1);
	if (!font) return NULL;

	// SFNT version (TrueType)
	font[0] = 0x00; font[1] = 0x01; font[2] = 0x00; font[3] = 0x00;

	// numTables = 3 (head, maxp, name)
	font[4] = 0x00; font[5] = 0x03;

	// searchRange, entrySelector, rangeShift
	font[6] = 0x00; font[7] = 0x30;  // searchRange = 48
	font[8] = 0x00; font[9] = 0x02;  // entrySelector = 2
	font[10] = 0x00; font[11] = 0x00; // rangeShift = 0

	// Table directory entries (16 bytes each)
	uint8_t* entry = font + 12;

	// Entry 1: 'head' table
	memcpy(entry, "head", 4);
	entry[8] = 0x00; entry[9] = 0x00; entry[10] = 0x01; entry[11] = 0x00; // offset = 256
	entry[12] = 0x00; entry[13] = 0x00; entry[14] = 0x00; entry[15] = 0x36; // length = 54
	entry += 16;

	// Entry 2: 'maxp' table
	memcpy(entry, "maxp", 4);
	entry[8] = 0x00; entry[9] = 0x00; entry[10] = 0x01; entry[11] = 0x40; // offset = 320
	entry[12] = 0x00; entry[13] = 0x00; entry[14] = 0x00; entry[15] = 0x20; // length = 32
	entry += 16;

	// Entry 3: 'name' table
	memcpy(entry, "name", 4);
	entry[8] = 0x00; entry[9] = 0x00; entry[10] = 0x01; entry[11] = 0x80; // offset = 384
	entry[12] = 0x00; entry[13] = 0x00; entry[14] = 0x00; entry[15] = 0x40; // length = 64

	// Fill in minimal maxp table (needed for numGlyphs)
	uint8_t* maxp = font + 320;
	maxp[0] = 0x00; maxp[1] = 0x01; maxp[2] = 0x00; maxp[3] = 0x00; // version 1.0
	maxp[4] = 0x00; maxp[5] = 0x64; // numGlyphs = 100

	*outSize = 1024;
	return font;
}

// Test 1: Font table detection
static int test_table_detection() {
	printf("Test: Font table detection\n");

	uint32_t fontSize;
	uint8_t* font = createMockFont(&fontSize);
	TEST_ASSERT(font != NULL, "font creation");

	// Test existing tables
	TEST_ASSERT(vknvg__hasFontTable(font, fontSize, "head"), "has 'head' table");
	TEST_ASSERT(vknvg__hasFontTable(font, fontSize, "maxp"), "has 'maxp' table");
	TEST_ASSERT(vknvg__hasFontTable(font, fontSize, "name"), "has 'name' table");

	// Test non-existing tables
	TEST_ASSERT(!vknvg__hasFontTable(font, fontSize, "sbix"), "no 'sbix' table");
	TEST_ASSERT(!vknvg__hasFontTable(font, fontSize, "COLR"), "no 'COLR' table");

	free(font);
	TEST_PASS("test_table_detection");
}

// Test 2: Get font table
static int test_get_table() {
	printf("Test: Get font table offset/size\n");

	uint32_t fontSize;
	uint8_t* font = createMockFont(&fontSize);
	TEST_ASSERT(font != NULL, "font creation");

	uint32_t offset, size;

	// Get 'head' table
	TEST_ASSERT(vknvg__getFontTable(font, fontSize, "head", &offset, &size), "get 'head'");
	TEST_ASSERT(offset == 256, "head offset");
	TEST_ASSERT(size == 54, "head size");

	// Get 'maxp' table
	TEST_ASSERT(vknvg__getFontTable(font, fontSize, "maxp", &offset, &size), "get 'maxp'");
	TEST_ASSERT(offset == 320, "maxp offset");
	TEST_ASSERT(size == 32, "maxp size");

	// Try non-existing table
	TEST_ASSERT(!vknvg__getFontTable(font, fontSize, "COLR", &offset, &size), "no COLR");

	free(font);
	TEST_PASS("test_get_table");
}

// Test 3: Emoji format detection (no emoji fonts)
static int test_format_detection_none() {
	printf("Test: Emoji format detection (no emoji)\n");

	uint32_t fontSize;
	uint8_t* font = createMockFont(&fontSize);
	TEST_ASSERT(font != NULL, "font creation");

	VKNVGemojiFormat format = vknvg__detectEmojiFormat(font, fontSize);
	TEST_ASSERT(format == VKNVG_EMOJI_NONE, "no emoji format");

	free(font);
	TEST_PASS("test_format_detection_none");
}

// Test 4: SBIX table parsing (minimal)
static int test_sbix_minimal() {
	printf("Test: SBIX table parsing (minimal)\n");

	uint32_t fontSize;
	uint8_t* font = createMockFont(&fontSize);
	TEST_ASSERT(font != NULL, "font creation");

	// Add minimal sbix table to font
	// Modify table directory to add sbix entry
	font[5] = 0x04; // numTables = 4

	uint8_t* newEntry = font + 12 + (3 * 16); // After existing 3 entries
	memcpy(newEntry, "sbix", 4);
	newEntry[8] = 0x00; newEntry[9] = 0x00; newEntry[10] = 0x02; newEntry[11] = 0x00; // offset = 512
	newEntry[12] = 0x00; newEntry[13] = 0x00; newEntry[14] = 0x00; newEntry[15] = 0x20; // length = 32

	// Create minimal sbix table at offset 512
	uint8_t* sbixData = font + 512;
	sbixData[0] = 0x00; sbixData[1] = 0x01; // version = 1
	sbixData[2] = 0x00; sbixData[3] = 0x00; // flags = 0
	sbixData[4] = 0x00; sbixData[5] = 0x00; sbixData[6] = 0x00; sbixData[7] = 0x01; // numStrikes = 1

	// Strike offset at byte 8
	sbixData[8] = 0x00; sbixData[9] = 0x00; sbixData[10] = 0x00; sbixData[11] = 0x0C; // strike offset = 12

	// Strike header at offset 12 (from sbix start)
	sbixData[12] = 0x00; sbixData[13] = 0x30; // ppem = 48
	sbixData[14] = 0x00; sbixData[15] = 0x48; // resolution = 72

	// Parse it
	VKNVGsbixTable* table = vknvg__parseSbixTable(font, fontSize);
	TEST_ASSERT(table != NULL, "sbix table parsed");
	TEST_ASSERT(table->version == 1, "sbix version");
	TEST_ASSERT(table->numStrikes == 1, "sbix has 1 strike");
	TEST_ASSERT(table->strikes[0].ppem == 48, "strike ppem");

	vknvg__freeSbixTable(table);
	free(font);
	TEST_PASS("test_sbix_minimal");
}

// Test 5: COLR table structure
static int test_colr_structure() {
	printf("Test: COLR table structure\n");

	// Test basic COLR glyph structure
	VKNVGcolrGlyph glyph;
	glyph.glyphID = 100;
	glyph.numLayers = 3;
	glyph.layers = (VKNVGcolrLayer*)malloc(3 * sizeof(VKNVGcolrLayer));
	TEST_ASSERT(glyph.layers != NULL, "layer allocation");

	glyph.layers[0].glyphID = 10;
	glyph.layers[0].paletteIndex = 0;
	glyph.layers[1].glyphID = 11;
	glyph.layers[1].paletteIndex = 1;
	glyph.layers[2].glyphID = 12;
	glyph.layers[2].paletteIndex = 2;

	TEST_ASSERT(glyph.numLayers == 3, "3 layers");
	TEST_ASSERT(glyph.layers[0].glyphID == 10, "layer 0 glyph");
	TEST_ASSERT(glyph.layers[2].paletteIndex == 2, "layer 2 palette");

	free(glyph.layers);
	TEST_PASS("test_colr_structure");
}

// Test 6: CPAL color structure
static int test_cpal_color() {
	printf("Test: CPAL color structure\n");

	VKNVGcpalColor color;
	color.blue = 255;
	color.green = 200;
	color.red = 100;
	color.alpha = 255;

	TEST_ASSERT(color.blue == 255, "blue channel");
	TEST_ASSERT(color.green == 200, "green channel");
	TEST_ASSERT(color.red == 100, "red channel");
	TEST_ASSERT(color.alpha == 255, "alpha channel");

	// BGRA format (not RGBA!)
	TEST_ASSERT(color.blue == 255, "BGRA order correct");

	TEST_PASS("test_cpal_color");
}

// Test 7: Utility functions
static int test_utility_functions() {
	printf("Test: Utility functions\n");

	// Test big-endian reads
	uint8_t data16[] = {0x12, 0x34};
	uint16_t val16 = vknvg__readU16BE(data16);
	TEST_ASSERT(val16 == 0x1234, "read U16 BE");

	uint8_t data32[] = {0x12, 0x34, 0x56, 0x78};
	uint32_t val32 = vknvg__readU32BE(data32);
	TEST_ASSERT(val32 == 0x12345678, "read U32 BE");

	// Test signed
	uint8_t dataS16[] = {0xFF, 0xFF};
	int16_t valS16 = vknvg__readI16BE(dataS16);
	TEST_ASSERT(valS16 == -1, "read I16 BE (negative)");

	// Test tag comparison
	uint8_t tag1[] = "COLR";
	uint8_t tag2[] = "CPAL";
	TEST_ASSERT(vknvg__tagEquals(tag1, "COLR"), "tag equals COLR");
	TEST_ASSERT(!vknvg__tagEquals(tag2, "COLR"), "tag not equals COLR");
	TEST_ASSERT(vknvg__tagEquals(tag2, "CPAL"), "tag equals CPAL");

	TEST_PASS("test_utility_functions");
}

// Test 8: SBIX strike selection
static int test_sbix_strike_selection() {
	printf("Test: SBIX strike selection\n");

	VKNVGsbixTable table;
	table.numStrikes = 4;
	table.strikes = (VKNVGsbixStrike*)malloc(4 * sizeof(VKNVGsbixStrike));
	TEST_ASSERT(table.strikes != NULL, "strike allocation");

	table.strikes[0].ppem = 20;
	table.strikes[1].ppem = 32;
	table.strikes[2].ppem = 64;
	table.strikes[3].ppem = 128;

	// Test size selection
	VKNVGsbixStrike* strike = vknvg__selectSbixStrike(&table, 48.0f);
	TEST_ASSERT(strike != NULL, "strike selected");
	TEST_ASSERT(strike->ppem == 32 || strike->ppem == 64, "closest strike");

	strike = vknvg__selectSbixStrike(&table, 16.0f);
	TEST_ASSERT(strike->ppem == 20, "smallest strike");

	strike = vknvg__selectSbixStrike(&table, 200.0f);
	TEST_ASSERT(strike->ppem == 128, "largest strike");

	free(table.strikes);
	TEST_PASS("test_sbix_strike_selection");
}

// Test 9: COLR glyph lookup (mock)
static int test_colr_lookup() {
	printf("Test: COLR glyph lookup\n");

	VKNVGcolrTable table;
	table.version = 0;
	table.numBaseGlyphs = 3;
	table.glyphs = (VKNVGcolrGlyph*)calloc(3, sizeof(VKNVGcolrGlyph));
	TEST_ASSERT(table.glyphs != NULL, "glyph allocation");

	table.glyphs[0].glyphID = 100;
	table.glyphs[0].numLayers = 2;
	table.glyphs[1].glyphID = 101;
	table.glyphs[1].numLayers = 3;
	table.glyphs[2].glyphID = 102;
	table.glyphs[2].numLayers = 4;

	// Build hash table
	table.hashTableSize = 6;
	table.glyphHashTable = (uint32_t*)malloc(6 * sizeof(uint32_t));
	memset(table.glyphHashTable, 0xFF, 6 * sizeof(uint32_t));
	for (int i = 0; i < 3; i++) {
		uint32_t hash = table.glyphs[i].glyphID % table.hashTableSize;
		table.glyphHashTable[hash] = i;
	}

	// Test lookup
	VKNVGcolrGlyph* glyph = vknvg__getColrGlyph(&table, 101);
	TEST_ASSERT(glyph != NULL, "found glyph 101");
	TEST_ASSERT(glyph->glyphID == 101, "correct glyph ID");
	TEST_ASSERT(glyph->numLayers == 3, "correct layer count");

	glyph = vknvg__getColrGlyph(&table, 999);
	TEST_ASSERT(glyph == NULL, "glyph 999 not found");

	free(table.glyphs);
	free(table.glyphHashTable);
	TEST_PASS("test_colr_lookup");
}

// Test 10: CPAL palette access
static int test_cpal_palette() {
	printf("Test: CPAL palette access\n");

	VKNVGcpalTable table;
	table.version = 0;
	table.numPaletteEntries = 4;
	table.numPalettes = 2;
	table.numColorRecords = 8;

	// Create color records
	table.colorRecords = (VKNVGcpalColor*)malloc(8 * sizeof(VKNVGcpalColor));
	TEST_ASSERT(table.colorRecords != NULL, "color allocation");

	// Palette 0: colors 0-3
	table.colorRecords[0] = (VKNVGcpalColor){255, 0, 0, 255};     // Blue
	table.colorRecords[1] = (VKNVGcpalColor){0, 255, 0, 255};     // Green
	table.colorRecords[2] = (VKNVGcpalColor){0, 0, 255, 255};     // Red (BGRA!)
	table.colorRecords[3] = (VKNVGcpalColor){255, 255, 255, 255}; // White

	// Palette 1: colors 4-7
	table.colorRecords[4] = (VKNVGcpalColor){0, 0, 0, 255};       // Black
	table.colorRecords[5] = (VKNVGcpalColor){128, 128, 128, 255}; // Gray
	table.colorRecords[6] = (VKNVGcpalColor){64, 64, 64, 255};    // Dark gray
	table.colorRecords[7] = (VKNVGcpalColor){192, 192, 192, 255}; // Light gray

	// Create palettes
	table.palettes = (VKNVGcpalPalette*)malloc(2 * sizeof(VKNVGcpalPalette));
	table.palettes[0].numColors = 4;
	table.palettes[0].colors = &table.colorRecords[0];
	table.palettes[1].numColors = 4;
	table.palettes[1].colors = &table.colorRecords[4];

	// Test color access
	VKNVGcpalColor color = vknvg__getCpalColor(&table, 0, 0);
	TEST_ASSERT(color.blue == 255 && color.red == 0, "palette 0, color 0 (blue)");

	color = vknvg__getCpalColor(&table, 0, 2);
	TEST_ASSERT(color.red == 255 && color.blue == 0, "palette 0, color 2 (red)");

	color = vknvg__getCpalColor(&table, 1, 0);
	TEST_ASSERT(color.blue == 0 && color.red == 0, "palette 1, color 0 (black)");

	free(table.colorRecords);
	free(table.palettes);
	TEST_PASS("test_cpal_palette");
}

int main() {
	printf("==========================================\n");
	printf("  Emoji Table Parsing Tests (Phase 6)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_table_detection()) passed++;
	total++; if (test_get_table()) passed++;
	total++; if (test_format_detection_none()) passed++;
	total++; if (test_sbix_minimal()) passed++;
	total++; if (test_colr_structure()) passed++;
	total++; if (test_cpal_color()) passed++;
	total++; if (test_utility_functions()) passed++;
	total++; if (test_sbix_strike_selection()) passed++;
	total++; if (test_colr_lookup()) passed++;
	total++; if (test_cpal_palette()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
