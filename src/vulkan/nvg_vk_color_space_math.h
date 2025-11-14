#ifndef NVG_VK_COLOR_SPACE_MATH_H
#define NVG_VK_COLOR_SPACE_MATH_H

// Color space mathematics: primaries conversion matrices and white point data
// Based on ITU-R BT.709, BT.2020, SMPTE standards

// 3x3 matrix for color primaries conversion (row-major for C, will be transposed for GLSL column-major)
typedef struct {
	float m[9];  // [0..2] = row 0, [3..5] = row 1, [6..8] = row 2
} NVGVkMat3;

// Quaternion for rotation representation (w, x, y, z)
typedef struct {
	float w, x, y, z;
} NVGVkQuat;

// Decomposed 3x3 matrix: rotation + scale
typedef struct {
	NVGVkQuat rotation;  // Rotation as quaternion
	float scale[3];      // Per-axis scale factors
} NVGVkMat3Decomposed;

// CIE 1931 xy chromaticity coordinates
typedef struct {
	float x, y;
} NVGVkChromaticity;

// Color primaries definition (red, green, blue, white point)
typedef struct {
	NVGVkChromaticity r, g, b, w;
} NVGVkColorPrimaries;

// Standard primaries (from ITU/SMPTE specifications)
extern const NVGVkColorPrimaries NVGVK_PRIMARIES_BT709;      // sRGB, BT.709 (HDTV)
extern const NVGVkColorPrimaries NVGVK_PRIMARIES_BT2020;     // BT.2020 (UHDTV, HDR)
extern const NVGVkColorPrimaries NVGVK_PRIMARIES_DISPLAYP3;  // Display P3 (Apple, DCI)
extern const NVGVkColorPrimaries NVGVK_PRIMARIES_ADOBERGB;   // Adobe RGB (1998)
extern const NVGVkColorPrimaries NVGVK_PRIMARIES_DCI_P3;     // DCI-P3 (digital cinema)

// Matrix operations
void nvgvk_mat3_identity(NVGVkMat3* m);
void nvgvk_mat3_multiply(const NVGVkMat3* a, const NVGVkMat3* b, NVGVkMat3* result);
void nvgvk_mat3_invert(const NVGVkMat3* m, NVGVkMat3* result);

// Build RGB→XYZ matrix from primaries
// Based on ITU-R BT.709-6 Section 3, ITU-R BT.2020-2 Section 4
void nvgvk_primaries_to_xyz_matrix(const NVGVkColorPrimaries* primaries, NVGVkMat3* matrix);

// Build XYZ→RGB matrix (inverse of above)
void nvgvk_xyz_to_primaries_matrix(const NVGVkColorPrimaries* primaries, NVGVkMat3* matrix);

// Build direct primaries conversion matrix: src_RGB → dst_RGB
// This is: dst_XYZ_to_RGB × src_RGB_to_XYZ
void nvgvk_primaries_conversion_matrix(const NVGVkColorPrimaries* src,
                                       const NVGVkColorPrimaries* dst,
                                       NVGVkMat3* matrix);

// Quaternion operations
void nvgvk_quat_identity(NVGVkQuat* q);
void nvgvk_quat_multiply(const NVGVkQuat* a, const NVGVkQuat* b, NVGVkQuat* result);
void nvgvk_quat_normalize(NVGVkQuat* q);
void nvgvk_quat_slerp(const NVGVkQuat* a, const NVGVkQuat* b, float t, NVGVkQuat* result);
void nvgvk_quat_to_mat3(const NVGVkQuat* q, NVGVkMat3* m);
void nvgvk_quat_from_mat3(const NVGVkMat3* m, NVGVkQuat* q);

// Matrix decomposition (polar decomposition into rotation + scale)
void nvgvk_mat3_decompose(const NVGVkMat3* m, NVGVkMat3Decomposed* decomposed);
void nvgvk_mat3_compose(const NVGVkMat3Decomposed* decomposed, NVGVkMat3* m);

// Interpolate between two matrices using quaternion SLERP for rotation
void nvgvk_mat3_interpolate(const NVGVkMat3* a, const NVGVkMat3* b, float t, NVGVkMat3* result);

#endif // NVG_VK_COLOR_SPACE_MATH_H
