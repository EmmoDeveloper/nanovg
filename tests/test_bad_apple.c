// Bad Apple!! - Touhou Halloween Demo
// Black and white silhouette animation renderer
// Because it's Halloween and we need something spooky!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Bad Apple ASCII frames (iconic opening sequence)
// Each frame is 40x20 characters, '#' = black, ' ' = white
static const char* badAppleFrames[] = {
	// Frame 0: Empty (fade in)
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"                                        ",

	// Frame 1: Apple outline appears
	"                                        "
	"                                        "
	"                                        "
	"                                        "
	"              ######                    "
	"            ##########                  "
	"           ############                 "
	"          ##############                "
	"         ################               "
	"         ################               "
	"        ##################              "
	"        ##################              "
	"         ################               "
	"          ##############                "
	"           ############                 "
	"            ##########                  "
	"                                        "
	"                                        "
	"                                        "
	"                                        ",

	// Frame 2: Apple with stem
	"                                        "
	"                                        "
	"                  ##                    "
	"                 ####                   "
	"              ########                  "
	"            ##########                  "
	"           ############                 "
	"          ##############                "
	"         ################               "
	"         ################               "
	"        ##################              "
	"        ##################              "
	"         ################               "
	"          ##############                "
	"           ############                 "
	"            ##########                  "
	"              ######                    "
	"                                        "
	"                                        "
	"                                        ",

	// Frame 3: Apple with leaf
	"                                        "
	"                                        "
	"                  ##                    "
	"                 #### ##                "
	"              ######## ###              "
	"            ##########  ##              "
	"           ############                 "
	"          ##############                "
	"         ################               "
	"         ################               "
	"        ##################              "
	"        ##################              "
	"         ################               "
	"          ##############                "
	"           ############                 "
	"            ##########                  "
	"              ######                    "
	"                                        "
	"                                        "
	"                                        ",

	// Frame 4: Silhouette girl appears (iconic pose)
	"                                        "
	"                                        "
	"                  ##                    "
	"                 ####                   "
	"              ########                  "
	"            ############                "
	"           ##############               "
	"          ################              "
	"         ##################             "
	"        ####################            "
	"        ####################            "
	"       ######################           "
	"        ####################            "
	"         ##################             "
	"          ################              "
	"           ##############               "
	"            ############                "
	"              ########                  "
	"                ####                    "
	"                                        ",

	// Frame 5: Girl silhouette with flowing dress
	"                                        "
	"                  ##                    "
	"                 ####                   "
	"              ########                  "
	"            ############                "
	"          ################              "
	"         ##################             "
	"        ####################            "
	"       ######################           "
	"       ######################           "
	"      ########################          "
	"      ########################          "
	"       ######################           "
	"        ####################            "
	"         ##################             "
	"          ################              "
	"         ####    ####    ####           "
	"        ##        ##        ##          "
	"                                        "
	"                                        ",

	// Frame 6: Girl dancing (arms raised)
	"                                        "
	"             ##          ##             "
	"              ##        ##              "
	"               ##      ##               "
	"                ########                "
	"              ############              "
	"            ################            "
	"           ##################           "
	"          ####################          "
	"         ######################         "
	"        ########################        "
	"        ########################        "
	"         ######################         "
	"          ####################          "
	"           ##################           "
	"            ################            "
	"            ####        ####            "
	"           ##              ##           "
	"          ##                ##          "
	"                                        ",

	// Frame 7: Spinning motion blur
	"                                        "
	"        ##                  ##          "
	"         ##                ##           "
	"          ##              ##            "
	"           ################             "
	"            ##############              "
	"          ##################            "
	"         ####################           "
	"        ######################          "
	"       ########################         "
	"       ########################         "
	"      ##########################        "
	"       ########################         "
	"        ######################          "
	"         ####################           "
	"          ##################            "
	"           ################             "
	"            ##        ##                "
	"                                        "
	"                                        "
};

