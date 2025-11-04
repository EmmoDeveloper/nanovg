#include "nvg_vk_color_space_math.h"
#include <math.h>
#include <string.h>

// Standard color primaries definitions (CIE 1931 xy chromaticity coordinates)
// Sources: ITU-R BT.709-6, ITU-R BT.2020-2, SMPTE EG 432-1, Adobe RGB (1998) spec

// BT.709 / sRGB primaries (HDTV, web standard)
// ITU-R BT.709-6, IEC 61966-2-1
const NVGVkColorPrimaries NVGVK_PRIMARIES_BT709 = {
	{0.64f, 0.33f},  // Red
	{0.30f, 0.60f},  // Green
	{0.15f, 0.06f},  // Blue
	{0.3127f, 0.3290f}  // D65 white point
};

// BT.2020 primaries (UHDTV, HDR)
// ITU-R BT.2020-2
const NVGVkColorPrimaries NVGVK_PRIMARIES_BT2020 = {
	{0.708f, 0.292f},  // Red
	{0.170f, 0.797f},  // Green
	{0.131f, 0.046f},  // Blue
	{0.3127f, 0.3290f}  // D65 white point
};

// Display P3 primaries (Apple displays, DCI-P3 with D65)
// SMPTE EG 432-1, DCI Specification V1.0
const NVGVkColorPrimaries NVGVK_PRIMARIES_DISPLAYP3 = {
	{0.680f, 0.320f},  // Red
	{0.265f, 0.690f},  // Green
	{0.150f, 0.060f},  // Blue
	{0.3127f, 0.3290f}  // D65 white point
};

// Adobe RGB (1998) primaries
// Adobe RGB (1998) Color Image Encoding
const NVGVkColorPrimaries NVGVK_PRIMARIES_ADOBERGB = {
	{0.64f, 0.33f},  // Red
	{0.21f, 0.71f},  // Green
	{0.15f, 0.06f},  // Blue
	{0.3127f, 0.3290f}  // D65 white point
};

// DCI-P3 primaries (digital cinema, DCI white point)
// SMPTE RP 431-2
const NVGVkColorPrimaries NVGVK_PRIMARIES_DCI_P3 = {
	{0.680f, 0.320f},  // Red
	{0.265f, 0.690f},  // Green
	{0.150f, 0.060f},  // Blue
	{0.314f, 0.351f}  // DCI white point (not D65!)
};

// Matrix operations

void nvgvk_mat3_identity(NVGVkMat3* m)
{
	memset(m->m, 0, sizeof(m->m));
	m->m[0] = 1.0f;
	m->m[4] = 1.0f;
	m->m[8] = 1.0f;
}

void nvgvk_mat3_multiply(const NVGVkMat3* a, const NVGVkMat3* b, NVGVkMat3* result)
{
	NVGVkMat3 temp;
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			temp.m[row * 3 + col] =
				a->m[row * 3 + 0] * b->m[0 * 3 + col] +
				a->m[row * 3 + 1] * b->m[1 * 3 + col] +
				a->m[row * 3 + 2] * b->m[2 * 3 + col];
		}
	}
	*result = temp;
}

void nvgvk_mat3_invert(const NVGVkMat3* m, NVGVkMat3* result)
{
	const float* a = m->m;
	float* inv = result->m;

	// Calculate determinant and cofactor matrix
	float det = a[0] * (a[4] * a[8] - a[7] * a[5]) -
	            a[1] * (a[3] * a[8] - a[5] * a[6]) +
	            a[2] * (a[3] * a[7] - a[4] * a[6]);

	if (fabs(det) < 1e-10f) {
		// Singular matrix, return identity
		nvgvk_mat3_identity(result);
		return;
	}

	float invDet = 1.0f / det;

	inv[0] = (a[4] * a[8] - a[7] * a[5]) * invDet;
	inv[1] = (a[2] * a[7] - a[1] * a[8]) * invDet;
	inv[2] = (a[1] * a[5] - a[2] * a[4]) * invDet;
	inv[3] = (a[5] * a[6] - a[3] * a[8]) * invDet;
	inv[4] = (a[0] * a[8] - a[2] * a[6]) * invDet;
	inv[5] = (a[3] * a[2] - a[0] * a[5]) * invDet;
	inv[6] = (a[3] * a[7] - a[6] * a[4]) * invDet;
	inv[7] = (a[6] * a[1] - a[0] * a[7]) * invDet;
	inv[8] = (a[0] * a[4] - a[3] * a[1]) * invDet;
}

