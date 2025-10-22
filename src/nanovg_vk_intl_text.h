// nanovg_vk_intl_text.h - International Text Rendering Integration
//
// Combines HarfBuzz shaping and BiDi reordering for full international text support.
// Provides high-level API for rendering complex scripts with proper directionality.
//
// Architecture:
// - Text analysis (script detection, BiDi analysis)
// - Segmentation into homogeneous runs
// - Per-run shaping with HarfBuzz
// - BiDi reordering for visual display
// - Cursor positioning and hit testing
// - Integration with virtual atlas
//
// Supported features:
// - Arabic (contextual forms, ligatures, diacritics)
// - Hebrew (RTL, combining marks)
// - Devanagari (conjuncts, vowel signs, reordering)
// - Thai (vowel/tone marks, word boundaries)
// - Mixed LTR/RTL text (English + Arabic)
// - Vertical text (Mongolian, CJK)

#ifndef NANOVG_VK_INTL_TEXT_H
#define NANOVG_VK_INTL_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "nanovg_vk_harfbuzz.h"
#include "nanovg_vk_bidi.h"

// Configuration
#define VKNVG_MAX_TEXT_RUNS 64			// Max runs per text paragraph
#define VKNVG_MAX_GLYPHS_PER_RUN 512	// Max glyphs per run

// Text run (homogeneous script/direction/font segment)
typedef struct VKNVGtextRun {
	uint32_t textStart;			// Start byte index in UTF-8 text
	uint32_t textLength;		// Length in bytes
	uint32_t charStart;			// Start character index
	uint32_t charLength;		// Length in characters

	VKNVGscript script;			// Script for this run
	VKNVGtextDirection direction;	// Text direction
	uint8_t bidiLevel;			// BiDi embedding level
	int fontID;					// NanoVG font ID

	// Shaped glyphs
	VKNVGshapedGlyph glyphs[VKNVG_MAX_GLYPHS_PER_RUN];
	uint32_t glyphCount;

	// Metrics
	float width;				// Run width in pixels
	float height;				// Run height in pixels
	float ascent;				// Ascent from baseline
	float descent;				// Descent from baseline
} VKNVGtextRun;

// Shaped text paragraph (full analysis + shaping result)
typedef struct VKNVGshapedParagraph {
	// Input
	char* text;					// UTF-8 text (owned by caller)
	uint32_t textLength;		// Text length in bytes
	VKNVGparagraphDirection baseDirection;	// Paragraph direction

	// Analysis
	VKNVGbidiResult bidiResult;	// BiDi analysis
	VKNVGtextRun runs[VKNVG_MAX_TEXT_RUNS];	// Text runs
	uint32_t runCount;

	// Visual order (for rendering)
	uint32_t visualRunOrder[VKNVG_MAX_TEXT_RUNS];	// Run indices in visual order

	// Metrics
	float totalWidth;			// Total paragraph width
	float totalHeight;			// Total paragraph height
	float maxAscent;			// Maximum ascent
	float maxDescent;			// Maximum descent

	// Flags
	uint32_t flags;				// Paragraph flags
} VKNVGshapedParagraph;

// Paragraph flags
#define VKNVG_PARA_FLAG_HAS_RTL			(1 << 0)	// Contains RTL text
#define VKNVG_PARA_FLAG_HAS_COMPLEX		(1 << 1)	// Contains complex scripts
#define VKNVG_PARA_FLAG_HAS_VERTICAL	(1 << 2)	// Contains vertical text
#define VKNVG_PARA_FLAG_NEEDS_BIDI		(1 << 3)	// Needs BiDi processing

// International text context (combines HarfBuzz + BiDi)
typedef struct VKNVGintlTextContext {
	VKNVGharfbuzzContext* harfbuzz;	// HarfBuzz context
	VKNVGbidiContext* bidi;			// BiDi context

	// Configuration
	bool enableBidi;				// Enable BiDi processing
	bool enableShaping;				// Enable HarfBuzz shaping
	bool enableComplexScripts;		// Enable complex script features

	// Default features
	VKNVGfeature defaultFeatures[VKNVG_MAX_FEATURES];
	uint32_t defaultFeatureCount;

	// Statistics
	uint32_t paragraphsProcessed;
	uint32_t runsProcessed;
	uint32_t glyphsShaped;
	uint32_t bidiReorderings;
} VKNVGintlTextContext;

// Cursor position result
typedef struct VKNVGcursorPosition {
	uint32_t byteOffset;		// Byte offset in UTF-8 text
	uint32_t charOffset;		// Character offset
	uint32_t runIndex;			// Run containing cursor
	uint32_t glyphIndex;		// Glyph index within run
	float x;					// X position (pixels)
	float y;					// Y position (pixels)
	bool isTrailing;			// True if after character
} VKNVGcursorPosition;

// API Functions

// Create international text context
VKNVGintlTextContext* vknvg__createIntlTextContext(void);

// Destroy international text context
void vknvg__destroyIntlTextContext(VKNVGintlTextContext* ctx);

