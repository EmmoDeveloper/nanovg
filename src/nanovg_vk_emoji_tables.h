// nanovg_vk_emoji_tables.h - Color Emoji Font Table Parsing
//
// Phase 6: Color Emoji Support
//
// Parsers for color emoji font formats:
// - SBIX: Apple's bitmap emoji (PNG embedded)
// - CBDT/CBLC: Google's bitmap emoji (PNG or raw BGRA)
// - COLR/CPAL: Vector layered color glyphs
//
// Font format specifications:
// - SBIX: https://docs.microsoft.com/en-us/typography/opentype/spec/sbix
// - CBDT/CBLC: https://docs.microsoft.com/en-us/typography/opentype/spec/cbdt
// - COLR: https://docs.microsoft.com/en-us/typography/opentype/spec/colr
// - CPAL: https://docs.microsoft.com/en-us/typography/opentype/spec/cpal

#ifndef NANOVG_VK_EMOJI_TABLES_H
#define NANOVG_VK_EMOJI_TABLES_H

#include <stdint.h>
#include <stdlib.h>

// Emoji format types
typedef enum VKNVGemojiFormat {
	VKNVG_EMOJI_NONE = 0,     // No color emoji support
	VKNVG_EMOJI_SBIX = 1,     // Apple bitmap (PNG)
	VKNVG_EMOJI_CBDT = 2,     // Google bitmap (PNG/BGRA)
	VKNVG_EMOJI_COLR = 3,     // Vector layered color
	VKNVG_EMOJI_SVG = 4,      // SVG-in-OpenType (future)
} VKNVGemojiFormat;

// ============================================================================
// SBIX (Apple Bitmap Emoji)
// ============================================================================

// SBIX strike (one size/resolution)
typedef struct VKNVGsbixStrike {
	uint16_t ppem;            // Pixels per em
	uint16_t resolution;      // DPI (usually 72)
	uint32_t numGlyphs;       // Number of glyphs in strike
	uint32_t* glyphOffsets;   // Offset array [numGlyphs + 1]
	uint8_t* strikeData;      // Raw strike data (contains all glyph data)
	uint32_t strikeDataSize;  // Size of strike data
} VKNVGsbixStrike;

// SBIX table (contains multiple strikes)
typedef struct VKNVGsbixTable {
	uint16_t version;         // Table version (usually 1)
	uint16_t flags;           // Flags
	uint32_t numStrikes;      // Number of strikes
	VKNVGsbixStrike* strikes; // Array of strikes
} VKNVGsbixTable;

// SBIX glyph data header
typedef struct VKNVGsbixGlyphData {
	int16_t originOffsetX;    // Horizontal origin offset
	int16_t originOffsetY;    // Vertical origin offset
	uint32_t graphicType;     // 'png ' or 'jpg ' or 'tiff' etc.
	uint8_t* data;            // PNG/image data
	uint32_t dataLength;      // Length of image data
} VKNVGsbixGlyphData;

// Parse SBIX table from font data
VKNVGsbixTable* vknvg__parseSbixTable(const uint8_t* fontData, uint32_t fontDataSize);

// Free SBIX table
void vknvg__freeSbixTable(VKNVGsbixTable* table);

// Select strike closest to target size
VKNVGsbixStrike* vknvg__selectSbixStrike(VKNVGsbixTable* table, float size);

// Get glyph data from strike
VKNVGsbixGlyphData* vknvg__getSbixGlyphData(VKNVGsbixStrike* strike, uint32_t glyphID);

// Free SBIX glyph data
void vknvg__freeSbixGlyphData(VKNVGsbixGlyphData* data);

// ============================================================================
// CBDT/CBLC (Google Bitmap Emoji)
// ============================================================================

// CBDT bitmap formats
typedef enum VKNVGcbdtFormat {
	VKNVG_CBDT_FORMAT_17 = 17, // Small metrics, PNG
	VKNVG_CBDT_FORMAT_18 = 18, // Big metrics, PNG
	VKNVG_CBDT_FORMAT_19 = 19, // Metrics in CBLC, PNG
} VKNVGcbdtFormat;

// Small glyph metrics
typedef struct VKNVGsmallGlyphMetrics {
	uint8_t height;
	uint8_t width;
	int8_t bearingX;
	int8_t bearingY;
	uint8_t advance;
} VKNVGsmallGlyphMetrics;

// Big glyph metrics
typedef struct VKNVGbigGlyphMetrics {
	uint8_t height;
	uint8_t width;
	int8_t horiBearingX;
	int8_t horiBearingY;
	uint8_t horiAdvance;
	int8_t vertBearingX;
	int8_t vertBearingY;
	uint8_t vertAdvance;
} VKNVGbigGlyphMetrics;

// CBDT glyph data
typedef struct VKNVGcbdtGlyphData {
	VKNVGcbdtFormat format;
	union {
		VKNVGsmallGlyphMetrics small;
		VKNVGbigGlyphMetrics big;
	} metrics;
	uint8_t* data;            // PNG or raw BGRA data
	uint32_t dataLength;
} VKNVGcbdtGlyphData;

// CBLC index subtable (simplified)
typedef struct VKNVGcblcIndexSubtable {
	uint16_t indexFormat;
	uint16_t imageFormat;
	uint32_t imageDataOffset;
	uint16_t firstGlyphIndex;
	uint16_t lastGlyphIndex;
	void* subtableData;       // Format-specific data
} VKNVGcblcIndexSubtable;

