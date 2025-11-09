// Color Space Transfer Functions (EOTF/OETF)
// GLSL include file for electro-optical and opto-electronic transfer functions
// Standards: IEC 61966-2-1 (sRGB), ITU-R BT.709, ITU-R BT.2020, SMPTE ST 2084 (PQ), ITU-R BT.2100 (HLG)

#ifndef COLOR_TRANSFER_GLSL
#define COLOR_TRANSFER_GLSL

// Transfer function IDs (must match NVGVkTransferFunctionID in C)
const int TRANSFER_LINEAR = 0;
const int TRANSFER_SRGB = 1;
const int TRANSFER_ITU = 2;
const int TRANSFER_PQ = 3;
const int TRANSFER_HLG = 4;
const int TRANSFER_DCI_P3 = 5;
const int TRANSFER_ADOBE_RGB = 6;

// ============================================================================
// EOTF (Electro-Optical Transfer Function): Non-linear → Linear
// Decodes gamma-encoded values to linear light
// ============================================================================

// sRGB EOTF (IEC 61966-2-1)
// Piecewise function: linear below 0.04045, pow(2.4) above
vec3 eotf_srgb(vec3 nonlinear) {
	vec3 linear;
	for (int i = 0; i < 3; i++) {
		if (nonlinear[i] <= 0.04045) {
			linear[i] = nonlinear[i] / 12.92;
		} else {
			linear[i] = pow((nonlinear[i] + 0.055) / 1.055, 2.4);
		}
	}
	return linear;
}

// ITU BT.709/601 EOTF
// Simple power function with offset
vec3 eotf_itu(vec3 nonlinear) {
	const float beta = 0.018;
	const float alpha = 1.099;
	const float gamma = 0.45;

	vec3 linear;
	for (int i = 0; i < 3; i++) {
		if (nonlinear[i] < beta * 4.5) {
			linear[i] = nonlinear[i] / 4.5;
		} else {
			linear[i] = pow((nonlinear[i] + (alpha - 1.0)) / alpha, 1.0 / gamma);
		}
	}
	return linear;
}

// PQ (Perceptual Quantizer) EETF - SMPTE ST 2084
// Maps [0, 1] to [0, 10,000] cd/m²
// This is the inverse PQ function (EOTF)
vec3 eotf_pq(vec3 nonlinear) {
	const float m1 = 0.1593017578125;      // 2610 / 16384
	const float m2 = 78.84375;             // 2523 / 32 * 128
	const float c1 = 0.8359375;            // 3424 / 4096
	const float c2 = 18.8515625;           // 2413 / 128 * 4096 / 32
	const float c3 = 18.6875;              // 2392 / 128

	vec3 linear;
	for (int i = 0; i < 3; i++) {
		float Np = pow(nonlinear[i], 1.0 / m2);
		float numerator = max(Np - c1, 0.0);
		float denominator = c2 - c3 * Np;
		linear[i] = pow(numerator / denominator, 1.0 / m1);
	}

	// Scale from [0, 1] to [0, 10000] cd/m²
	return linear * 10000.0;
}

// HLG (Hybrid Log-Gamma) OETF⁻¹ - ITU-R BT.2100
// Maps [0, 1] to [0, 1000] cd/m² (scene-referred)
vec3 eotf_hlg(vec3 nonlinear) {
	const float a = 0.17883277;
	const float b = 0.28466892;  // 1 - 4a
	const float c = 0.55991073;  // 0.5 - a * ln(4a)

	vec3 linear;
	for (int i = 0; i < 3; i++) {
		if (nonlinear[i] <= 0.5) {
			linear[i] = (nonlinear[i] * nonlinear[i]) / 3.0;
		} else {
			linear[i] = (exp((nonlinear[i] - c) / a) + b) / 12.0;
		}
	}

	// Nominal peak: 1000 cd/m² for HLG
	return linear * 1000.0;
}

// DCI-P3 Gamma 2.6
vec3 eotf_dci_p3(vec3 nonlinear) {
	return pow(nonlinear, vec3(2.6));
}

// Adobe RGB Gamma 2.2
vec3 eotf_adobe_rgb(vec3 nonlinear) {
	return pow(nonlinear, vec3(2.19921875));  // Exact: 563/256
}

// ============================================================================
// OETF (Opto-Electronic Transfer Function): Linear → Non-linear
// Encodes linear light to gamma-encoded values
// ============================================================================

