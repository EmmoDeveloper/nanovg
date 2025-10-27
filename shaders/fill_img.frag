#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 fragPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

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

void main() {
	// Transform fragment position by paint matrix to get texture coordinate
	mat3 paintMat3 = mat3(
		frag.paintMat[0].xyz,  // column 0
		frag.paintMat[1].xyz,  // column 1
		frag.paintMat[2].xyz   // column 2
	);
	vec3 pt = vec3(fragPos.x, fragPos.y, 1.0);
	vec2 paintPos = (paintMat3 * pt).xy;

	// Normalize by extent to get UV coordinates [0, 1]
	vec2 uv = paintPos / frag.extent;

	// Sample texture
	vec4 texColor = texture(texSampler, uv);

	// Apply alpha from innerCol (image patterns use white * alpha)
	outColor = texColor * frag.innerCol;
}
