// Color Space UBO (Uniform Buffer Object)
// Shared descriptor set for color space conversion
// Must match NVGVkColorSpaceUniforms in C (nvg_vk_types.h)

#ifndef COLOR_SPACE_UBO_GLSL
#define COLOR_SPACE_UBO_GLSL

// Note: Including shader must have GL_GOOGLE_include_directive extension enabled
#include "color_conversion.glsl"

// Color space conversion uniform buffer (set = 1, binding = 0)
// 64 bytes total, std140 layout
layout(set = 1, binding = 0) uniform ColorSpaceUBO {
	mat3 gamutMatrix;		// 36 bytes (3x3 float matrix)
	int srcTransferID;		// 4 bytes
	int dstTransferID;		// 4 bytes
	float hdrScale;			// 4 bytes
	int useGamutMapping;	// 4 bytes
	int useToneMapping;		// 4 bytes
	int _padding[2];		// 8 bytes (align to 64 bytes)
} colorSpace;				// Total: 64 bytes

// Convert color using the global color space UBO
vec4 applyColorSpace(vec4 color) {
	ColorSpaceParams params;
	params.gamutMatrix = colorSpace.gamutMatrix;
	params.srcTransferID = colorSpace.srcTransferID;
	params.dstTransferID = colorSpace.dstTransferID;
	params.hdrScale = colorSpace.hdrScale;
	params.useGamutMapping = colorSpace.useGamutMapping;
	params.useToneMapping = colorSpace.useToneMapping;

	return convertColorSpace(color, params, true);
}

// Check if color space conversion is identity (no-op)
bool isIdentityColorSpace() {
	return (colorSpace.srcTransferID == colorSpace.dstTransferID &&
	        colorSpace.hdrScale == 1.0 &&
	        colorSpace.gamutMatrix == mat3(1.0));
}

#endif // COLOR_SPACE_UBO_GLSL
