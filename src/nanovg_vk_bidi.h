// nanovg_vk_bidi.h - Unicode Bidirectional Algorithm (UAX #9)
//
// Implements Unicode BiDi algorithm for handling mixed LTR/RTL text:
// - Analyze text to determine directionality
// - Resolve embedding levels
// - Reorder text for visual display
// - Handle weak and neutral characters
// - Support explicit directional controls
//
// Based on Unicode Standard Annex #9 (UAX #9)
// https://www.unicode.org/reports/tr9/
//
// Architecture:
// - Character classification (L, R, AL, EN, etc.)
// - Paragraph level detection
// - Embedding level resolution
// - Visual reordering for rendering
// - Cursor positioning in logical order

#ifndef NANOVG_VK_BIDI_H
#define NANOVG_VK_BIDI_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define VKNVG_MAX_BIDI_TEXT_LENGTH 4096		// Max characters per paragraph
#define VKNVG_MAX_BIDI_LEVEL 125			// Max embedding level (spec allows 125)

// BiDi character types (from UAX #9)
typedef enum VKNVGbidiType {
	// Strong types
	VKNVG_BIDI_L = 0,		// Left-to-Right (Latin, Cyrillic, etc.)
	VKNVG_BIDI_R = 1,		// Right-to-Left (Hebrew)
	VKNVG_BIDI_AL = 2,		// Right-to-Left Arabic

	// Weak types
	VKNVG_BIDI_EN = 3,		// European Number
	VKNVG_BIDI_ES = 4,		// European Number Separator (+, -)
	VKNVG_BIDI_ET = 5,		// European Number Terminator ($, %)
	VKNVG_BIDI_AN = 6,		// Arabic Number
	VKNVG_BIDI_CS = 7,		// Common Number Separator (., ,)
	VKNVG_BIDI_NSM = 8,		// Non-Spacing Mark
	VKNVG_BIDI_BN = 9,		// Boundary Neutral (zero-width)

	// Neutral types
	VKNVG_BIDI_B = 10,		// Paragraph Separator
	VKNVG_BIDI_S = 11,		// Segment Separator
	VKNVG_BIDI_WS = 12,		// Whitespace
	VKNVG_BIDI_ON = 13,		// Other Neutral

	// Explicit formatting types
	VKNVG_BIDI_LRE = 14,	// Left-to-Right Embedding
	VKNVG_BIDI_LRO = 15,	// Left-to-Right Override
	VKNVG_BIDI_RLE = 16,	// Right-to-Left Embedding
	VKNVG_BIDI_RLO = 17,	// Right-to-Left Override
	VKNVG_BIDI_PDF = 18,	// Pop Directional Format
	VKNVG_BIDI_LRI = 19,	// Left-to-Right Isolate
	VKNVG_BIDI_RLI = 20,	// Right-to-Left Isolate
	VKNVG_BIDI_FSI = 21,	// First Strong Isolate
	VKNVG_BIDI_PDI = 22,	// Pop Directional Isolate
} VKNVGbidiType;

// Base paragraph direction
typedef enum VKNVGparagraphDirection {
	VKNVG_PARA_LTR = 0,		// Left-to-right paragraph
	VKNVG_PARA_RTL = 1,		// Right-to-left paragraph
	VKNVG_PARA_AUTO = 2,	// Auto-detect from first strong character
} VKNVGparagraphDirection;

// BiDi run (contiguous sequence of same level)
typedef struct VKNVGbidiRun {
	uint32_t start;			// Start character index (logical order)
	uint32_t length;		// Run length
	uint8_t level;			// Embedding level (even=LTR, odd=RTL)
	VKNVGbidiType type;		// Resolved character type
} VKNVGbidiRun;

// BiDi analysis result
typedef struct VKNVGbidiResult {
	// Per-character data
	uint8_t levels[VKNVG_MAX_BIDI_TEXT_LENGTH];		// Embedding levels
	VKNVGbidiType types[VKNVG_MAX_BIDI_TEXT_LENGTH];	// Character types
	uint32_t visualOrder[VKNVG_MAX_BIDI_TEXT_LENGTH];	// Logical → visual mapping
	uint32_t logicalOrder[VKNVG_MAX_BIDI_TEXT_LENGTH];	// Visual → logical mapping

	// Runs (for efficient rendering)
	VKNVGbidiRun runs[256];	// Max 256 runs per paragraph
	uint32_t runCount;

	// Metadata
	uint32_t textLength;	// Number of characters
	uint8_t baseLevel;		// Paragraph base level (0=LTR, 1=RTL)
	uint8_t maxLevel;		// Maximum embedding level in text
	uint32_t flags;			// Result flags
} VKNVGbidiResult;

