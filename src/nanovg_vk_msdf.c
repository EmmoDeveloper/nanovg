//
// Copyright (c) 2025 NanoVG MSDF Generator
//
// Simplified Multi-channel Signed Distance Field generation
// For production use, consider integrating the full msdfgen library
//

#include "nanovg_vk_msdf.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#define MSDF_MAX_CONTOURS 256
#define MSDF_MAX_POINTS 4096

typedef struct {
	float x, y;
} MSPoint;

typedef struct {
	MSPoint* points;
	int count;
	int capacity;
} MSContour;

typedef struct {
	MSContour contours[MSDF_MAX_CONTOURS];
	int contourCount;
	MSPoint allPoints[MSDF_MAX_POINTS];
	int pointCount;
} MSShape;

static float msdf_distance(MSPoint a, MSPoint b) {
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	return sqrtf(dx * dx + dy * dy);
}

static float msdf_cross(MSPoint a, MSPoint b) {
	return a.x * b.y - a.y * b.x;
}

// Compute signed distance from point p to line segment (a, b)
static float msdf_segmentDistance(MSPoint p, MSPoint a, MSPoint b, int* pseudo) {
	MSPoint ap = {p.x - a.x, p.y - a.y};
	MSPoint ab = {b.x - a.x, b.y - a.y};

	float h = (ap.x * ab.x + ap.y * ab.y) / (ab.x * ab.x + ab.y * ab.y);
	h = fmaxf(0.0f, fminf(1.0f, h));

	MSPoint closest = {a.x + ab.x * h, a.y + ab.y * h};
	float dist = msdf_distance(p, closest);

	// Determine sign based on cross product
	float cross = msdf_cross(ab, ap);
	*pseudo = (cross >= 0) ? 1 : -1;

	return dist;
}

// FreeType outline decomposition callbacks
static int msdf_moveTo(const FT_Vector* to, void* user) {
	MSShape* shape = (MSShape*)user;

	if (shape->contourCount >= MSDF_MAX_CONTOURS) return 1;

	MSContour* contour = &shape->contours[shape->contourCount];
	contour->points = &shape->allPoints[shape->pointCount];
	contour->count = 0;
	contour->capacity = MSDF_MAX_POINTS - shape->pointCount;

	shape->contourCount++;

	// Add first point
	if (shape->pointCount < MSDF_MAX_POINTS) {
		shape->allPoints[shape->pointCount].x = to->x / 64.0f;
		shape->allPoints[shape->pointCount].y = to->y / 64.0f;
		shape->pointCount++;
		contour->count++;
	}

	return 0;
}

static int msdf_lineTo(const FT_Vector* to, void* user) {
	MSShape* shape = (MSShape*)user;

	if (shape->contourCount == 0 || shape->pointCount >= MSDF_MAX_POINTS) return 1;

	MSContour* contour = &shape->contours[shape->contourCount - 1];
	shape->allPoints[shape->pointCount].x = to->x / 64.0f;
	shape->allPoints[shape->pointCount].y = to->y / 64.0f;
	shape->pointCount++;
	contour->count++;

	return 0;
}

static int msdf_conicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
	MSShape* shape = (MSShape*)user;

	if (shape->contourCount == 0 || shape->pointCount == 0) return 1;

	MSContour* contour = &shape->contours[shape->contourCount - 1];
	if (contour->count == 0) return 1;

	// Get start point (last point in contour)
	MSPoint p0 = contour->points[contour->count - 1];
	MSPoint p1 = {control->x / 64.0f, control->y / 64.0f};
	MSPoint p2 = {to->x / 64.0f, to->y / 64.0f};

	// Subdivide quadratic Bezier into line segments
	// P(t) = (1-t)²P0 + 2(1-t)tP1 + t²P2
	const int subdivisions = 8;
	for (int i = 1; i <= subdivisions; i++) {
		if (shape->pointCount >= MSDF_MAX_POINTS) return 1;

		float t = (float)i / (float)subdivisions;
		float t1 = 1.0f - t;
		float t1_sq = t1 * t1;
		float t_sq = t * t;

		MSPoint p;
		p.x = t1_sq * p0.x + 2.0f * t1 * t * p1.x + t_sq * p2.x;
		p.y = t1_sq * p0.y + 2.0f * t1 * t * p1.y + t_sq * p2.y;

		shape->allPoints[shape->pointCount] = p;
		shape->pointCount++;
		contour->count++;
	}

	return 0;
}

static int msdf_cubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
	MSShape* shape = (MSShape*)user;

	if (shape->contourCount == 0 || shape->pointCount == 0) return 1;

	MSContour* contour = &shape->contours[shape->contourCount - 1];
	if (contour->count == 0) return 1;

	// Get start point (last point in contour)
	MSPoint p0 = contour->points[contour->count - 1];
	MSPoint p1 = {control1->x / 64.0f, control1->y / 64.0f};
	MSPoint p2 = {control2->x / 64.0f, control2->y / 64.0f};
	MSPoint p3 = {to->x / 64.0f, to->y / 64.0f};

	// Subdivide cubic Bezier into line segments
	// P(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
	const int subdivisions = 12;
	for (int i = 1; i <= subdivisions; i++) {
		if (shape->pointCount >= MSDF_MAX_POINTS) return 1;

		float t = (float)i / (float)subdivisions;
		float t1 = 1.0f - t;
		float t1_sq = t1 * t1;
		float t1_cu = t1_sq * t1;
		float t_sq = t * t;
		float t_cu = t_sq * t;

		MSPoint p;
		p.x = t1_cu * p0.x + 3.0f * t1_sq * t * p1.x + 3.0f * t1 * t_sq * p2.x + t_cu * p3.x;
		p.y = t1_cu * p0.y + 3.0f * t1_sq * t * p1.y + 3.0f * t1 * t_sq * p2.y + t_cu * p3.y;

		shape->allPoints[shape->pointCount] = p;
		shape->pointCount++;
		contour->count++;
	}

	return 0;
}

