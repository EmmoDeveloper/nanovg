// nanovg_vk_harfbuzz.h - HarfBuzz Integration for Complex Text Shaping
//
// Provides HarfBuzz text shaping for NanoVG to support:
// - Complex scripts (Arabic, Devanagari, Thai, Khmer, etc.)
// - OpenType features (ligatures, kerning, contextual alternates)
// - Proper glyph positioning and substitution
// - Multi-language text shaping
//
// Architecture:
// - HarfBuzz buffer management for text input
// - Font face creation from FreeType fonts
// - Shaped glyph output with positions and advances
// - Integration with virtual atlas glyph cache
// - Support for script-specific features

#ifndef NANOVG_VK_HARFBUZZ_H
#define NANOVG_VK_HARFBUZZ_H

#include <stdint.h>
#include <stdbool.h>

// HarfBuzz forward declarations (to avoid requiring HarfBuzz headers in all files)
typedef struct hb_buffer_t hb_buffer_t;
typedef struct hb_font_t hb_font_t;
typedef struct hb_face_t hb_face_t;

// FreeType forward declaration
typedef struct FT_FaceRec_* FT_Face;

// Configuration
#define VKNVG_MAX_SHAPED_GLYPHS 1024	// Max glyphs per shaped text run
#define VKNVG_MAX_FEATURES 32			// Max OpenType features per shaping call

// Text direction
typedef enum VKNVGtextDirection {
	VKNVG_TEXT_DIRECTION_LTR = 0,		// Left-to-right (Latin, Cyrillic, etc.)
	VKNVG_TEXT_DIRECTION_RTL = 1,		// Right-to-left (Arabic, Hebrew)
	VKNVG_TEXT_DIRECTION_TTB = 2,		// Top-to-bottom (Mongolian)
	VKNVG_TEXT_DIRECTION_BTT = 3,		// Bottom-to-top (rare)
	VKNVG_TEXT_DIRECTION_AUTO = 4,		// Auto-detect from script
} VKNVGtextDirection;

// Script identifier (ISO 15924 script codes)
typedef enum VKNVGscript {
	VKNVG_SCRIPT_INVALID = 0,
	VKNVG_SCRIPT_LATIN = 1,				// Latin (latn)
	VKNVG_SCRIPT_ARABIC = 2,			// Arabic (arab)
	VKNVG_SCRIPT_HEBREW = 3,			// Hebrew (hebr)
	VKNVG_SCRIPT_DEVANAGARI = 4,		// Devanagari (deva)
	VKNVG_SCRIPT_BENGALI = 5,			// Bengali (beng)
	VKNVG_SCRIPT_THAI = 6,				// Thai (thai)
	VKNVG_SCRIPT_KHMER = 7,				// Khmer (khmr)
	VKNVG_SCRIPT_HANGUL = 8,			// Hangul (hang)
	VKNVG_SCRIPT_HIRAGANA = 9,			// Hiragana (hira)
	VKNVG_SCRIPT_KATAKANA = 10,			// Katakana (kana)
	VKNVG_SCRIPT_HAN = 11,				// Han/CJK (hani)
	VKNVG_SCRIPT_MYANMAR = 12,			// Myanmar (mymr)
	VKNVG_SCRIPT_SINHALA = 13,			// Sinhala (sinh)
	VKNVG_SCRIPT_AUTO = 14,				// Auto-detect from text
} VKNVGscript;

// Language tag (BCP 47 language codes)
typedef struct VKNVGlanguage {
	char tag[8];	// e.g., "en", "ar", "th", "hi"
} VKNVGlanguage;

// OpenType feature (4-character tag + value)
typedef struct VKNVGfeature {
	char tag[4];	// e.g., "liga", "kern", "dlig"
	uint32_t value;	// 0 = off, 1 = on, or feature-specific value
	uint32_t start;	// Start character index (or 0 for all)
	uint32_t end;	// End character index (or -1 for all)
} VKNVGfeature;

// Shaped glyph output
typedef struct VKNVGshapedGlyph {
	uint32_t glyphIndex;	// Glyph ID in font
	uint32_t cluster;		// Character cluster index (for cursor positioning)
	int32_t xOffset;		// Horizontal offset (26.6 fixed point)
	int32_t yOffset;		// Vertical offset (26.6 fixed point)
	int32_t xAdvance;		// Horizontal advance (26.6 fixed point)
	int32_t yAdvance;		// Vertical advance (26.6 fixed point)
} VKNVGshapedGlyph;

// Shaping result
typedef struct VKNVGshapingResult {
	VKNVGshapedGlyph glyphs[VKNVG_MAX_SHAPED_GLYPHS];
	uint32_t glyphCount;
	VKNVGtextDirection direction;
	VKNVGscript script;
	uint32_t flags;			// Result flags (reversed, etc.)
} VKNVGshapingResult;