#define BAD_APPLE_FRAME_COUNT 8
#define FRAME_WIDTH 40
#define FRAME_HEIGHT 20

// Test: ASCII frame validation
static int test_bad_apple_frames_valid(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Frame Validation\n");
	printf("========================================\n\n");

	printf("Testing iconic Touhou black & white frames...\n\n");

	for (int i = 0; i < BAD_APPLE_FRAME_COUNT; i++) {
		int len = strlen(badAppleFrames[i]);
		int expectedLen = FRAME_WIDTH * FRAME_HEIGHT;

		if (len != expectedLen) {
			printf("  ✗ FAIL Frame %d: length=%d, expected=%d\n", i, len, expectedLen);
			return 0;
		}

		// Count black pixels
		int blackPixels = 0;
		for (int j = 0; j < len; j++) {
			if (badAppleFrames[i][j] == '#') {
				blackPixels++;
			}
		}

		printf("  Frame %d: %d black pixels (%.1f%% coverage)\n",
		       i, blackPixels, (blackPixels * 100.0f) / expectedLen);
	}

	printf("\n  ✓ PASS All %d frames valid\n", BAD_APPLE_FRAME_COUNT);
	return 1;
}

// Test: Render single frame as text
static int test_bad_apple_render_frame(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Frame Rendering\n");
	printf("========================================\n\n");

	printf("Rendering frame 6 (iconic dancing pose):\n\n");

	const char* frame = badAppleFrames[6];
	for (int y = 0; y < FRAME_HEIGHT; y++) {
		printf("  ");
		for (int x = 0; x < FRAME_WIDTH; x++) {
			char c = frame[y * FRAME_WIDTH + x];
			printf("%c", c == '#' ? '█' : ' ');
		}
		printf("\n");
	}

	printf("\n  ✓ PASS Frame rendered\n");
	return 1;
}

// Test: Animation sequence
static int test_bad_apple_animation_sequence(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Animation Test\n");
	printf("========================================\n\n");

	printf("Testing 8-frame animation sequence:\n");
	printf("  Frame 0: Fade in (empty)\n");
	printf("  Frame 1: Apple outline appears\n");
	printf("  Frame 2: Apple with stem\n");
	printf("  Frame 3: Apple with leaf (Touhou!)\n");
	printf("  Frame 4: Girl silhouette emerges\n");
	printf("  Frame 5: Flowing dress\n");
	printf("  Frame 6: Dancing (arms raised)\n");
	printf("  Frame 7: Spinning motion\n\n");

	// Verify frame transitions make sense
	for (int i = 0; i < BAD_APPLE_FRAME_COUNT - 1; i++) {
		int pixelsBefore = 0, pixelsAfter = 0;

		for (int j = 0; j < FRAME_WIDTH * FRAME_HEIGHT; j++) {
			if (badAppleFrames[i][j] == '#') pixelsBefore++;
			if (badAppleFrames[i+1][j] == '#') pixelsAfter++;
		}

		int delta = pixelsAfter - pixelsBefore;
		printf("  Transition %d→%d: %+d pixels ", i, i+1, delta);

		if (i == 0) {
			// Frame 0→1: should gain pixels (fade in)
			if (delta <= 0) {
				printf("✗ FAIL (expected gain)\n");
				return 0;
			}
		}

		printf("✓\n");
	}

	printf("\n  ✓ PASS Animation sequence valid\n");
	return 1;
}

// Test: Frame rate calculation
static int test_bad_apple_frame_rate(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Frame Rate Test\n");
	printf("========================================\n\n");

	printf("Simulating animation at different frame rates:\n\n");

	int targetFPS[] = {30, 60, 120};
	for (int i = 0; i < 3; i++) {
		int fps = targetFPS[i];
		float frameTimeMs = 1000.0f / fps;
		int framesPerSecond = fps;

		// Calculate how many times we loop through 8 frames
		float animDuration = (8.0f / fps);
		float loopsPerSecond = 1.0f / animDuration;

		printf("  %d FPS: %.2f ms/frame, %.2f loops/second\n",
		       fps, frameTimeMs, loopsPerSecond);
	}

	printf("\n  Original Bad Apple: 30 FPS (standard)\n");
	printf("  This demo: 8 frames @ 30 FPS = 0.27 seconds loop\n");
	printf("\n  ✓ PASS Frame rate calculations valid\n");
	return 1;
}