int vknvg__generateMSDF(FT_GlyphSlot glyph, unsigned char* output, VKNVGmsdfParams* params) {
	if (!glyph || !output || !params) return 0;
	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE) return 0;

	MSShape shape;
	memset(&shape, 0, sizeof(shape));

	// Decompose outline
	FT_Outline_Funcs funcs;
	funcs.move_to = msdf_moveTo;
	funcs.line_to = msdf_lineTo;
	funcs.conic_to = msdf_conicTo;
	funcs.cubic_to = msdf_cubicTo;
	funcs.shift = 0;
	funcs.delta = 0;

	if (FT_Outline_Decompose(&glyph->outline, &funcs, &shape) != 0) {
		return 0;
	}

	if (shape.contourCount == 0) {
		// Empty glyph - fill with max distance
		memset(output, 0, params->width * params->height * 3);
		return 1;
	}

	// Generate MSDF
	float scale = params->scale;
	float range = params->range;

	for (int y = 0; y < params->height; y++) {
		for (int x = 0; x < params->width; x++) {
			MSPoint p = {
				(x - params->offsetX) / scale,
				(y - params->offsetY) / scale
			};

			float minDistR = FLT_MAX;
			float minDistG = FLT_MAX;
			float minDistB = FLT_MAX;
			int signR = 1, signG = 1, signB = 1;

			// Find distance to nearest edge for each channel
			for (int c = 0; c < shape.contourCount; c++) {
				MSContour* contour = &shape.contours[c];

				for (int i = 0; i < contour->count; i++) {
					int next = (i + 1) % contour->count;
					MSPoint a = contour->points[i];
					MSPoint b = contour->points[next];

					int pseudo;
					float dist = msdf_segmentDistance(p, a, b, &pseudo);

					// Assign to different channels based on edge index
					// This creates the multi-channel effect
					int channel = i % 3;
					if (channel == 0 && dist < minDistR) {
						minDistR = dist;
						signR = pseudo;
					} else if (channel == 1 && dist < minDistG) {
						minDistG = dist;
						signG = pseudo;
					} else if (channel == 2 && dist < minDistB) {
						minDistB = dist;
						signB = pseudo;
					}
				}
			}

			// Convert distances to 0-255 range
			// Distance 0 = 128, range extends from 128-range*scale to 128+range*scale
			float distR = (signR * minDistR * scale) / range;
			float distG = (signG * minDistG * scale) / range;
			float distB = (signB * minDistB * scale) / range;

			int idx = (y * params->width + x) * 3;
			output[idx + 0] = (unsigned char)fmaxf(0.0f, fminf(255.0f, (distR + 1.0f) * 127.5f));
			output[idx + 1] = (unsigned char)fmaxf(0.0f, fminf(255.0f, (distG + 1.0f) * 127.5f));
			output[idx + 2] = (unsigned char)fmaxf(0.0f, fminf(255.0f, (distB + 1.0f) * 127.5f));
		}
	}

	return 1;
}

int vknvg__generateSDF(FT_GlyphSlot glyph, unsigned char* output, VKNVGmsdfParams* params) {
	if (!glyph || !output || !params) return 0;
	if (glyph->format != FT_GLYPH_FORMAT_OUTLINE) return 0;

	MSShape shape;
	memset(&shape, 0, sizeof(shape));

	// Decompose outline
	FT_Outline_Funcs funcs;
	funcs.move_to = msdf_moveTo;
	funcs.line_to = msdf_lineTo;
	funcs.conic_to = msdf_conicTo;
	funcs.cubic_to = msdf_cubicTo;
	funcs.shift = 0;
	funcs.delta = 0;

	if (FT_Outline_Decompose(&glyph->outline, &funcs, &shape) != 0) {
		return 0;
	}

	if (shape.contourCount == 0) {
		// Empty glyph
		memset(output, 0, params->width * params->height);
		return 1;
	}

	// Generate SDF
	float scale = params->scale;
	float range = params->range;

	for (int y = 0; y < params->height; y++) {
		for (int x = 0; x < params->width; x++) {
			MSPoint p = {
				(x - params->offsetX) / scale,
				(y - params->offsetY) / scale
			};

			float minDist = FLT_MAX;
			int sign = 1;

			// Find distance to nearest edge
			for (int c = 0; c < shape.contourCount; c++) {
				MSContour* contour = &shape.contours[c];

				for (int i = 0; i < contour->count; i++) {
					int next = (i + 1) % contour->count;
					MSPoint a = contour->points[i];
					MSPoint b = contour->points[next];

					int pseudo;
					float dist = msdf_segmentDistance(p, a, b, &pseudo);

					if (dist < minDist) {
						minDist = dist;
						sign = pseudo;
					}
				}
			}

			// Convert distance to 0-255 range
			float signedDist = (sign * minDist * scale) / range;
			output[y * params->width + x] = (unsigned char)fmaxf(0.0f, fminf(255.0f, (signedDist + 1.0f) * 127.5f));
		}
	}

	return 1;
}