// sRGB OETF (inverse of EOTF)
vec3 oetf_srgb(vec3 linear) {
	vec3 nonlinear;
	for (int i = 0; i < 3; i++) {
		if (linear[i] <= 0.0031308) {
			nonlinear[i] = linear[i] * 12.92;
		} else {
			nonlinear[i] = 1.055 * pow(linear[i], 1.0 / 2.4) - 0.055;
		}
	}
	return nonlinear;
}

// ITU OETF
vec3 oetf_itu(vec3 linear) {
	const float beta = 0.018;
	const float alpha = 1.099;
	const float gamma = 0.45;

	vec3 nonlinear;
	for (int i = 0; i < 3; i++) {
		if (linear[i] < beta) {
			nonlinear[i] = linear[i] * 4.5;
		} else {
			nonlinear[i] = alpha * pow(linear[i], gamma) - (alpha - 1.0);
		}
	}
	return nonlinear;
}

// PQ OETF - SMPTE ST 2084 (inverse PQ)
vec3 oetf_pq(vec3 linear) {
	// Scale from cd/m² to [0, 1]
	linear = linear / 10000.0;

	const float m1 = 0.1593017578125;
	const float m2 = 78.84375;
	const float c1 = 0.8359375;
	const float c2 = 18.8515625;
	const float c3 = 18.6875;

	vec3 nonlinear;
	for (int i = 0; i < 3; i++) {
		float Y = pow(linear[i], m1);
		nonlinear[i] = pow((c1 + c2 * Y) / (1.0 + c3 * Y), m2);
	}
	return nonlinear;
}

// HLG OETF - ITU-R BT.2100
vec3 oetf_hlg(vec3 linear) {
	// Scale from cd/m² to [0, 1]
	linear = linear / 1000.0;

	const float a = 0.17883277;
	const float b = 0.28466892;
	const float c = 0.55991073;

	vec3 nonlinear;
	for (int i = 0; i < 3; i++) {
		float L = linear[i] * 12.0;  // Denormalize
		if (L <= 1.0) {
			nonlinear[i] = sqrt(3.0 * L);
		} else {
			nonlinear[i] = a * log(L - b) + c;
		}
	}
	return nonlinear;
}

// DCI-P3 inverse gamma
vec3 oetf_dci_p3(vec3 linear) {
	return pow(linear, vec3(1.0 / 2.6));
}

// Adobe RGB inverse gamma
vec3 oetf_adobe_rgb(vec3 linear) {
	return pow(linear, vec3(1.0 / 2.19921875));
}

// ============================================================================
// Dispatch functions: apply EOTF/OETF based on transfer function ID
// ============================================================================

vec3 applyEOTF(vec3 nonlinear, int transferID) {
	if (transferID == TRANSFER_LINEAR) {
		return nonlinear;
	} else if (transferID == TRANSFER_SRGB) {
		return eotf_srgb(nonlinear);
	} else if (transferID == TRANSFER_ITU) {
		return eotf_itu(nonlinear);
	} else if (transferID == TRANSFER_PQ) {
		return eotf_pq(nonlinear);
	} else if (transferID == TRANSFER_HLG) {
		return eotf_hlg(nonlinear);
	} else if (transferID == TRANSFER_DCI_P3) {
		return eotf_dci_p3(nonlinear);
	} else if (transferID == TRANSFER_ADOBE_RGB) {
		return eotf_adobe_rgb(nonlinear);
	}
	return nonlinear;  // Fallback: no conversion
}

vec3 applyOETF(vec3 linear, int transferID) {
	if (transferID == TRANSFER_LINEAR) {
		return linear;
	} else if (transferID == TRANSFER_SRGB) {
		return oetf_srgb(linear);
	} else if (transferID == TRANSFER_ITU) {
		return oetf_itu(linear);
	} else if (transferID == TRANSFER_PQ) {
		return oetf_pq(linear);
	} else if (transferID == TRANSFER_HLG) {
		return oetf_hlg(linear);
	} else if (transferID == TRANSFER_DCI_P3) {
		return oetf_dci_p3(linear);
	} else if (transferID == TRANSFER_ADOBE_RGB) {
		return oetf_adobe_rgb(linear);
	}
	return linear;  // Fallback: no conversion
}

#endif // COLOR_TRANSFER_GLSL
