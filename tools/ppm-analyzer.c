#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int x, y;
	unsigned char r, g, b;
} Pixel;

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <ppm_file>\n", argv[0]);
		return 1;
	}

	FILE* f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		return 1;
	}

	// Read P6 header
	char magic[3];
	if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
		fprintf(stderr, "Not a P6 PPM file\n");
		fclose(f);
		return 1;
	}

	// Skip whitespace and comments
	int c;
	while ((c = fgetc(f)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '#')) {
		if (c == '#') {
			while ((c = fgetc(f)) != EOF && c != '\n');
		}
	}
	ungetc(c, f);

	int width, height, maxval;
	if (fscanf(f, "%d %d", &width, &height) != 2) {
		fprintf(stderr, "Failed to read dimensions\n");
		fclose(f);
		return 1;
	}

	if (fscanf(f, "%d", &maxval) != 1) {
		fprintf(stderr, "Failed to read maxval\n");
		fclose(f);
		return 1;
	}

	fgetc(f); // Skip single whitespace after maxval

	printf("PPM: %dx%d, maxval=%d\n", width, height, maxval);

	// Read pixel data
	size_t dataSize = width * height * 3;
	unsigned char* data = malloc(dataSize);
	if (fread(data, 1, dataSize, f) != dataSize) {
		fprintf(stderr, "Failed to read pixel data\n");
		free(data);
		fclose(f);
		return 1;
	}
	fclose(f);

	// Analyze color distribution
	int total_pixels = width * height;
	int black_pixels = 0;
	int gray_pixels = 0;
	int white_pixels = 0;
	int colored_pixels = 0;

	for (int i = 0; i < total_pixels; i++) {
		unsigned char r = data[i * 3];
		unsigned char g = data[i * 3 + 1];
		unsigned char b = data[i * 3 + 2];

		int max_channel = r > g ? (r > b ? r : b) : (g > b ? g : b);
		int min_channel = r < g ? (r < b ? r : b) : (g < b ? g : b);

		if (max_channel < 10) {
			black_pixels++;
		} else if (max_channel > 245 && min_channel > 245) {
			white_pixels++;
		} else if (max_channel - min_channel < 30) {
			gray_pixels++;
		} else {
			colored_pixels++;
		}
	}

	printf("\nColor distribution:\n");
	printf("  Black pixels: %d (%.1f%%)\n", black_pixels, 100.0 * black_pixels / total_pixels);
	printf("  White pixels: %d (%.1f%%)\n", white_pixels, 100.0 * white_pixels / total_pixels);
	printf("  Gray pixels: %d (%.1f%%)\n", gray_pixels, 100.0 * gray_pixels / total_pixels);
	printf("  Colored pixels: %d (%.1f%%)\n", colored_pixels, 100.0 * colored_pixels / total_pixels);

	// Find non-black pixels
	int min_x = width, max_x = 0;
	int min_y = height, max_y = 0;
	int count = 0;
	int threshold = 10;

	// Track y-distribution
	int* rows_with_content = calloc(height, sizeof(int));

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			unsigned char r = data[idx];
			unsigned char g = data[idx + 1];
			unsigned char b = data[idx + 2];

			if (r > threshold || g > threshold || b > threshold) {
				if (x < min_x) min_x = x;
				if (x > max_x) max_x = x;
				if (y < min_y) min_y = y;
				if (y > max_y) max_y = y;
				count++;
				rows_with_content[y]++;
			}
		}
	}

	// Analyze vertical distribution
	int rows_with_pixels = 0;
	int first_row = -1, last_row = -1;
	for (int y = 0; y < height; y++) {
		if (rows_with_content[y] > 0) {
			rows_with_pixels++;
			if (first_row == -1) first_row = y;
			last_row = y;
		}
	}

	printf("\nVertical distribution:\n");
	printf("  First row with content: %d (%.1f%% from top)\n", first_row, 100.0 * first_row / height);
	printf("  Last row with content: %d (%.1f%% from top)\n", last_row, 100.0 * last_row / height);
	printf("  Total rows with pixels: %d/%d\n", rows_with_pixels, height);
	printf("  Content height: %d pixels\n", last_row - first_row + 1);

	// Sample a few rows to show distribution
	printf("\nSample row density (pixels per row):\n");
	for (int y = first_row; y <= last_row && y < first_row + 20; y++) {
		if (rows_with_content[y] > 0) {
			printf("  Row %4d: %4d pixels\n", y, rows_with_content[y]);
		}
	}

	if (count == 0) {
		printf("No non-black pixels found!\n");
		free(data);
		free(rows_with_content);
		return 0;
	}

	printf("\nBounding box:\n");
	printf("  X: %d to %d (width: %d)\n", min_x, max_x, max_x - min_x + 1);
	printf("  Y: %d to %d (height: %d)\n", min_y, max_y, max_y - min_y + 1);
	printf("  Total non-black pixels: %d\n", count);

	// Create text grid
	int cell_size = 4;
	int grid_w = (max_x - min_x) / cell_size + 1;
	int grid_h = (max_y - min_y) / cell_size + 1;

	int* grid = calloc(grid_w * grid_h, sizeof(int));

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			unsigned char r = data[idx];
			unsigned char g = data[idx + 1];
			unsigned char b = data[idx + 2];

			if (r > threshold || g > threshold || b > threshold) {
				int gx = (x - min_x) / cell_size;
				int gy = (y - min_y) / cell_size;
				grid[gy * grid_w + gx]++;
			}
		}
	}

	printf("\nText representation (cell_size=%d):\n", cell_size);
	for (int gy = 0; gy < grid_h; gy++) {
		for (int gx = 0; gx < grid_w; gx++) {
			int cnt = grid[gy * grid_w + gx];
			printf("%s", cnt > 0 ? "█" : " ");
		}
		printf("\n");
	}

	free(grid);
	free(data);
	free(rows_with_content);
	return 0;
}
