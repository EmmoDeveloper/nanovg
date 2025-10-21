// Test for Compute-based Glyph Rasterization
// Verifies compute raster data structures and outline handling

#include "test_framework.h"
#include "../src/nanovg_vk_compute_raster.h"
#include <stdio.h>
#include <string.h>

// Test: Glyph outline structures
TEST(glyph_outline_structure)
{
	VKNVGglyphOutline outline = {0};

	// Add a simple square contour
	outline.pointCount = 5;
	outline.contourCount = 1;

	outline.points[0].x = 0.0f;
	outline.points[0].y = 0.0f;
	outline.points[0].type = VKNVG_POINT_MOVE;

	outline.points[1].x = 100.0f;
	outline.points[1].y = 0.0f;
	outline.points[1].type = VKNVG_POINT_LINE;

	outline.points[2].x = 100.0f;
	outline.points[2].y = 100.0f;
	outline.points[2].type = VKNVG_POINT_LINE;

	outline.points[3].x = 0.0f;
	outline.points[3].y = 100.0f;
	outline.points[3].type = VKNVG_POINT_LINE;

	outline.points[4].x = 0.0f;
	outline.points[4].y = 0.0f;
	outline.points[4].type = VKNVG_POINT_LINE;

	outline.contours[0].pointOffset = 0;
	outline.contours[0].pointCount = 5;
	outline.contours[0].closed = 1;

	outline.boundingBox[0] = 0.0f;
	outline.boundingBox[1] = 0.0f;
	outline.boundingBox[2] = 100.0f;
	outline.boundingBox[3] = 100.0f;

	printf("    Square outline: %u points, %u contour\n",
	       outline.pointCount, outline.contourCount);
	printf("    Bounding box: (%.1f,%.1f) - (%.1f,%.1f)\n",
	       outline.boundingBox[0], outline.boundingBox[1],
	       outline.boundingBox[2], outline.boundingBox[3]);

	ASSERT_TRUE(outline.pointCount == 5);
	ASSERT_TRUE(outline.contourCount == 1);
	ASSERT_TRUE(outline.contours[0].closed == 1);
}

// Test: Point type enum
TEST(point_type_enum)
{
	printf("    MOVE=%d, LINE=%d, QUAD=%d, CUBIC=%d\n",
	       VKNVG_POINT_MOVE, VKNVG_POINT_LINE,
	       VKNVG_POINT_QUAD, VKNVG_POINT_CUBIC);

	ASSERT_TRUE(VKNVG_POINT_MOVE == 0);
	ASSERT_TRUE(VKNVG_POINT_LINE == 1);
	ASSERT_TRUE(VKNVG_POINT_QUAD == 2);
	ASSERT_TRUE(VKNVG_POINT_CUBIC == 3);
}

// Test: Rasterization parameters
TEST(rasterization_parameters)
{
	VKNVGcomputeRasterParams params = {0};

	params.width = 64;
	params.height = 64;
	params.scale = 1.0f;
	params.sdfRadius = 8.0f;
	params.generateSDF = 1;

	printf("    Raster params: %ux%u, scale=%.1f, SDF radius=%.1f\n",
	       params.width, params.height, params.scale, params.sdfRadius);

	ASSERT_TRUE(params.width == 64);
	ASSERT_TRUE(params.height == 64);
	ASSERT_TRUE(params.generateSDF == 1);
	ASSERT_TRUE(params.sdfRadius == 8.0f);
}

// Test: Bezier curve outline
TEST(bezier_curve_outline)
{
	VKNVGglyphOutline outline = {0};

	// Add a curve with quadratic Bézier
	outline.pointCount = 3;
	outline.contourCount = 1;

	outline.points[0].x = 0.0f;
	outline.points[0].y = 0.0f;
	outline.points[0].type = VKNVG_POINT_MOVE;

	outline.points[1].x = 50.0f;
	outline.points[1].y = 100.0f;
	outline.points[1].type = VKNVG_POINT_QUAD;  // Control point

	outline.points[2].x = 100.0f;
	outline.points[2].y = 0.0f;
	outline.points[2].type = VKNVG_POINT_LINE;

	outline.contours[0].pointOffset = 0;
	outline.contours[0].pointCount = 3;
	outline.contours[0].closed = 0;

	printf("    Quadratic Bézier: %u points\n", outline.pointCount);
	printf("    Control point at (%.1f, %.1f)\n",
	       outline.points[1].x, outline.points[1].y);

	ASSERT_TRUE(outline.pointCount == 3);
	ASSERT_TRUE(outline.points[1].type == VKNVG_POINT_QUAD);
	ASSERT_TRUE(outline.contours[0].closed == 0);
}