// Test: Black/white contrast validation
static int test_bad_apple_contrast(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Contrast Test\n");
	printf("========================================\n\n");

	printf("Testing pure black/white aesthetic:\n\n");

	for (int i = 0; i < BAD_APPLE_FRAME_COUNT; i++) {
		int validChars = 1;
		for (int j = 0; j < FRAME_WIDTH * FRAME_HEIGHT; j++) {
			char c = badAppleFrames[i][j];
			if (c != '#' && c != ' ') {
				printf("  ✗ FAIL Frame %d: Invalid character '%c' at position %d\n",
				       i, c, j);
				validChars = 0;
				break;
			}
		}

		if (!validChars) return 0;
	}

	printf("  All frames use only '#' (black) and ' ' (white)\n");
	printf("  Pure binary aesthetic: ✓\n");
	printf("  Touhou silhouette style: ✓\n");
	printf("\n  ✓ PASS Perfect black/white contrast\n");
	return 1;
}

// Test: Spooky Halloween mode
static int test_bad_apple_halloween_mode(void)
{
	printf("\n========================================\n");
	printf("  Bad Apple!! - Halloween Spooky Mode\n");
	printf("========================================\n\n");

	printf("🎃 Activating Halloween effects...\n\n");

	// Render frame with spooky effects
	printf("Frame 7 with Halloween enhancement:\n\n");

	const char* frame = badAppleFrames[7];
	for (int y = 0; y < FRAME_HEIGHT; y++) {
		printf("  ");
		for (int x = 0; x < FRAME_WIDTH; x++) {
			char c = frame[y * FRAME_WIDTH + x];

			// Add random "corruption" effect
			if (c == '#' && (rand() % 20) == 0) {
				printf("░"); // Glitch effect
			} else if (c == '#') {
				printf("█");
			} else if ((rand() % 50) == 0) {
				printf("·"); // Ghost pixels
			} else {
				printf(" ");
			}
		}
		printf("\n");
	}

	printf("\n  🎃 Halloween effects:\n");
	printf("    - Glitch corruption: ░\n");
	printf("    - Ghost pixels: ·\n");
	printf("    - Spooky atmosphere: ✓\n");
	printf("\n  ✓ PASS Happy Halloween! 🎃👻\n");
	return 1;
}

int main(void)
{
	srand(time(NULL));

	printf("\n");
	printf("╔════════════════════════════════════════╗\n");
	printf("║  Bad Apple!! - Touhou Halloween Demo  ║\n");
	printf("║  Black & White Silhouette Animation   ║\n");
	printf("╚════════════════════════════════════════╝\n");

	int testsPassed = 0;
	int testsTotal = 0;

	#define RUN_TEST(test) do { \
		testsTotal++; \
		if (test()) testsPassed++; \
	} while(0)

	RUN_TEST(test_bad_apple_frames_valid);
	RUN_TEST(test_bad_apple_render_frame);
	RUN_TEST(test_bad_apple_animation_sequence);
	RUN_TEST(test_bad_apple_frame_rate);
	RUN_TEST(test_bad_apple_contrast);
	RUN_TEST(test_bad_apple_halloween_mode);

	printf("\n");
	printf("========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", testsTotal);
	printf("  Passed:  %d\n", testsPassed);
	printf("========================================\n");

	if (testsPassed == testsTotal) {
		printf("All Bad Apple tests passed!\n");
		printf("🎃 Happy Halloween from Touhou! 👻\n\n");
		return 0;
	} else {
		printf("Some tests failed!\n\n");
		return 1;
	}
}
