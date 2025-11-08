#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.ppm>\n", argv[0]);
		return 1;
	}

	FILE* f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		return 1;
	}

	char magic[3];
	int w, h, maxval;
	fscanf(f, "%2s\n%d %d\n%d\n", magic, &w, &h, &maxval);

	if (strcmp(magic, "P6") != 0) {
		fprintf(stderr, "Not a P6 PPM file\n");
		fclose(f);
		return 1;
	}

	int total_pixels = w * h;
	int black_pixels = 0;
	int white_pixels = 0;
	int colored_pixels = 0;

	unsigned char rgb[3];
	for (int i = 0; i < total_pixels; i++) {
		if (fread(rgb, 1, 3, f) != 3) break;

		if (rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 0) {
			black_pixels++;
		} else if (rgb[0] == 255 && rgb[1] == 255 && rgb[2] == 255) {
			white_pixels++;
		} else {
			colored_pixels++;
		}
	}

	fclose(f);

	printf("%s: %dx%d\n", argv[1], w, h);
	printf("  Black pixels: %d (%.1f%%)\n", black_pixels, 100.0 * black_pixels / total_pixels);
	printf("  White pixels: %d (%.1f%%)\n", white_pixels, 100.0 * white_pixels / total_pixels);
	printf("  Colored pixels: %d (%.1f%%)\n", colored_pixels, 100.0 * colored_pixels / total_pixels);

	if (colored_pixels > total_pixels * 0.01) {
		printf("  Status: HAS CONTENT\n");
	} else if (black_pixels > total_pixels * 0.9) {
		printf("  Status: MOSTLY BLACK\n");
	} else {
		printf("  Status: MOSTLY BACKGROUND\n");
	}

	return 0;
}