// Result flags
#define VKNVG_BIDI_FLAG_HAS_RTL		(1 << 0)	// Contains RTL text
#define VKNVG_BIDI_FLAG_HAS_ARABIC	(1 << 1)	// Contains Arabic
#define VKNVG_BIDI_FLAG_HAS_NUMBERS	(1 << 2)	// Contains numbers
#define VKNVG_BIDI_FLAG_MIRRORED	(1 << 3)	// Has mirrored characters

// BiDi context (reusable for multiple analyses)
typedef struct VKNVGbidiContext {
	// Working buffers
	uint8_t levels[VKNVG_MAX_BIDI_TEXT_LENGTH];
	VKNVGbidiType types[VKNVG_MAX_BIDI_TEXT_LENGTH];
	uint32_t codepoints[VKNVG_MAX_BIDI_TEXT_LENGTH];

	// Statistics
	uint32_t analysisCount;
	uint32_t reorderingCount;
	uint32_t rtlTextCount;
} VKNVGbidiContext;

// API Functions

// Create BiDi context
VKNVGbidiContext* vknvg__createBidiContext(void);

// Destroy BiDi context
void vknvg__destroyBidiContext(VKNVGbidiContext* ctx);

// Analyze text and determine BiDi properties
// text: UTF-8 encoded text
// textLen: Length in bytes
// baseDirection: Paragraph direction (or AUTO)
// result: Output BiDi analysis
// Returns 1 on success, 0 on error
int vknvg__analyzeBidi(VKNVGbidiContext* ctx,
                       const char* text,
                       uint32_t textLen,
                       VKNVGparagraphDirection baseDirection,
                       VKNVGbidiResult* result);

// Reorder text for visual display (fills visualOrder array in result)
// Call after vknvg__analyzeBidi()
void vknvg__reorderBidi(VKNVGbidiContext* ctx, VKNVGbidiResult* result);

// Get character type for Unicode codepoint
VKNVGbidiType vknvg__getBidiType(uint32_t codepoint);

// Check if character should be mirrored in RTL context
// Returns mirrored codepoint, or original if no mirror
uint32_t vknvg__getMirroredChar(uint32_t codepoint);

// Detect paragraph base direction from text
VKNVGparagraphDirection vknvg__detectBaseDirection(const char* text, uint32_t textLen);

// Get statistics
void vknvg__getBidiStats(VKNVGbidiContext* ctx,
                         uint32_t* analysisCount,
                         uint32_t* reorderingCount,
                         uint32_t* rtlTextCount);

// Reset statistics
void vknvg__resetBidiStats(VKNVGbidiContext* ctx);

// Internal functions (BiDi algorithm implementation)
// (Implementations in nanovg_vk_bidi.c)

// Helper: Check if level is RTL
static inline bool vknvg__isRTL(uint8_t level)
{
	return (level & 1) != 0;
}

// Helper: Get embedding direction from level
static inline VKNVGbidiType vknvg__levelDirection(uint8_t level)
{
	return vknvg__isRTL(level) ? VKNVG_BIDI_R : VKNVG_BIDI_L;
}

// Unicode BiDi control characters
#define VKNVG_CHAR_LRM		0x200E	// Left-to-Right Mark
#define VKNVG_CHAR_RLM		0x200F	// Right-to-Left Mark
#define VKNVG_CHAR_ALM		0x061C	// Arabic Letter Mark
#define VKNVG_CHAR_LRE		0x202A	// Left-to-Right Embedding
#define VKNVG_CHAR_RLE		0x202B	// Right-to-Left Embedding
#define VKNVG_CHAR_PDF		0x202C	// Pop Directional Formatting
#define VKNVG_CHAR_LRO		0x202D	// Left-to-Right Override
#define VKNVG_CHAR_RLO		0x202E	// Right-to-Left Override
#define VKNVG_CHAR_LRI		0x2066	// Left-to-Right Isolate
#define VKNVG_CHAR_RLI		0x2067	// Right-to-Left Isolate
#define VKNVG_CHAR_FSI		0x2068	// First Strong Isolate
#define VKNVG_CHAR_PDI		0x2069	// Pop Directional Isolate

#endif // NANOVG_VK_BIDI_H
