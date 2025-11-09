#version 450

// Fragment input
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec2 inPosition;

// Fragment output
layout(location = 0) out vec4 outColor;

// Uniform buffer for fragment shader parameters
layout(set = 0, binding = 0) uniform FragmentUniforms {
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
	int texType;
	int type;
} frag;

// Alpha texture sampler for subpixel rendering
layout(set = 0, binding = 1) uniform sampler2D tex;

// Scissoring mask (optimized)
float scissorMask(vec2 p) {
	vec2 sc = (frag.scissorMat * vec3(p, 1.0)).xy;
	sc = (vec2(0.5) - abs(sc) + frag.scissorExt) * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	// Transform position for texture sampling
	vec2 pt = (frag.paintMat * vec3(inPosition, 1.0)).xy / frag.extent;

	// Calculate subpixel offset (1/3 of a pixel for RGB subpixels)
	vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
	float subpixelOffset = texelSize.x * 0.333;

	// Sample three times with subpixel offsets for RGB channels
	// This assumes horizontal RGB stripe layout (most common)
	float r = texture(tex, pt + vec2(-subpixelOffset, 0.0)).r;
	float g = texture(tex, pt).r;
	float b = texture(tex, pt + vec2(subpixelOffset, 0.0)).r;

	// Construct subpixel-antialiased RGB values
	vec3 rgb = vec3(r, g, b);

	// Apply text color
	rgb *= frag.innerCol.rgb;

	// Calculate average alpha for the fragment
	float alpha = (r + g + b) * 0.333 * frag.innerCol.a;

	// Apply scissor mask
	float scissor = scissorMask(inPosition);

	// Output with subpixel-antialiased RGB
	outColor = vec4(rgb, alpha * scissor);
}
