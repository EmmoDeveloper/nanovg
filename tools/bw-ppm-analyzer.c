#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file.ppm>\n", argv[0]);
		return 1;
	}

	FILE* f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		return 1;
	}

	// Read PPM header
	char magic[3];
	int width, height, maxval;
	if (fscanf(f, "%2s\n%d %d\n%d\n", magic, &width, &height, &maxval) != 4) {
		fprintf(stderr, "Invalid PPM header\n");
		fclose(f);
		return 1;
	}

	if (strcmp(magic, "P6") != 0) {
		fprintf(stderr, "Only P6 PPM format supported\n");
		fclose(f);
		return 1;
	}

	printf("=== Black/White PPM Analyzer ===\n");
	printf("Image: %dx%d, maxval=%d\n\n", width, height, maxval);

	// Read pixel data
	unsigned char* pixels = malloc(width * height * 3);
	if (!pixels) {
		fprintf(stderr, "Failed to allocate memory\n");
		fclose(f);
		return 1;
	}

	if (fread(pixels, 3, width * height, f) != (size_t)(width * height)) {
		fprintf(stderr, "Failed to read pixel data\n");
		free(pixels);
		fclose(f);
		return 1;
	}
	fclose(f);

	// Analyze as pure black/white
	int black_pixels = 0;
	int white_pixels = 0;
	int min_x = width, max_x = -1;
	int min_y = height, max_y = -1;

	// Treat as binary: if ANY channel is non-zero, it's white
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			unsigned char r = pixels[idx];
			unsigned char g = pixels[idx + 1];
			unsigned char b = pixels[idx + 2];

			// Binary classification
			if (r == 0 && g == 0 && b == 0) {
				black_pixels++;
			} else {
				white_pixels++;
				// Track bounding box of white pixels
				if (x < min_x) min_x = x;
				if (x > max_x) max_x = x;
				if (y < min_y) min_y = y;
				if (y > max_y) max_y = y;
			}
		}
	}

	printf("Black pixels: %d (%.2f%%)\n", black_pixels,
		100.0 * black_pixels / (width * height));
	printf("White pixels: %d (%.2f%%)\n", white_pixels,
		100.0 * white_pixels / (width * height));

	if (white_pixels > 0) {
		printf("\nWhite pixel bounding box:\n");
		printf("  X: %d to %d (width: %d)\n", min_x, max_x, max_x - min_x + 1);
		printf("  Y: %d to %d (height: %d)\n", min_y, max_y, max_y - min_y + 1);
		printf("  Center: (%.1f, %.1f)\n",
			(min_x + max_x) / 2.0, (min_y + max_y) / 2.0);
	}

	free(pixels);
	return 0;
}
