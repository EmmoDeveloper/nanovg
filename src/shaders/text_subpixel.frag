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
	// Sample the LCD texture - RGB channels contain subpixel coverage
	vec4 texColor = texture(texSampler, fragTexCoord);

	// Extract RGB subpixel coverage values
	vec3 coverage = texColor.rgb;

	// Apply text color to each subpixel channel
	vec3 rgb = coverage * frag.innerCol.rgb;

	// Use average coverage as overall alpha
	float alpha = (coverage.r + coverage.g + coverage.b) * 0.333 * frag.innerCol.a;

	// Output premultiplied color for proper blending
	outColor = vec4(rgb, alpha);
}