// Register font for international text rendering
int vknvg__registerIntlFont(VKNVGintlTextContext* ctx,
                            int nvgFontID,
                            FT_Face ftFace);

// Unregister font
void vknvg__unregisterIntlFont(VKNVGintlTextContext* ctx, int nvgFontID);

// Shape paragraph (full analysis + shaping + reordering)
// text: UTF-8 text
// textLen: Length in bytes
// fontID: Primary font ID
// baseDirection: Paragraph direction
// fontSize: Font size in pixels
// paragraph: Output shaped paragraph
// Returns 1 on success, 0 on error
int vknvg__shapeParagraph(VKNVGintlTextContext* ctx,
                          const char* text,
                          uint32_t textLen,
                          int fontID,
                          VKNVGparagraphDirection baseDirection,
                          float fontSize,
                          VKNVGshapedParagraph* paragraph);

// Free shaped paragraph resources
void vknvg__freeParagraph(VKNVGshapedParagraph* paragraph);

// Get cursor position from pixel coordinates (hit testing)
// paragraph: Shaped paragraph
// x, y: Pixel coordinates
// cursor: Output cursor position
// Returns 1 if hit, 0 if outside text
int vknvg__getCursorPosition(const VKNVGshapedParagraph* paragraph,
                             float x, float y,
                             VKNVGcursorPosition* cursor);

// Get pixel position from character offset (caret positioning)
// paragraph: Shaped paragraph
// charOffset: Character offset
// cursor: Output cursor position
// Returns 1 on success, 0 if offset invalid
int vknvg__getCharacterPosition(const VKNVGshapedParagraph* paragraph,
                                uint32_t charOffset,
                                VKNVGcursorPosition* cursor);

// Set default OpenType features (applied to all shaping)
void vknvg__setDefaultFeatures(VKNVGintlTextContext* ctx,
                               const VKNVGfeature* features,
                               uint32_t featureCount);

// Enable/disable BiDi processing
void vknvg__setBidiEnabled(VKNVGintlTextContext* ctx, bool enabled);

// Enable/disable complex script shaping
void vknvg__setComplexScriptsEnabled(VKNVGintlTextContext* ctx, bool enabled);

// Get statistics
void vknvg__getIntlTextStats(VKNVGintlTextContext* ctx,
                             uint32_t* paragraphs,
                             uint32_t* runs,
                             uint32_t* glyphs,
                             uint32_t* bidiReorderings);

// Reset statistics
void vknvg__resetIntlTextStats(VKNVGintlTextContext* ctx);

// Internal functions

// Segment text into runs (by script, direction, font)
static uint32_t vknvg__segmentText(VKNVGintlTextContext* ctx,
                                   const char* text,
                                   uint32_t textLen,
                                   const VKNVGbidiResult* bidiResult,
                                   int fontID,
                                   VKNVGtextRun* runs);

// Shape single run with HarfBuzz
static int vknvg__shapeRun(VKNVGintlTextContext* ctx,
                           const char* text,
                           VKNVGtextRun* run,
                           float fontSize);

// Reorder runs for visual display
static void vknvg__reorderRuns(VKNVGshapedParagraph* paragraph);

// Calculate run metrics
static void vknvg__calculateRunMetrics(VKNVGtextRun* run, float fontSize);

// Calculate paragraph metrics
static void vknvg__calculateParagraphMetrics(VKNVGshapedParagraph* paragraph);

// Script-specific processing

// Apply Arabic contextual forms (initial, medial, final, isolated)
static void vknvg__processArabicShaping(VKNVGtextRun* run);

// Apply Devanagari reordering rules
static void vknvg__processDevanagariReordering(VKNVGtextRun* run);

// Apply Thai vowel/tone mark positioning
static void vknvg__processThaiMarks(VKNVGtextRun* run);

// Detect script boundaries
static bool vknvg__isScriptBoundary(VKNVGscript s1, VKNVGscript s2);

// Helper: Convert font size to 26.6 fixed point
static inline int32_t vknvg__toFixed26_6(float value)
{
	return (int32_t)(value * 64.0f);
}

// Helper: Convert 26.6 fixed point to float
static inline float vknvg__fromFixed26_6(int32_t value)
{
	return (float)value / 64.0f;
}

// Helper: Check if script is RTL
static inline bool vknvg__isRTLScript(VKNVGscript script)
{
	return script == VKNVG_SCRIPT_ARABIC ||
	       script == VKNVG_SCRIPT_HEBREW;
}

// Helper: Check if script is complex
static inline bool vknvg__isComplexScript(VKNVGscript script)
{
	return script == VKNVG_SCRIPT_ARABIC ||
	       script == VKNVG_SCRIPT_DEVANAGARI ||
	       script == VKNVG_SCRIPT_BENGALI ||
	       script == VKNVG_SCRIPT_THAI ||
	       script == VKNVG_SCRIPT_KHMER ||
	       script == VKNVG_SCRIPT_MYANMAR ||
	       script == VKNVG_SCRIPT_SINHALA;
}

#endif // NANOVG_VK_INTL_TEXT_H
