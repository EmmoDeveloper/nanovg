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

void main() {
	// Simple gradient from inner to outer color
	float d = length(fragTexCoord - 0.5);
	outColor = mix(frag.innerCol, frag.outerCol, d);
}
