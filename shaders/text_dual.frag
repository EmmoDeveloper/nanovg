#version 450

// Fragment input
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec2 inPosition;
layout(location = 2) flat in uint inRenderMode;  // 0 = SDF, 1 = Color Emoji

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

// SDF texture sampler (binding 1)
layout(set = 0, binding = 1) uniform sampler2D sdfTex;

// Color atlas texture sampler (binding 2)
layout(set = 0, binding = 2) uniform sampler2D colorTex;

// Scissoring mask
float scissorMask(vec2 p) {
	vec2 sc = (frag.scissorMat * vec3(p, 1.0)).xy;
	sc = (vec2(0.5) - abs(sc) + frag.scissorExt) * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	vec4 result;

	if (inRenderMode == 0u) {
		// SDF rendering mode
		vec2 pt = (frag.paintMat * vec3(inPosition, 1.0)).xy / frag.extent;
		float distance = texture(sdfTex, pt).r;
		float edgeWidth = fwidth(distance) * 0.7;
		float fillAlpha = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, distance);

		// Outline effect
		float outlineWidth = frag.strokeMult * 0.1;
		float outlineAlpha = 0.0;
		vec3 finalColor = frag.innerCol.rgb;

		if (outlineWidth > 0.0) {
			float outerEdge = 0.5 - outlineWidth;
			outlineAlpha = smoothstep(outerEdge - edgeWidth, outerEdge + edgeWidth, distance);
			outlineAlpha *= (1.0 - fillAlpha);
			finalColor = mix(frag.outerCol.rgb, frag.innerCol.rgb, fillAlpha / max(fillAlpha + outlineAlpha, 0.001));
		}

		// Glow effect
		float glowAlpha = 0.0;
		if (frag.feather > 1.0) {
			float glowWidth = (frag.feather - 1.0) * 0.2;
			float glowEdge = 0.5 - outlineWidth - glowWidth;
			glowAlpha = smoothstep(glowEdge - edgeWidth * 2.0, glowEdge, distance);
			glowAlpha *= (1.0 - fillAlpha - outlineAlpha) * 0.5;
			finalColor = mix(finalColor, frag.outerCol.rgb, glowAlpha * 0.3);
		}

		// Shadow effect
		float shadowAlpha = 0.0;
		if (frag.radius > 0.0) {
			vec2 shadowOffset = vec2(frag.radius * 0.01, frag.radius * 0.01);
			float shadowDist = texture(sdfTex, pt + shadowOffset).r;
			shadowAlpha = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, shadowDist);
			shadowAlpha *= (1.0 - fillAlpha - outlineAlpha) * frag.outerCol.a;

			float totalAlpha = fillAlpha + outlineAlpha;
			if (shadowAlpha > 0.0 && totalAlpha < 1.0) {
				finalColor = mix(frag.outerCol.rgb, finalColor, totalAlpha);
			}
		}

		float totalAlpha = clamp(fillAlpha + outlineAlpha + glowAlpha + shadowAlpha, 0.0, 1.0);
		result = vec4(finalColor, frag.innerCol.a * totalAlpha);
	} else {
		// Color emoji rendering mode
		vec4 texColor = texture(colorTex, inTexCoord);
		result = vec4(texColor.rgb, texColor.a * frag.innerCol.a);
	}

	// Apply scissor mask
	float scissor = scissorMask(inPosition);
	outColor = vec4(result.rgb, result.a * scissor);
}