// HarfBuzz context (manages HarfBuzz state)
typedef struct VKNVGharfbuzzContext {
	hb_buffer_t* buffer;	// HarfBuzz buffer for text input
	hb_font_t** fonts;		// Array of HarfBuzz fonts (one per NanoVG font)
	uint32_t fontCount;		// Number of loaded fonts
	uint32_t fontCapacity;	// Font array capacity

	// Font mapping (NanoVG font ID → HarfBuzz font)
	struct {
		int nvgFontID;		// NanoVG font ID
		hb_font_t* hbFont;	// Corresponding HarfBuzz font
		FT_Face ftFace;		// FreeType face (for rasterization)
	} fontMap[256];			// Support up to 256 fonts
	uint32_t fontMapCount;

	// Statistics
	uint32_t shapingCalls;
	uint32_t totalGlyphsShaped;
	uint32_t complexScriptRuns;
} VKNVGharfbuzzContext;

// API Functions

// Create HarfBuzz context
VKNVGharfbuzzContext* vknvg__createHarfBuzzContext(void);

// Destroy HarfBuzz context
void vknvg__destroyHarfBuzzContext(VKNVGharfbuzzContext* ctx);

// Register font with HarfBuzz (call when font is loaded)
// Returns 0 on success, -1 on error
int vknvg__registerHarfBuzzFont(VKNVGharfbuzzContext* ctx,
                                int nvgFontID,
                                FT_Face ftFace);

// Unregister font (call when font is deleted)
void vknvg__unregisterHarfBuzzFont(VKNVGharfbuzzContext* ctx, int nvgFontID);

// Shape text using HarfBuzz
// text: UTF-8 encoded text
// textLen: Length in bytes
// fontID: NanoVG font ID
// direction: Text direction (or AUTO to detect)
// script: Script identifier (or AUTO to detect)
// language: Language tag (or NULL for default)
// features: OpenType features to enable (or NULL)
// featureCount: Number of features
// result: Output shaped glyphs
// Returns 1 on success, 0 on error
int vknvg__shapeText(VKNVGharfbuzzContext* ctx,
                     const char* text,
                     uint32_t textLen,
                     int fontID,
                     VKNVGtextDirection direction,
                     VKNVGscript script,
                     const VKNVGlanguage* language,
                     const VKNVGfeature* features,
                     uint32_t featureCount,
                     VKNVGshapingResult* result);

// Auto-detect script from UTF-8 text
VKNVGscript vknvg__detectScript(const char* text, uint32_t textLen);

// Get default text direction for script
VKNVGtextDirection vknvg__getScriptDirection(VKNVGscript script);

// Get statistics
void vknvg__getHarfBuzzStats(VKNVGharfbuzzContext* ctx,
                             uint32_t* shapingCalls,
                             uint32_t* totalGlyphs,
                             uint32_t* complexRuns);

// Reset statistics
void vknvg__resetHarfBuzzStats(VKNVGharfbuzzContext* ctx);

// Internal helper functions

// Convert HarfBuzz direction to VKNVGtextDirection
static inline VKNVGtextDirection vknvg__hbDirectionToVKNVG(int hbDirection);

// Convert VKNVGscript to HarfBuzz script
static inline uint32_t vknvg__scriptToHarfBuzz(VKNVGscript script);

// Analyze text for script and direction
static void vknvg__analyzeText(const char* text, uint32_t textLen,
                               VKNVGscript* outScript,
                               VKNVGtextDirection* outDirection);

// Common OpenType feature tags
#define VKNVG_FEATURE_LIGA "liga"	// Standard ligatures (fi, fl)
#define VKNVG_FEATURE_DLIG "dlig"	// Discretionary ligatures
#define VKNVG_FEATURE_KERN "kern"	// Kerning
#define VKNVG_FEATURE_CLIG "clig"	// Contextual ligatures
#define VKNVG_FEATURE_CALT "calt"	// Contextual alternates
#define VKNVG_FEATURE_SMCP "smcp"	// Small capitals
#define VKNVG_FEATURE_C2SC "c2sc"	// Capitals to small capitals

// Arabic-specific features
#define VKNVG_FEATURE_INIT "init"	// Initial forms
#define VKNVG_FEATURE_MEDI "medi"	// Medial forms
#define VKNVG_FEATURE_FINA "fina"	// Final forms
#define VKNVG_FEATURE_ISOL "isol"	// Isolated forms
#define VKNVG_FEATURE_RLIG "rlig"	// Required ligatures

// Devanagari-specific features
#define VKNVG_FEATURE_NUKT "nukt"	// Nukta forms
#define VKNVG_FEATURE_AKHN "akhn"	// Akhands
#define VKNVG_FEATURE_RPHF "rphf"	// Reph forms
#define VKNVG_FEATURE_BLWF "blwf"	// Below-base forms
#define VKNVG_FEATURE_HALF "half"	// Half forms
#define VKNVG_FEATURE_PSTF "pstf"	// Post-base forms
#define VKNVG_FEATURE_VATU "vatu"	// Vattu variants

// Helper macros for creating features
#define VKNVG_MAKE_FEATURE(t, v) \
	{ {t[0], t[1], t[2], t[3]}, (v), 0, (uint32_t)-1 }

#define VKNVG_MAKE_FEATURE_RANGE(t, v, s, e) \
	{ {t[0], t[1], t[2], t[3]}, (v), (s), (e) }

#endif // NANOVG_VK_HARFBUZZ_H
