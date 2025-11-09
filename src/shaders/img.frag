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
	vec4 texColor = texture(texSampler, fragTexCoord);
	// Match OpenGL behavior:
	// texType: 0=RGBA premultiplied, 1=RGBA non-premultiplied, 2=ALPHA

	if (frag.texType == 2) {
		// ALPHA texture: use R channel as grayscale mask
		float alpha = texColor.r;
		outColor = vec4(frag.innerCol.rgb, frag.innerCol.a) * alpha;
	} else if (frag.texType == 1) {
		// RGBA non-premultiplied: premultiply alpha
		vec4 color = vec4(texColor.xyz * texColor.w, texColor.w);
		outColor = color * frag.innerCol;
	} else {
		// RGBA premultiplied or other
		outColor = texColor * frag.innerCol;
	}
}
