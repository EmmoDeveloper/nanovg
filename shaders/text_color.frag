#version 450

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(set = 0, binding = 1) uniform FragmentUniforms {
	mat3 scissorMat;
	mat3 paintMat;
	vec4 innerCol;
	vec4 outerCol;
	vec2 scissorExt;
	vec2 scissorScale;
	vec2 extent;
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	float texType;
	float type;
} frag;

float scissorMask(vec2 p) {
	vec2 sc = (abs((frag.scissorMat * vec3(p, 1.0)).xy) - frag.scissorExt);
	sc = vec2(0.5, 0.5) - sc * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	// Sample RGBA color texture (color emoji/color fonts)
	vec4 texColor = texture(tex, ftcoord);

	// Apply tint from innerCol if needed
	// For color emoji, typically we want to preserve the original colors
	// But allow modulation via innerCol.a for opacity control
	outColor = vec4(texColor.rgb, texColor.a * frag.innerCol.a);

	// Apply scissor mask
	outColor *= scissorMask(fpos);
}
