#include "vknvg_msdf.h"
#include <math.h>
#include <float.h>
#include <string.h>

// Vector2 for 2D calculations
typedef struct {
	float x, y;
} Vec2;

// Edge segment (line or curve)
typedef struct {
	Vec2 p0, p1, p2;  // Points (p2 used for quadratic curves)
	int type;         // 1=line, 2=quadratic
	Vec2 direction;   // Edge direction for MSDF channel selection
} Edge;

// Contour (collection of edges)
typedef struct {
	Edge* edges;
	int edgeCount;
	int edgeCapacity;
} Contour;

static float vec2_dot(Vec2 a, Vec2 b) {
	return a.x * b.x + a.y * b.y;
}

static float vec2_length(Vec2 v) {
	return sqrtf(v.x * v.x + v.y * v.y);
}

static Vec2 vec2_normalize(Vec2 v) {
	float len = vec2_length(v);
	if (len > 0.0f) {
		v.x /= len;
		v.y /= len;
	}
	return v;
}

static Vec2 vec2_sub(Vec2 a, Vec2 b) {
	Vec2 r = {a.x - b.x, a.y - b.y};
	return r;
}

// Calculate signed distance from point to line segment
static float distanceToLine(Vec2 p, Vec2 a, Vec2 b, float* t) {
	Vec2 ap = vec2_sub(p, a);
	Vec2 ab = vec2_sub(b, a);
	float abLen2 = vec2_dot(ab, ab);

	if (abLen2 < 1e-10f) {
		*t = 0.0f;
		return vec2_length(ap);
	}

	*t = vec2_dot(ap, ab) / abLen2;
	*t = fmaxf(0.0f, fminf(1.0f, *t));

	Vec2 closest = {a.x + ab.x * (*t), a.y + ab.y * (*t)};
	return vec2_length(vec2_sub(p, closest));
}

// Calculate signed distance from point to quadratic Bezier curve
static float distanceToQuadratic(Vec2 p, Vec2 p0, Vec2 p1, Vec2 p2, float* t) {
	// Sample curve more densely for better accuracy
	float minDist = FLT_MAX;
	float bestT = 0.0f;

	// Use 32 samples for better quality
	for (int i = 0; i <= 32; i++) {
		float tt = i / 32.0f;
		float s = 1.0f - tt;
		Vec2 point = {
			s * s * p0.x + 2.0f * s * tt * p1.x + tt * tt * p2.x,
			s * s * p0.y + 2.0f * s * tt * p1.y + tt * tt * p2.y
		};
		float dist = vec2_length(vec2_sub(p, point));
		if (dist < minDist) {
			minDist = dist;
			bestT = tt;
		}
	}

	*t = bestT;
	return minDist;
}

// FreeType outline decomposition callbacks
typedef struct {
	Contour* contours;
	int contourCount;
	int contourCapacity;
	Vec2 currentPoint;
	Vec2 firstPoint;
} OutlineData;

static int moveToFunc(const FT_Vector* to, void* user) {
	OutlineData* data = (OutlineData*)user;
	data->currentPoint.x = to->x / 64.0f;
	data->currentPoint.y = to->y / 64.0f;
	data->firstPoint = data->currentPoint;

	// Start new contour
	if (data->contourCount >= data->contourCapacity) {
		data->contourCapacity = data->contourCapacity ? data->contourCapacity * 2 : 8;
		data->contours = (Contour*)realloc(data->contours, data->contourCapacity * sizeof(Contour));
	}
	data->contours[data->contourCount].edges = NULL;
	data->contours[data->contourCount].edgeCount = 0;
	data->contours[data->contourCount].edgeCapacity = 0;
	data->contourCount++;

	return 0;
}

static int lineToFunc(const FT_Vector* to, void* user) {
	OutlineData* data = (OutlineData*)user;
	Contour* contour = &data->contours[data->contourCount - 1];

	// Add line edge
	if (contour->edgeCount >= contour->edgeCapacity) {
		contour->edgeCapacity = contour->edgeCapacity ? contour->edgeCapacity * 2 : 16;
		contour->edges = (Edge*)realloc(contour->edges, contour->edgeCapacity * sizeof(Edge));
	}

	Edge* edge = &contour->edges[contour->edgeCount++];
	edge->p0 = data->currentPoint;
	edge->p1.x = to->x / 64.0f;
	edge->p1.y = to->y / 64.0f;
	edge->type = 1;
	edge->direction = vec2_normalize(vec2_sub(edge->p1, edge->p0));

	data->currentPoint = edge->p1;
	return 0;
}

