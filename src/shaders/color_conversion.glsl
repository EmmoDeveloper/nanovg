// Master Color Space Conversion
// GLSL include file that combines transfer functions, gamut mapping, and HDR processing
// For complete source→destination color space conversions

#ifndef COLOR_CONVERSION_GLSL
#define COLOR_CONVERSION_GLSL

#include "color_transfer.glsl"
#include "color_gamut.glsl"
#include "color_hdr.glsl"

// Color space conversion parameters
// These should be passed via push constants or uniforms
struct ColorSpaceParams {
	mat3 gamutMatrix;      // Primaries conversion matrix (3x3)
	int srcTransferID;     // Source transfer function (NVGVK_TRANSFER_*)
	int dstTransferID;     // Destination transfer function
	float hdrScale;        // Luminance scaling factor (dst_nits / src_nits)
	int useGamutMapping;   // 1 = use soft gamut mapping, 0 = hard clip
	int useToneMapping;    // 1 = apply tone mapping for HDR→SDR, 0 = simple scale
};

// Complete color space conversion
// Input: color in source color space
// Output: color in destination color space
// preserveAlpha: if true, alpha channel is passed through unchanged
vec4 convertColorSpace(vec4 color, ColorSpaceParams params, bool preserveAlpha) {
	// Step 1: Decode source transfer function (non-linear → linear)
	vec3 linear = applyEOTF(color.rgb, params.srcTransferID);

	// Step 2: Convert gamut (source primaries → destination primaries)
	linear = convertGamut(linear, params.gamutMatrix);

	// Step 3: Handle out-of-gamut colors
	if (params.useGamutMapping != 0) {
		linear = mapToGamut(linear);
	} else {
		linear = clipToGamut(linear);
	}

	// Step 4: Scale HDR luminance
	if (params.hdrScale != 1.0) {
		// Check if we're converting HDR → SDR (scale < 1.0)
		if (params.hdrScale < 1.0 && params.useToneMapping != 0) {
			// Apply tone mapping for better appearance
			linear = hdrToSdr(linear, 1.0 / params.hdrScale);
		} else {
			// Simple linear scaling
			linear = linear * params.hdrScale;
		}
	}

	// Step 5: Encode destination transfer function (linear → non-linear)
	vec3 result = applyOETF(linear, params.dstTransferID);

	// Return with original or unit alpha
	return vec4(result, preserveAlpha ? color.a : 1.0);
}

// Simplified conversion without gamut/tone mapping control
vec4 convertColorSpaceSimple(vec4 color, mat3 gamutMatrix, int srcTransfer, int dstTransfer, float hdrScale) {
	ColorSpaceParams params;
	params.gamutMatrix = gamutMatrix;
	params.srcTransferID = srcTransfer;
	params.dstTransferID = dstTransfer;
	params.hdrScale = hdrScale;
	params.useGamutMapping = 1;  // Use soft mapping by default
	params.useToneMapping = 1;   // Use tone mapping by default

	return convertColorSpace(color, params, true);
}

// Identity conversion (no-op, for same color space)
vec4 convertColorSpaceIdentity(vec4 color) {
	return color;
}

// Quick sRGB → Linear conversion (common case)
vec4 srgbToLinear(vec4 color) {
	return vec4(applyEOTF(color.rgb, TRANSFER_SRGB), color.a);
}

// Quick Linear → sRGB conversion (common case)
vec4 linearToSrgb(vec4 color) {
	return vec4(applyOETF(color.rgb, TRANSFER_SRGB), color.a);
}

#endif // COLOR_CONVERSION_GLSL
