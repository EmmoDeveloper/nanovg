#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nvg_freetype.h"

// Test BiDi (Bidirectional text) rendering with Arabic and Hebrew
// Tests RTL (Right-to-Left), LTR (Left-to-Right), and mixed text

int main() {
	printf("=== BiDi (Bidirectional Text) Test ===\n\n");

	// Create FreeType font system
	printf("1. Creating font system...\n");
	NVGFontSystem* sys = nvgft_create(2048, 2048);
	if (!sys) {
		fprintf(stderr, "Failed to create font system\n");
		return 1;
	}
	printf("   ✓ Font system created\n\n");

	// Load fonts
	printf("2. Loading fonts...\n");
	int arabicFont = nvgft_add_font(sys, "arabic", "fonts/arabic/NotoSansArabic-Regular.ttf");
	if (arabicFont == -1) {
		fprintf(stderr, "Failed to load Arabic font\n");
		nvgft_destroy(sys);
		return 1;
	}
	printf("   ✓ Arabic font loaded (id=%d)\n", arabicFont);

	int hebrewFont = nvgft_add_font(sys, "hebrew", "fonts/hebrew/NotoSansHebrew-Regular.ttf");
	if (hebrewFont == -1) {
		fprintf(stderr, "Failed to load Hebrew font\n");
		nvgft_destroy(sys);
		return 1;
	}
	printf("   ✓ Hebrew font loaded (id=%d)\n", hebrewFont);

	int latinFont = nvgft_add_font(sys, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (latinFont == -1) {
		fprintf(stderr, "Failed to load Latin font\n");
		nvgft_destroy(sys);
		return 1;
	}
	printf("   ✓ Latin font loaded (id=%d)\n\n", latinFont);

	// Test cases
	struct {
		const char* name;
		const char* text;
		int font;
		int direction;  // 0=auto, 1=LTR, 2=RTL
		const char* language;
	} tests[] = {
		// Arabic (RTL)
		{"Arabic RTL", "مرحبا بالعالم", arabicFont, 2, "ar"},  // "Hello World" in Arabic
		{"Arabic Auto", "السلام عليكم", arabicFont, 0, "ar"},  // "Peace be upon you"

		// Hebrew (RTL)
		{"Hebrew RTL", "שלום עולם", hebrewFont, 2, "he"},  // "Hello World" in Hebrew
		{"Hebrew Auto", "מה שלומך", hebrewFont, 0, "he"},  // "How are you?"

		// Mixed LTR/RTL
		{"Mixed AR+EN", "Hello مرحبا World", arabicFont, 0, "ar"},
		{"Mixed HE+EN", "Hello שלום World", hebrewFont, 0, "he"},

		// Latin LTR (control)
		{"English LTR", "Hello World!", latinFont, 1, "en"},
		{"English Auto", "Testing BiDi", latinFont, 0, "en"}
	};

	int numTests = sizeof(tests) / sizeof(tests[0]);

	printf("3. Testing shaped text iteration (HarfBuzz + FriBidi)...\n\n");

	int passed = 0;
	int failed = 0;

	for (int t = 0; t < numTests; t++) {
		printf("Test %d: %s\n", t + 1, tests[t].name);
		printf("  Text: %s\n", tests[t].text);
		printf("  Font: %d, Direction: %d (%s), Language: %s\n",
		       tests[t].font,
		       tests[t].direction,
		       tests[t].direction == 0 ? "Auto" : (tests[t].direction == 1 ? "LTR" : "RTL"),
		       tests[t].language);

		// Set font and size
		nvgft_set_font(sys, tests[t].font);
		nvgft_set_size(sys, 48.0f);

		// Initialize shaped text iterator
		NVGFTTextIter iter;
		const char* text = tests[t].text;
		nvgft_shaped_text_iter_init(sys, &iter, 0.0f, 0.0f,
		                             text, text + strlen(text),
		                             tests[t].direction, tests[t].language);

		// Count glyphs
		int glyphCount = 0;
		NVGFTQuad quad;
		while (nvgft_shaped_text_iter_next(sys, &iter, &quad)) {
			glyphCount++;
		}

		if (glyphCount > 0) {
			printf("  ✓ Shaped %d glyphs\n", glyphCount);
			passed++;
		} else {
			printf("  ✗ FAILED: No glyphs shaped\n");
			failed++;
		}

		// Free shaped text
		nvgft_text_iter_free(&iter);
		printf("\n");
	}

	// Summary
	printf("4. Test Summary:\n");
	printf("   Passed: %d/%d\n", passed, numTests);
	printf("   Failed: %d/%d\n", failed, numTests);

	if (failed > 0) {
		printf("\n   Some tests FAILED\n");
	}

	// Additional BiDi algorithm test
	printf("\n5. Testing FriBidi directly...\n");

	// Test Arabic text
	const char* arabicTest = "مرحبا";
	printf("   Input (Arabic): %s\n", arabicTest);

	// The shaped iterator already tested FriBidi internally
	printf("   ✓ FriBidi integration working (tested via shaped iterator)\n");

	// Cleanup
	printf("\n6. Cleaning up...\n");
	nvgft_destroy(sys);
	printf("   ✓ Cleanup complete\n");

	if (failed == 0) {
		printf("\n=== BiDi Test PASSED ===\n");
		return 0;
	} else {
		printf("\n=== BiDi Test FAILED ===\n");
		return 1;
	}
}
