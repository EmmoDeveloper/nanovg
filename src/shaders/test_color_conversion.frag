#version 450
#extension GL_GOOGLE_include_directive : require

// Test fragment shader to verify color conversion GLSL compiles

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

// Color space conversion parameters (shared across all draws)
layout(set = 1, binding = 0) uniform ColorSpaceUBO {
	mat3 gamutMatrix;       // 3x3 matrix for gamut conversion
	int srcTransferID;      // Source transfer function
	int dstTransferID;      // Destination transfer function
	float hdrScale;         // HDR luminance scale
	int useGamutMapping;    // Soft gamut mapping flag
	int useToneMapping;     // Tone mapping flag
} colorSpace;


// Include color conversion system
#include "color_conversion.glsl"

layout(push_constant) uniform PushConstants {
	mat3 gamutMatrix;
	int srcTransfer;
	int dstTransfer;
	float hdrScale;
} pc;

void main() {
	// Test: convert a test color
	vec4 testColor = vec4(0.5, 0.3, 0.8, 1.0);

	// Apply conversion
	outColor = convertColorSpaceSimple(
		testColor,
		pc.gamutMatrix,
		pc.srcTransfer,
		pc.dstTransfer,
		pc.hdrScale
	);
}