// CBLC bitmap size (one size/resolution)
typedef struct VKNVGcblcBitmapSize {
	uint32_t indexSubTableArrayOffset;
	uint32_t indexTablesSize;
	uint32_t numberOfIndexSubTables;
	uint8_t ppemX;
	uint8_t ppemY;
	uint8_t bitDepth;         // Always 32 for CBDT
	uint8_t flags;
	VKNVGcblcIndexSubtable* subtables;
} VKNVGcblcBitmapSize;

// CBLC table
typedef struct VKNVGcblcTable {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint32_t numSizes;
	VKNVGcblcBitmapSize* bitmapSizes;
} VKNVGcblcTable;

// CBDT table (references CBLC for metadata)
typedef struct VKNVGcbdtTable {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint8_t* tableData;       // Raw CBDT table data
	uint32_t tableSize;
	VKNVGcblcTable* cblc;     // Associated CBLC table
} VKNVGcbdtTable;

// Parse CBDT/CBLC tables
VKNVGcbdtTable* vknvg__parseCbdtTable(const uint8_t* fontData, uint32_t fontDataSize);

// Free CBDT table
void vknvg__freeCbdtTable(VKNVGcbdtTable* table);

// Select bitmap size closest to target
VKNVGcblcBitmapSize* vknvg__selectCbdtSize(VKNVGcbdtTable* table, float size);

// Get glyph data from CBDT
VKNVGcbdtGlyphData* vknvg__getCbdtGlyphData(VKNVGcbdtTable* table,
                                             VKNVGcblcBitmapSize* bitmapSize,
                                             uint32_t glyphID);

// Free CBDT glyph data
void vknvg__freeCbdtGlyphData(VKNVGcbdtGlyphData* data);

// ============================================================================
// COLR/CPAL (Vector Layered Color)
// ============================================================================

// COLR layer (one layer of a colored glyph)
typedef struct VKNVGcolrLayer {
	uint16_t glyphID;         // Glyph outline to render
	uint16_t paletteIndex;    // Color index in CPAL (0xFFFF = foreground)
} VKNVGcolrLayer;

// COLR glyph (base glyph with layers)
typedef struct VKNVGcolrGlyph {
	uint16_t glyphID;         // Base glyph ID
	uint16_t numLayers;       // Number of layers
	VKNVGcolrLayer* layers;   // Array of layers (bottom to top)
} VKNVGcolrGlyph;

// COLR table (version 0)
typedef struct VKNVGcolrTable {
	uint16_t version;         // Table version (0 or 1)
	uint16_t numBaseGlyphs;   // Number of base glyphs
	VKNVGcolrGlyph* glyphs;   // Array of colored glyphs

	// Hash table for quick lookup
	uint32_t* glyphHashTable; // Maps glyphID → index in glyphs array
	uint32_t hashTableSize;
} VKNVGcolrTable;

// CPAL color (BGRA in sRGB)
typedef struct VKNVGcpalColor {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t alpha;
} VKNVGcpalColor;

// CPAL palette
typedef struct VKNVGcpalPalette {
	uint16_t numColors;       // Number of colors in palette
	VKNVGcpalColor* colors;   // Array of BGRA colors
} VKNVGcpalPalette;

// CPAL table
typedef struct VKNVGcpalTable {
	uint16_t version;         // Table version (0 or 1)
	uint16_t numPaletteEntries;  // Colors per palette
	uint16_t numPalettes;     // Number of palettes
	uint16_t numColorRecords; // Total color records
	VKNVGcpalColor* colorRecords; // All color records
	VKNVGcpalPalette* palettes;   // Array of palettes
} VKNVGcpalTable;

// Parse COLR table
VKNVGcolrTable* vknvg__parseColrTable(const uint8_t* fontData, uint32_t fontDataSize);

// Free COLR table
void vknvg__freeColrTable(VKNVGcolrTable* table);

// Get COLR glyph layers (returns NULL if not a colored glyph)
VKNVGcolrGlyph* vknvg__getColrGlyph(VKNVGcolrTable* table, uint32_t glyphID);

// Parse CPAL table
VKNVGcpalTable* vknvg__parseCpalTable(const uint8_t* fontData, uint32_t fontDataSize);

// Free CPAL table
void vknvg__freeCpalTable(VKNVGcpalTable* table);

// Get color from palette
VKNVGcpalColor vknvg__getCpalColor(VKNVGcpalTable* table, uint16_t paletteIndex, uint16_t colorIndex);

// ============================================================================
// Font Format Detection
// ============================================================================

// Detect available emoji formats in font
VKNVGemojiFormat vknvg__detectEmojiFormat(const uint8_t* fontData, uint32_t fontDataSize);

// Check if font has specific table
int vknvg__hasFontTable(const uint8_t* fontData, uint32_t fontDataSize, const char* tag);

// Get font table offset and size
int vknvg__getFontTable(const uint8_t* fontData, uint32_t fontDataSize,
                        const char* tag, uint32_t* outOffset, uint32_t* outSize);

// ============================================================================
// Utility Functions
// ============================================================================

// Read big-endian uint16
static inline uint16_t vknvg__readU16BE(const uint8_t* data) {
	return (uint16_t)((data[0] << 8) | data[1]);
}

// Read big-endian uint32
static inline uint32_t vknvg__readU32BE(const uint8_t* data) {
	return (uint32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

// Read big-endian int16
static inline int16_t vknvg__readI16BE(const uint8_t* data) {
	return (int16_t)vknvg__readU16BE(data);
}

// Compare font table tag
static inline int vknvg__tagEquals(const uint8_t* data, const char* tag) {
	return data[0] == tag[0] && data[1] == tag[1] &&
	       data[2] == tag[2] && data[3] == tag[3];
}

#endif // NANOVG_VK_EMOJI_TABLES_H
