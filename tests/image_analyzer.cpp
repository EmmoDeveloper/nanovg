#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>

struct Point {
	int x;
	int y;
	unsigned char r, g, b;
};

class ImageAnalyzer {
private:
	int width;
	int height;
	unsigned char* data;

public:
	ImageAnalyzer(const char* ppm_file) : width(0), height(0), data(nullptr) {
		FILE* f = fopen(ppm_file, "rb");
		if (!f) {
			fprintf(stderr, "Failed to open %s\n", ppm_file);
			return;
		}

		char magic[3];
		if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
			fprintf(stderr, "Not a P6 PPM file\n");
			fclose(f);
			return;
		}

		// Skip whitespace and comments
		int c;
		while ((c = fgetc(f)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '#')) {
			if (c == '#') {
				while ((c = fgetc(f)) != EOF && c != '\n');
			}
		}
		ungetc(c, f);

		if (fscanf(f, "%d %d", &width, &height) != 2) {
			fprintf(stderr, "Failed to read dimensions\n");
			fclose(f);
			return;
		}

		int maxval;
		if (fscanf(f, "%d", &maxval) != 1) {
			fprintf(stderr, "Failed to read maxval\n");
			fclose(f);
			return;
		}

		// Skip single whitespace after maxval
		fgetc(f);

		data = (unsigned char*)malloc(width * height * 3);
		if (fread(data, 1, width * height * 3, f) != (size_t)(width * height * 3)) {
			fprintf(stderr, "Failed to read pixel data\n");
			free(data);
			data = nullptr;
		}

		fclose(f);
		printf("Loaded image: %dx%d\n", width, height);
	}

	~ImageAnalyzer() {
		if (data) free(data);
	}

	std::vector<Point> findNonBlackPixels(int threshold = 10) {
		std::vector<Point> points;

		if (!data) return points;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int idx = (y * width + x) * 3;
				unsigned char r = data[idx];
				unsigned char g = data[idx + 1];
				unsigned char b = data[idx + 2];

				// If any channel is above threshold, it's not black
				if (r > threshold || g > threshold || b > threshold) {
					Point p = {x, y, r, g, b};
					points.push_back(p);
				}
			}
		}

		return points;
	}

	void analyzeBoundingBox(const std::vector<Point>& points) {
		if (points.empty()) {
			printf("No non-black pixels found!\n");
			return;
		}

		int min_x = width, max_x = 0;
		int min_y = height, max_y = 0;

		for (const auto& p : points) {
			min_x = std::min(min_x, p.x);
			max_x = std::max(max_x, p.x);
			min_y = std::min(min_y, p.y);
			max_y = std::max(max_y, p.y);
		}

		printf("Bounding box:\n");
		printf("  X: %d to %d (width: %d)\n", min_x, max_x, max_x - min_x + 1);
		printf("  Y: %d to %d (height: %d)\n", min_y, max_y, max_y - min_y + 1);
		printf("  Total non-black pixels: %zu\n", points.size());
	}

	void printTextGrid(const std::vector<Point>& points, int cell_size = 4) {
		if (points.empty()) return;

		int min_x = width, max_x = 0;
		int min_y = height, max_y = 0;

		for (const auto& p : points) {
			min_x = std::min(min_x, p.x);
			max_x = std::max(max_x, p.x);
			min_y = std::min(min_y, p.y);
			max_y = std::max(max_y, p.y);
		}

		int grid_w = (max_x - min_x) / cell_size + 1;
		int grid_h = (max_y - min_y) / cell_size + 1;

		// Create grid
		std::vector<int> grid(grid_w * grid_h, 0);

		for (const auto& p : points) {
			int gx = (p.x - min_x) / cell_size;
			int gy = (p.y - min_y) / cell_size;
			grid[gy * grid_w + gx]++;
		}

		printf("\nText representation (cell_size=%d):\n", cell_size);
		for (int gy = 0; gy < grid_h; gy++) {
			for (int gx = 0; gx < grid_w; gx++) {
				int count = grid[gy * grid_w + gx];
				if (count > 0) {
					printf("█");
				} else {
					printf(" ");
				}
			}
			printf("\n");
		}
	}
};

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <ppm_file>\n", argv[0]);
		return 1;
	}

	ImageAnalyzer analyzer(argv[1]);
	auto points = analyzer.findNonBlackPixels(10);

	analyzer.analyzeBoundingBox(points);
	analyzer.printTextGrid(points, 4);

	return 0;
}
