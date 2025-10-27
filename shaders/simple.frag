#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 fragPosition;

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

float scissorMask(vec2 p) {
	vec2 sc = (frag.scissorMat * vec3(p, 1.0)).xy;
	sc = (vec2(0.5) - abs(sc) + frag.scissorExt) * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	float scissor = scissorMask(fragPosition);
	outColor = frag.innerCol * scissor;
}