// Build RGB→XYZ conversion matrix from primaries
// Algorithm from ITU-R BT.709-6 Section 3.3, also described in Poynton's "Digital Video and HD"
void nvgvk_primaries_to_xyz_matrix(const NVGVkColorPrimaries* primaries, NVGVkMat3* matrix)
{
	// Convert xy chromaticity to XYZ (assuming Y=1)
	// For a given (x,y): X = x/y, Y = 1, Z = (1-x-y)/y

	float Xr = primaries->r.x / primaries->r.y;
	float Yr = 1.0f;
	float Zr = (1.0f - primaries->r.x - primaries->r.y) / primaries->r.y;

	float Xg = primaries->g.x / primaries->g.y;
	float Yg = 1.0f;
	float Zg = (1.0f - primaries->g.x - primaries->g.y) / primaries->g.y;

	float Xb = primaries->b.x / primaries->b.y;
	float Yb = 1.0f;
	float Zb = (1.0f - primaries->b.x - primaries->b.y) / primaries->b.y;

	// White point
	float Xw = primaries->w.x / primaries->w.y;
	float Yw = 1.0f;
	float Zw = (1.0f - primaries->w.x - primaries->w.y) / primaries->w.y;

	// Build matrix of primaries
	NVGVkMat3 primariesMatrix;
	primariesMatrix.m[0] = Xr; primariesMatrix.m[1] = Xg; primariesMatrix.m[2] = Xb;
	primariesMatrix.m[3] = Yr; primariesMatrix.m[4] = Yg; primariesMatrix.m[5] = Yb;
	primariesMatrix.m[6] = Zr; primariesMatrix.m[7] = Zg; primariesMatrix.m[8] = Zb;

	// Invert to get scaling factors
	NVGVkMat3 primariesInv;
	nvgvk_mat3_invert(&primariesMatrix, &primariesInv);

	// Multiply by white point to get RGB scaling factors (Sr, Sg, Sb)
	float Sr = primariesInv.m[0] * Xw + primariesInv.m[1] * Yw + primariesInv.m[2] * Zw;
	float Sg = primariesInv.m[3] * Xw + primariesInv.m[4] * Yw + primariesInv.m[5] * Zw;
	float Sb = primariesInv.m[6] * Xw + primariesInv.m[7] * Yw + primariesInv.m[8] * Zw;

	// Final RGB→XYZ matrix: scale each column of primaries matrix
	matrix->m[0] = Xr * Sr; matrix->m[1] = Xg * Sg; matrix->m[2] = Xb * Sb;
	matrix->m[3] = Yr * Sr; matrix->m[4] = Yg * Sg; matrix->m[5] = Yb * Sb;
	matrix->m[6] = Zr * Sr; matrix->m[7] = Zg * Sg; matrix->m[8] = Zb * Sb;
}

void nvgvk_xyz_to_primaries_matrix(const NVGVkColorPrimaries* primaries, NVGVkMat3* matrix)
{
	NVGVkMat3 rgbToXyz;
	nvgvk_primaries_to_xyz_matrix(primaries, &rgbToXyz);
	nvgvk_mat3_invert(&rgbToXyz, matrix);
}

void nvgvk_primaries_conversion_matrix(const NVGVkColorPrimaries* src,
                                       const NVGVkColorPrimaries* dst,
                                       NVGVkMat3* matrix)
{
	NVGVkMat3 srcToXYZ, xyzToDst;
	nvgvk_primaries_to_xyz_matrix(src, &srcToXYZ);
	nvgvk_xyz_to_primaries_matrix(dst, &xyzToDst);
	nvgvk_mat3_multiply(&xyzToDst, &srcToXYZ, matrix);
}
