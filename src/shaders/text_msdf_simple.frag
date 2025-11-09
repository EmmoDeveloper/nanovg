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

// MSDF helper functions
float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(vec2 texCoord) {
	// Calculate the distance range in screen pixels
	// MSDF_PIXEL_RANGE from nvg_freetype.c must match this value
	const float MSDF_PIXEL_RANGE = 16.0;
	vec2 unitRange = vec2(MSDF_PIXEL_RANGE) / vec2(textureSize(texSampler, 0));
	vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main() {
	// Sample MSDF texture (RGB channels contain distance field)
	vec3 msdf = texture(texSampler, fragTexCoord).rgb;

	// Calculate signed distance from median of RGB channels
	float sd = median(msdf.r, msdf.g, msdf.b);

	// Convert to screen-space distance
	float screenPxDistance = screenPxRange(fragTexCoord) * (sd - 0.5);

	// Calculate opacity with antialiasing
	float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	// Apply text color from innerCol
	outColor = vec4(frag.innerCol.rgb, opacity * frag.innerCol.a);
}