static int conicToFunc(const FT_Vector* control, const FT_Vector* to, void* user) {
	OutlineData* data = (OutlineData*)user;
	Contour* contour = &data->contours[data->contourCount - 1];

	// Add quadratic edge
	if (contour->edgeCount >= contour->edgeCapacity) {
		contour->edgeCapacity = contour->edgeCapacity ? contour->edgeCapacity * 2 : 16;
		contour->edges = (Edge*)realloc(contour->edges, contour->edgeCapacity * sizeof(Edge));
	}

	Edge* edge = &contour->edges[contour->edgeCount++];
	edge->p0 = data->currentPoint;
	edge->p1.x = control->x / 64.0f;
	edge->p1.y = control->y / 64.0f;
	edge->p2.x = to->x / 64.0f;
	edge->p2.y = to->y / 64.0f;
	edge->type = 2;
	edge->direction = vec2_normalize(vec2_sub(edge->p2, edge->p0));

	data->currentPoint = edge->p2;
	return 0;
}

static int cubicToFunc(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
	// Simplified: convert cubic to quadratic
	OutlineData* data = (OutlineData*)user;
	Vec2 ctrl;
	ctrl.x = (control1->x + control2->x) / 128.0f;
	ctrl.y = (control1->y + control2->y) / 128.0f;

	FT_Vector ctrlVec = {(FT_Pos)(ctrl.x * 64.0f), (FT_Pos)(ctrl.y * 64.0f)};
	conicToFunc(&ctrlVec, to, user);
	return 0;
}

void vknvg__generateSDF(FT_GlyphSlot glyph, unsigned char* output, const VKNVGmsdfParams* params) {
	if (!glyph || !output || !params || glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
		memset(output, 128, params->width * params->height);
		return;
	}

	// Decompose outline into contours and edges
	OutlineData data = {0};
	FT_Outline_Funcs funcs = {moveToFunc, lineToFunc, conicToFunc, cubicToFunc, 0, 0};
	FT_Outline_Decompose(&glyph->outline, &funcs, &data);

	// Close last contour
	if (data.contourCount > 0) {
		FT_Vector lastPoint = {(FT_Pos)(data.firstPoint.x * 64.0f), (FT_Pos)(data.firstPoint.y * 64.0f)};
		lineToFunc(&lastPoint, &data);
	}

	// Calculate bounding box
	FT_BBox bbox;
	FT_Outline_Get_CBox(&glyph->outline, &bbox);
	float xMin = bbox.xMin / 64.0f;
	float yMin = bbox.yMin / 64.0f;
	float xMax = bbox.xMax / 64.0f;
	float yMax = bbox.yMax / 64.0f;

	// Generate SDF
	for (int y = 0; y < params->height; y++) {
		for (int x = 0; x < params->width; x++) {
			Vec2 p;
			p.x = xMin + (x - params->offsetX) * (xMax - xMin) / (params->width - 2 * params->offsetX);
			p.y = yMin + (y - params->offsetY) * (yMax - yMin) / (params->height - 2 * params->offsetY);

			float minDist = FLT_MAX;

			// Find minimum distance to all edges
			for (int c = 0; c < data.contourCount; c++) {
				Contour* contour = &data.contours[c];
				for (int e = 0; e < contour->edgeCount; e++) {
					Edge* edge = &contour->edges[e];
					float t;
					float dist;

					if (edge->type == 1) {
						dist = distanceToLine(p, edge->p0, edge->p1, &t);
					} else {
						dist = distanceToQuadratic(p, edge->p0, edge->p1, edge->p2, &t);
					}

					if (dist < minDist) {
						minDist = dist;
					}
				}
			}

			// Determine if point is inside or outside (winding number test)
			int winding = 0;
			for (int c = 0; c < data.contourCount; c++) {
				Contour* contour = &data.contours[c];
				for (int e = 0; e < contour->edgeCount; e++) {
					Edge* edge = &contour->edges[e];
					Vec2 p0 = edge->p0;
					Vec2 p1 = (edge->type == 1) ? edge->p1 : edge->p2;

					if ((p0.y <= p.y && p1.y > p.y) || (p0.y > p.y && p1.y <= p.y)) {
						float vt = (p.y - p0.y) / (p1.y - p0.y);
						if (p.x < p0.x + vt * (p1.x - p0.x)) {
							winding += (p0.y < p1.y) ? 1 : -1;
						}
					}
				}
			}

			// Convert to normalized distance
			float signedDist = (winding != 0) ? minDist : -minDist;
			signedDist /= params->range;
			signedDist = signedDist * 0.5f + 0.5f;
			signedDist = fmaxf(0.0f, fminf(1.0f, signedDist));

			output[y * params->width + x] = (unsigned char)(signedDist * 255.0f);
		}
	}

	// Cleanup
	for (int i = 0; i < data.contourCount; i++) {
		free(data.contours[i].edges);
	}
	free(data.contours);
}

// Assign edge colors based on direction for MSDF
static void assignEdgeColors(OutlineData* data) {
	for (int c = 0; c < data->contourCount; c++) {
		Contour* contour = &data->contours[c];
		for (int e = 0; e < contour->edgeCount; e++) {
			Edge* edge = &contour->edges[e];

			// Compute edge direction angle
			float angle = atan2f(edge->direction.y, edge->direction.x);
			if (angle < 0.0f) angle += 2.0f * 3.14159265f;

			// Assign color based on angle (divide 360° into 3 sectors of 120° each)
			float sector = angle * 3.0f / (2.0f * 3.14159265f);

			if (sector < 1.0f) {
				edge->type |= (1 << 8);  // Red channel (bit 8)
			} else if (sector < 2.0f) {
				edge->type |= (2 << 8);  // Green channel (bit 9)
			} else {
				edge->type |= (3 << 8);  // Blue channel (bits 8+9)
			}
		}
	}
}

