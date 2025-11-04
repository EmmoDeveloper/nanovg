// HDR Color Processing
// GLSL include file for HDR tone mapping and luminance scaling

#ifndef COLOR_HDR_GLSL
#define COLOR_HDR_GLSL

// Scale linear light from source luminance range to destination
// srcNits, dstNits: peak luminance in cd/m²
// Example: SDR (80 cd/m²) → HDR10 (10,000 cd/m²) needs scaling
vec3 scaleHDR(vec3 linear, float srcNits, float dstNits) {
	if (srcNits <= 0.0 || dstNits <= 0.0) {
		return linear;
	}
	return linear * (dstNits / srcNits);
}

// ACES Filmic Tone Mapping
// Maps HDR [0, inf) to SDR [0, 1] with pleasing film-like rolloff
// Good for HDR → SDR conversion
vec3 toneMapACES(vec3 hdr) {
	// ACES approximation by Narkowicz
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;

	vec3 x = hdr;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reinhard Tone Mapping
// Simple HDR → SDR compression
vec3 toneMapReinhard(vec3 hdr) {
	return hdr / (1.0 + hdr);
}

// Reinhard Extended (with white point)
// Allows controlling where highlights clip
vec3 toneMapReinhardExtended(vec3 hdr, float whitePoint) {
	vec3 numerator = hdr * (1.0 + (hdr / (whitePoint * whitePoint)));
	return numerator / (1.0 + hdr);
}

// Hable/Uncharted 2 Tone Mapping
// Used in Uncharted 2, very cinematic look
vec3 toneMapHable(vec3 hdr) {
	const float A = 0.15;  // Shoulder strength
	const float B = 0.50;  // Linear strength
	const float C = 0.10;  // Linear angle
	const float D = 0.20;  // Toe strength
	const float E = 0.02;  // Toe numerator
	const float F = 0.30;  // Toe denominator

	vec3 x = hdr;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Luminance-based tone mapping
// Compresses luminance while preserving chromaticity
vec3 toneMapLuminance(vec3 hdr, float maxLuminance) {
	// Calculate luminance (BT.709)
	float Y = dot(hdr, vec3(0.2126, 0.7152, 0.0722));

	if (Y <= 0.0) {
		return hdr;
	}

	// Tone map luminance
	float Yt = Y / (1.0 + Y / maxLuminance);

	// Preserve chromaticity
	return hdr * (Yt / Y);
}

// Simple linear scaling with soft clip
// Maps [0, srcPeak] to [0, 1] with smooth rolloff above srcPeak
vec3 scaleAndClip(vec3 linear, float srcPeak) {
	return linear / (srcPeak + linear);
}

// HDR to SDR conversion helper
// Combines scaling and tone mapping
vec3 hdrToSdr(vec3 hdrLinear, float hdrPeak) {
	// Scale to approximately SDR range
	vec3 scaled = hdrLinear * (80.0 / hdrPeak);

	// Apply tone mapping for values > 1.0
	return toneMapACES(scaled);
}

// SDR to HDR conversion helper
// Expands SDR content to HDR luminance range
vec3 sdrToHdr(vec3 sdrLinear, float hdrPeak) {
	// Simple linear scaling
	// More sophisticated: could apply inverse EOTF or expand highlights
	return sdrLinear * (hdrPeak / 80.0);
}

#endif // COLOR_HDR_GLSL