// Test: Maximum capacity
TEST(maximum_capacity)
{
	printf("    Max contours: %d\n", VKNVG_MAX_GLYPH_CONTOURS);
	printf("    Max points: %d\n", VKNVG_MAX_GLYPH_POINTS);
	printf("    Workgroup size: %dx%d threads\n",
	       VKNVG_COMPUTE_WORKGROUP_SIZE, VKNVG_COMPUTE_WORKGROUP_SIZE);

	ASSERT_TRUE(VKNVG_MAX_GLYPH_CONTOURS == 32);
	ASSERT_TRUE(VKNVG_MAX_GLYPH_POINTS == 512);
	ASSERT_TRUE(VKNVG_COMPUTE_WORKGROUP_SIZE == 8);
}

// Test: Complex glyph outline
TEST(complex_glyph_outline)
{
	VKNVGglyphOutline outline = {0};

	// Simulate a complex glyph with multiple contours (like 'B')
	outline.contourCount = 2;

	// First contour (outer)
	outline.contours[0].pointOffset = 0;
	outline.contours[0].pointCount = 4;
	outline.contours[0].closed = 1;

	// Second contour (inner hole)
	outline.contours[1].pointOffset = 4;
	outline.contours[1].pointCount = 4;
	outline.contours[1].closed = 1;

	outline.pointCount = 8;

	printf("    Complex glyph: %u contours, %u total points\n",
	       outline.contourCount, outline.pointCount);
	printf("    Contour 0: %u points (outer)\n", outline.contours[0].pointCount);
	printf("    Contour 1: %u points (hole)\n", outline.contours[1].pointCount);

	ASSERT_TRUE(outline.contourCount == 2);
	ASSERT_TRUE(outline.pointCount == 8);
	ASSERT_TRUE(outline.contours[0].pointOffset == 0);
	ASSERT_TRUE(outline.contours[1].pointOffset == 4);
}

// Test: SDF generation settings
TEST(sdf_generation_settings)
{
	VKNVGcomputeRasterParams standardRaster = {0};
	standardRaster.width = 32;
	standardRaster.height = 32;
	standardRaster.generateSDF = 0;
	standardRaster.sdfRadius = 0.0f;

	VKNVGcomputeRasterParams sdfRaster = {0};
	sdfRaster.width = 64;
	sdfRaster.height = 64;
	sdfRaster.generateSDF = 1;
	sdfRaster.sdfRadius = 16.0f;

	printf("    Standard raster: %ux%u, SDF=%u\n",
	       standardRaster.width, standardRaster.height,
	       standardRaster.generateSDF);
	printf("    SDF raster: %ux%u, SDF=%u, radius=%.1f\n",
	       sdfRaster.width, sdfRaster.height,
	       sdfRaster.generateSDF, sdfRaster.sdfRadius);

	ASSERT_TRUE(standardRaster.generateSDF == 0);
	ASSERT_TRUE(sdfRaster.generateSDF == 1);
	ASSERT_TRUE(sdfRaster.sdfRadius == 16.0f);
}

int main(void)
{
	printf("\n");
	printf("==========================================\n");
	printf("  Compute Rasterization Tests\n");
	printf("==========================================\n");
	printf("\n");

	RUN_TEST(test_glyph_outline_structure);
	RUN_TEST(test_point_type_enum);
	RUN_TEST(test_rasterization_parameters);
	RUN_TEST(test_bezier_curve_outline);
	RUN_TEST(test_maximum_capacity);
	RUN_TEST(test_complex_glyph_outline);
	RUN_TEST(test_sdf_generation_settings);

	print_test_summary();
	return test_exit_code();
}
