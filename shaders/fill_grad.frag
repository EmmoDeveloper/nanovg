#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 fragPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform FragUniforms {
	mat3x4 scissorMat;
	mat3x4 paintMat;
	vec4 innerCol;
	vec4 outerCol;
	vec2 scissorExt;
	vec2 scissorScale;
	vec2 extent;
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
} frag;

// Signed distance function for rounded rectangle
float sdroundrect(vec2 pt, vec2 ext, float rad) {
	vec2 ext2 = ext - vec2(rad, rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

void main() {
	// Transform fragment position by paint matrix to get gradient coordinate
	// paintMat is mat3x4 (3 cols × 4 rows in GLSL layout)
	// Extract first 3 columns, first 3 rows to form mat3
	mat3 paintMat3 = mat3(
		frag.paintMat[0].xyz,  // column 0
		frag.paintMat[1].xyz,  // column 1
		frag.paintMat[2].xyz   // column 2
	);
	vec3 pt = vec3(fragPos.x, fragPos.y, 1.0);
	vec2 paintPos = (paintMat3 * pt).xy;

	// Unified gradient formula (works for linear, radial, and box gradients)
	float d = clamp((sdroundrect(paintPos, frag.extent, frag.radius) + frag.feather*0.5) / frag.feather, 0.0, 1.0);

	// Interpolate between innerCol and outerCol
	outColor = mix(frag.innerCol, frag.outerCol, d);
}