void vknvg__generateMSDF(FT_GlyphSlot glyph, unsigned char* output, const VKNVGmsdfParams* params) {
	if (!glyph || !output || !params || glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
		for (int i = 0; i < params->width * params->height * 4; i += 4) {
			output[i] = output[i+1] = output[i+2] = 128;
			output[i+3] = 255;
		}
		return;
	}

	// Decompose outline into contours and edges
	OutlineData data = {0};
	FT_Outline_Funcs funcs = {moveToFunc, lineToFunc, conicToFunc, cubicToFunc, 0, 0};
	FT_Outline_Decompose(&glyph->outline, &funcs, &data);

	// Close last contour
	if (data.contourCount > 0) {
		FT_Vector lastPoint = {(FT_Pos)(data.firstPoint.x * 64.0f), (FT_Pos)(data.firstPoint.y * 64.0f)};
		lineToFunc(&lastPoint, &data);
	}

	// Assign colors to edges based on direction
	assignEdgeColors(&data);

	// Calculate bounding box
	FT_BBox bbox;
	FT_Outline_Get_CBox(&glyph->outline, &bbox);
	float xMin = bbox.xMin / 64.0f;
	float yMin = bbox.yMin / 64.0f;
	float xMax = bbox.xMax / 64.0f;
	float yMax = bbox.yMax / 64.0f;

	// Generate MSDF (one distance per RGB channel)
	for (int y = 0; y < params->height; y++) {
		for (int x = 0; x < params->width; x++) {
			Vec2 p;
			p.x = xMin + (x - params->offsetX) * (xMax - xMin) / (params->width - 2 * params->offsetX);
			p.y = yMin + (y - params->offsetY) * (yMax - yMin) / (params->height - 2 * params->offsetY);

			float minDistR = FLT_MAX;
			float minDistG = FLT_MAX;
			float minDistB = FLT_MAX;

			// Find minimum distance to edges of each color
			for (int c = 0; c < data.contourCount; c++) {
				Contour* contour = &data.contours[c];
				for (int e = 0; e < contour->edgeCount; e++) {
					Edge* edge = &contour->edges[e];
					float t;
					float dist;

					int edgeType = edge->type & 0xFF;
					int edgeColor = (edge->type >> 8) & 0xFF;

					if (edgeType == 1) {
						dist = distanceToLine(p, edge->p0, edge->p1, &t);
					} else {
						dist = distanceToQuadratic(p, edge->p0, edge->p1, edge->p2, &t);
					}

					// Update minimum distance for appropriate channel(s)
					if (edgeColor & 1) minDistR = fminf(minDistR, dist);
					if (edgeColor & 2) minDistG = fminf(minDistG, dist);
					if (edgeColor == 3) minDistB = fminf(minDistB, dist);
				}
			}

			// Determine if point is inside or outside (winding number test)
			int winding = 0;
			for (int c = 0; c < data.contourCount; c++) {
				Contour* contour = &data.contours[c];
				for (int e = 0; e < contour->edgeCount; e++) {
					Edge* edge = &contour->edges[e];
					Vec2 p0 = edge->p0;
					Vec2 p1 = ((edge->type & 0xFF) == 1) ? edge->p1 : edge->p2;

					if ((p0.y <= p.y && p1.y > p.y) || (p0.y > p.y && p1.y <= p.y)) {
						float vt = (p.y - p0.y) / (p1.y - p0.y);
						if (p.x < p0.x + vt * (p1.x - p0.x)) {
							winding += (p0.y < p1.y) ? 1 : -1;
						}
					}
				}
			}

			// Convert to normalized distance for each channel
			float sign = (winding != 0) ? 1.0f : -1.0f;
			float sdR = sign * minDistR / params->range * 0.5f + 0.5f;
			float sdG = sign * minDistG / params->range * 0.5f + 0.5f;
			float sdB = sign * minDistB / params->range * 0.5f + 0.5f;

			sdR = fmaxf(0.0f, fminf(1.0f, sdR));
			sdG = fmaxf(0.0f, fminf(1.0f, sdG));
			sdB = fmaxf(0.0f, fminf(1.0f, sdB));

			int idx = y * params->stride + x * 4;
			output[idx + 0] = (unsigned char)(sdR * 255.0f);
			output[idx + 1] = (unsigned char)(sdG * 255.0f);
			output[idx + 2] = (unsigned char)(sdB * 255.0f);
			output[idx + 3] = 255;
		}
	}

	// Cleanup
	for (int i = 0; i < data.contourCount; i++) {
		free(data.contours[i].edges);
	}
	free(data.contours);
}
