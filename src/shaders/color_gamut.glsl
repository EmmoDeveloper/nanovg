// Color Gamut Conversion
// GLSL include file for converting between different RGB primaries (color gamuts)
// Via CIE XYZ intermediate color space

#ifndef COLOR_GAMUT_GLSL
#define COLOR_GAMUT_GLSL

// Apply 3x3 color space conversion matrix
// Input: linear RGB in source gamut
// Output: linear RGB in destination gamut
vec3 convertGamut(vec3 rgb, mat3 conversionMatrix) {
	return conversionMatrix * rgb;
}

// Clip out-of-gamut colors to valid range
// Simple hard clipping - preserves hue but changes saturation
vec3 clipToGamut(vec3 rgb) {
	return clamp(rgb, 0.0, 1.0);
}

// Gamut mapping: preserve appearance of out-of-gamut colors
// Uses simple desaturation to bring colors into gamut
// More sophisticated than hard clipping
vec3 mapToGamut(vec3 rgb) {
	// If all channels in range, no mapping needed
	if (rgb.r >= 0.0 && rgb.r <= 1.0 &&
	    rgb.g >= 0.0 && rgb.g <= 1.0 &&
	    rgb.b >= 0.0 && rgb.b <= 1.0) {
		return rgb;
	}

	// Calculate luminance (BT.709 coefficients)
	float Y = dot(rgb, vec3(0.2126, 0.7152, 0.0722));

	// Desaturate toward luminance until in gamut
	// This preserves perceptual lightness better than clipping
	float alpha = 0.0;
	vec3 desaturated;

	// Binary search for minimum alpha that brings color into gamut
	for (int i = 0; i < 10; i++) {  // 10 iterations gives 0.1% precision
		alpha = alpha + pow(0.5, float(i + 1));
		desaturated = mix(rgb, vec3(Y), alpha);

		if (desaturated.r < 0.0 || desaturated.r > 1.0 ||
		    desaturated.g < 0.0 || desaturated.g > 1.0 ||
		    desaturated.b < 0.0 || desaturated.b > 1.0) {
			// Still out of gamut, continue
		} else {
			// In gamut, reduce alpha
			alpha = alpha - pow(0.5, float(i + 1));
		}
	}

	return mix(rgb, vec3(Y), alpha);
}

// Alternative: Soft clip (Reinhard tone mapping per channel)
// Gradually compresses values approaching the gamut boundary
vec3 softClipToGamut(vec3 rgb) {
	vec3 result;
	for (int i = 0; i < 3; i++) {
		if (rgb[i] < 0.0) {
			result[i] = 0.0;
		} else if (rgb[i] > 1.0) {
			// Reinhard: x / (1 + x)
			// Maps [1, inf) to [0.5, 1.0]
			result[i] = rgb[i] / (1.0 + rgb[i]);
		} else {
			result[i] = rgb[i];
		}
	}
	return result;
}

#endif // COLOR_GAMUT_GLSL
