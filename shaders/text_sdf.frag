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

// SDF texture sampler
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

	// Sample SDF texture (distance stored in alpha channel)
	float distance = texture(tex, pt).r;

	// Calculate edge width for antialiasing
	float edgeWidth = fwidth(distance) * 0.7;

	// Base text alpha (main fill)
	float fillAlpha = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, distance);

	// Outline effect (controlled by strokeMult)
	// When strokeMult > 0, create an outline using outerCol
	float outlineWidth = frag.strokeMult * 0.1;	// Outline width in SDF space
	float outlineAlpha = 0.0;
	vec3 finalColor = frag.innerCol.rgb;

	if (outlineWidth > 0.0) {
		// Outline is the region between the fill edge and the outer edge
		float outerEdge = 0.5 - outlineWidth;
		outlineAlpha = smoothstep(outerEdge - edgeWidth, outerEdge + edgeWidth, distance);
		outlineAlpha *= (1.0 - fillAlpha);	// Only show outline where fill is transparent

		// Blend outline color
		finalColor = mix(frag.outerCol.rgb, frag.innerCol.rgb, fillAlpha / max(fillAlpha + outlineAlpha, 0.001));
	}

	// Glow effect (controlled by feather)
	// When feather > 1.0, add a soft glow extending beyond the outline
	float glowAlpha = 0.0;
	if (frag.feather > 1.0) {
		float glowWidth = (frag.feather - 1.0) * 0.2;
		float glowEdge = 0.5 - outlineWidth - glowWidth;
		glowAlpha = smoothstep(glowEdge - edgeWidth * 2.0, glowEdge, distance);
		glowAlpha *= (1.0 - fillAlpha - outlineAlpha) * 0.5;	// Fade the glow

		// Add glow to color
		finalColor = mix(finalColor, frag.outerCol.rgb, glowAlpha * 0.3);
	}

	// Shadow effect (controlled by radius parameter)
	// When radius > 0, sample offset texture for shadow
	float shadowAlpha = 0.0;
	if (frag.radius > 0.0) {
		vec2 shadowOffset = vec2(frag.radius * 0.01, frag.radius * 0.01);	// Shadow offset
		float shadowDist = texture(tex, pt + shadowOffset).r;
		shadowAlpha = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, shadowDist);
		shadowAlpha *= (1.0 - fillAlpha - outlineAlpha) * frag.outerCol.a;	// Shadow under text

		// Composite shadow behind text
		float totalAlpha = fillAlpha + outlineAlpha;
		if (shadowAlpha > 0.0 && totalAlpha < 1.0) {
			finalColor = mix(frag.outerCol.rgb, finalColor, totalAlpha);
		}
	}

	// Combine all alpha channels
	float totalAlpha = clamp(fillAlpha + outlineAlpha + glowAlpha + shadowAlpha, 0.0, 1.0);

	// Apply scissor mask
	float scissor = scissorMask(inPosition);

	// Final output
	outColor = vec4(finalColor, frag.innerCol.a * totalAlpha * scissor);
}
