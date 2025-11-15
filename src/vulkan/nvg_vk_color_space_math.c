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

// Quaternion operations

void nvgvk_quat_identity(NVGVkQuat* q)
{
	q->w = 1.0f;
	q->x = 0.0f;
	q->y = 0.0f;
	q->z = 0.0f;
}

void nvgvk_quat_multiply(const NVGVkQuat* a, const NVGVkQuat* b, NVGVkQuat* result)
{
	NVGVkQuat temp;
	temp.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
	temp.x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y;
	temp.y = a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x;
	temp.z = a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w;
	*result = temp;
}

void nvgvk_quat_normalize(NVGVkQuat* q)
{
	float len = sqrtf(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
	if (len > 1e-10f) {
		float invLen = 1.0f / len;
		q->w *= invLen;
		q->x *= invLen;
		q->y *= invLen;
		q->z *= invLen;
	} else {
		nvgvk_quat_identity(q);
	}
}

void nvgvk_quat_slerp(const NVGVkQuat* a, const NVGVkQuat* b, float t, NVGVkQuat* result)
{
	// Compute dot product
	float dot = a->w * b->w + a->x * b->x + a->y * b->y + a->z * b->z;

	// If negative, negate one quaternion to take shorter path
	NVGVkQuat b_copy = *b;
	if (dot < 0.0f) {
		b_copy.w = -b_copy.w;
		b_copy.x = -b_copy.x;
		b_copy.y = -b_copy.y;
		b_copy.z = -b_copy.z;
		dot = -dot;
	}

	// Clamp dot to avoid numerical issues
	if (dot > 0.9995f) {
		// Linear interpolation for very close quaternions
		result->w = a->w + t * (b_copy.w - a->w);
		result->x = a->x + t * (b_copy.x - a->x);
		result->y = a->y + t * (b_copy.y - a->y);
		result->z = a->z + t * (b_copy.z - a->z);
		nvgvk_quat_normalize(result);
		return;
	}

	// Spherical interpolation
	float theta = acosf(dot);
	float sinTheta = sinf(theta);
	float wa = sinf((1.0f - t) * theta) / sinTheta;
	float wb = sinf(t * theta) / sinTheta;

	result->w = wa * a->w + wb * b_copy.w;
	result->x = wa * a->x + wb * b_copy.x;
	result->y = wa * a->y + wb * b_copy.y;
	result->z = wa * a->z + wb * b_copy.z;
}

void nvgvk_quat_to_mat3(const NVGVkQuat* q, NVGVkMat3* m)
{
	float xx = q->x * q->x;
	float yy = q->y * q->y;
	float zz = q->z * q->z;
	float xy = q->x * q->y;
	float xz = q->x * q->z;
	float yz = q->y * q->z;
	float wx = q->w * q->x;
	float wy = q->w * q->y;
	float wz = q->w * q->z;

	m->m[0] = 1.0f - 2.0f * (yy + zz);
	m->m[1] = 2.0f * (xy - wz);
	m->m[2] = 2.0f * (xz + wy);

	m->m[3] = 2.0f * (xy + wz);
	m->m[4] = 1.0f - 2.0f * (xx + zz);
	m->m[5] = 2.0f * (yz - wx);

	m->m[6] = 2.0f * (xz - wy);
	m->m[7] = 2.0f * (yz + wx);
	m->m[8] = 1.0f - 2.0f * (xx + yy);
}

void nvgvk_quat_from_mat3(const NVGVkMat3* m, NVGVkQuat* q)
{
	// Shepperd's method for numerical stability
	float trace = m->m[0] + m->m[4] + m->m[8];

	if (trace > 0.0f) {
		float s = sqrtf(trace + 1.0f) * 2.0f;
		q->w = 0.25f * s;
		q->x = (m->m[7] - m->m[5]) / s;
		q->y = (m->m[2] - m->m[6]) / s;
		q->z = (m->m[3] - m->m[1]) / s;
	} else if (m->m[0] > m->m[4] && m->m[0] > m->m[8]) {
		float s = sqrtf(1.0f + m->m[0] - m->m[4] - m->m[8]) * 2.0f;
		q->w = (m->m[7] - m->m[5]) / s;
		q->x = 0.25f * s;
		q->y = (m->m[1] + m->m[3]) / s;
		q->z = (m->m[2] + m->m[6]) / s;
	} else if (m->m[4] > m->m[8]) {
		float s = sqrtf(1.0f + m->m[4] - m->m[0] - m->m[8]) * 2.0f;
		q->w = (m->m[2] - m->m[6]) / s;
		q->x = (m->m[1] + m->m[3]) / s;
		q->y = 0.25f * s;
		q->z = (m->m[5] + m->m[7]) / s;
	} else {
		float s = sqrtf(1.0f + m->m[8] - m->m[0] - m->m[4]) * 2.0f;
		q->w = (m->m[3] - m->m[1]) / s;
		q->x = (m->m[2] + m->m[6]) / s;
		q->y = (m->m[5] + m->m[7]) / s;
		q->z = 0.25f * s;
	}

	nvgvk_quat_normalize(q);
}

// Simplified decomposition using Gram-Schmidt orthogonalization
// Color space matrices are close to orthogonal, so this provides good approximation
void nvgvk_mat3_decompose(const NVGVkMat3* m, NVGVkMat3Decomposed* decomposed)
{
	// Extract column vectors
	float v0[3] = {m->m[0], m->m[3], m->m[6]};
	float v1[3] = {m->m[1], m->m[4], m->m[7]};
	float v2[3] = {m->m[2], m->m[5], m->m[8]};

	// Compute scales as column vector magnitudes
	decomposed->scale[0] = sqrtf(v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2]);
	decomposed->scale[1] = sqrtf(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
	decomposed->scale[2] = sqrtf(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);

	// Normalize to get rotation matrix
	NVGVkMat3 r;
	if (decomposed->scale[0] > 1e-10f && decomposed->scale[1] > 1e-10f && decomposed->scale[2] > 1e-10f) {
		r.m[0] = v0[0] / decomposed->scale[0];
		r.m[3] = v0[1] / decomposed->scale[0];
		r.m[6] = v0[2] / decomposed->scale[0];

		r.m[1] = v1[0] / decomposed->scale[1];
		r.m[4] = v1[1] / decomposed->scale[1];
		r.m[7] = v1[2] / decomposed->scale[1];

		r.m[2] = v2[0] / decomposed->scale[2];
		r.m[5] = v2[1] / decomposed->scale[2];
		r.m[8] = v2[2] / decomposed->scale[2];
	} else {
		nvgvk_mat3_identity(&r);
		decomposed->scale[0] = decomposed->scale[1] = decomposed->scale[2] = 1.0f;
	}

	// Convert rotation matrix to quaternion
	nvgvk_quat_from_mat3(&r, &decomposed->rotation);
}

void nvgvk_mat3_compose(const NVGVkMat3Decomposed* decomposed, NVGVkMat3* m)
{
	// Convert quaternion to rotation matrix
	NVGVkMat3 r;
	nvgvk_quat_to_mat3(&decomposed->rotation, &r);

	// Apply scale: M = R × S
	for (int i = 0; i < 3; i++) {
		m->m[i * 3 + 0] = r.m[i * 3 + 0] * decomposed->scale[0];
		m->m[i * 3 + 1] = r.m[i * 3 + 1] * decomposed->scale[1];
		m->m[i * 3 + 2] = r.m[i * 3 + 2] * decomposed->scale[2];
	}
}

void nvgvk_mat3_interpolate(const NVGVkMat3* a, const NVGVkMat3* b, float t, NVGVkMat3* result)
{
	// Simple linear interpolation for color space matrices
	// Color matrices contain shear and cannot be accurately decomposed into rotation+scale
	for (int i = 0; i < 9; i++) {
		result->m[i] = a->m[i] + t * (b->m[i] - a->m[i]);
	}
}
