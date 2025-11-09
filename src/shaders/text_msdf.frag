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

// MSDF helper functions
float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(vec2 texCoord) {
	// Calculate the distance range in screen pixels
	vec2 unitRange = vec2(4.0) / vec2(textureSize(tex, 0));
	vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float scissorMask(vec2 p) {
	vec2 sc = (abs((frag.scissorMat * vec3(p, 1.0)).xy) - frag.scissorExt);
	sc = vec2(0.5, 0.5) - sc * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	// Sample MSDF texture (RGB channels contain distance field)
	vec3 msdf = texture(tex, ftcoord).rgb;

	// Calculate signed distance from median of RGB channels
	float sd = median(msdf.r, msdf.g, msdf.b);

	// Convert to screen-space distance
	float screenPxDistance = screenPxRange(ftcoord) * (sd - 0.5);

	// Calculate opacity with antialiasing
	float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	// Apply stroke effects if enabled
	if (frag.strokeMult > 0.0) {
		// Outline effect
		float outlineWidth = frag.strokeMult * screenPxRange(ftcoord);
		float outerEdge = 0.5 - outlineWidth;
		float outlineAlpha = clamp((sd - outerEdge) / outlineWidth, 0.0, 1.0);

		// Combine fill and outline
		vec3 finalColor = mix(frag.outerCol.rgb, frag.innerCol.rgb, opacity);
		float finalAlpha = max(opacity * frag.innerCol.a, outlineAlpha * frag.outerCol.a);

		outColor = vec4(finalColor, finalAlpha);
	} else {
		// Simple fill
		outColor = vec4(frag.innerCol.rgb, opacity * frag.innerCol.a);
	}

	// Apply glow effect if feather > 0
	if (frag.feather > 0.0) {
		float glowWidth = frag.feather * screenPxRange(ftcoord);
		float glowEdge = 0.5 + glowWidth;
		float glowAlpha = 1.0 - clamp((sd - 0.5) / glowWidth, 0.0, 1.0);

		// Add glow to existing color
		outColor.rgb = mix(outColor.rgb, frag.outerCol.rgb, glowAlpha * (1.0 - outColor.a));
		outColor.a = max(outColor.a, glowAlpha * frag.outerCol.a);
	}

	// Apply scissor mask
	outColor *= scissorMask(fpos);
}
