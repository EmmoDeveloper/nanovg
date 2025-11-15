#version 450

layout(location = 0) in vec2 fragTexCoord;

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
	// Sample grayscale alpha texture
	float alpha = texture(texSampler, fragTexCoord).r;

	// Apply text color with alpha from texture (non-premultiplied for SRC_ALPHA blend)
	outColor = vec4(frag.innerCol.rgb, frag.innerCol.a * alpha);
}
