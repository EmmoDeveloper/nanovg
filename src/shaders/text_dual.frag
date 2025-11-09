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
	int texType;		// 1=SDF, 2=MSDF, 3=RGBA
	int type;
} frag;

// Dual-mode texture samplers
layout(set = 0, binding = 1) uniform sampler2D sdfAtlas;    // SDF/MSDF atlas (grayscale or RGB)
layout(set = 0, binding = 2) uniform sampler2D colorAtlas;  // RGBA color emoji atlas

// MSDF helper functions
float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(vec2 texCoord) {
	// Calculate the distance range in screen pixels
	vec2 unitRange = vec2(4.0) / vec2(textureSize(sdfAtlas, 0));
	vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

// Scissoring mask
float scissorMask(vec2 p) {
	vec2 sc = (frag.scissorMat * vec3(p, 1.0)).xy;
	sc = (vec2(0.5) - abs(sc) + frag.scissorExt) * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

void main() {
	// Sample color atlas first to detect emoji
	vec4 colorSample = texture(colorAtlas, inTexCoord);

	// Detect which mode to use based on color atlas alpha
	// If color atlas has valid data (alpha > threshold), use color emoji mode
	float colorThreshold = 0.01;
	bool useColorMode = (colorSample.a > colorThreshold);

	vec4 finalColor;

	if (useColorMode) {
		// Color emoji mode: use RGBA color from color atlas
		// Apply tint alpha for opacity control
		finalColor = vec4(colorSample.rgb, colorSample.a * frag.innerCol.a);
	} else {
		// Text mode: either SDF or MSDF
		// Sample the atlas as RGB (works for both SDF and MSDF)
		vec3 atlasRGB = texture(sdfAtlas, inTexCoord).rgb;

		float distance;
		float edgeWidth;

		// Determine rendering mode based on texType
		if (frag.texType == 2) {
			// MSDF mode: multi-channel signed distance field
			distance = median(atlasRGB.r, atlasRGB.g, atlasRGB.b);

			// MSDF uses screen-space distance calculation
			float screenPxDistance = screenPxRange(inTexCoord) * (distance - 0.5);
			edgeWidth = 0.5;  // MSDF handles antialiasing differently

			// Convert back to 0-1 range for opacity calculation
			distance = clamp(screenPxDistance + 0.5, 0.0, 1.0);
		} else {
			// SDF mode: single-channel signed distance field
			distance = atlasRGB.r;

			// Calculate edge width for antialiasing
			edgeWidth = fwidth(distance) * 0.7;
		}

		// Base text alpha (main fill)
		float fillAlpha;
		if (frag.texType == 2) {
			// MSDF: distance is already antialiased
			fillAlpha = distance;
		} else {
			// SDF: apply smoothstep antialiasing
			fillAlpha = smoothstep(0.5 - edgeWidth, 0.5 + edgeWidth, distance);
		}

		// Outline effect (controlled by strokeMult)
		float outlineWidth = frag.strokeMult * 0.1;
		float outlineAlpha = 0.0;
		vec3 textColor = frag.innerCol.rgb;

		if (outlineWidth > 0.0) {
			float outerEdge = 0.5 - outlineWidth;

			if (frag.texType == 2) {
				// MSDF outline
				float screenPxDistance = screenPxRange(inTexCoord) * (median(atlasRGB.r, atlasRGB.g, atlasRGB.b) - 0.5);
				float outlineDistance = clamp(screenPxDistance + 0.5 - outlineWidth, 0.0, 1.0);
				outlineAlpha = outlineDistance * (1.0 - fillAlpha);
			} else {
				// SDF outline
				outlineAlpha = smoothstep(outerEdge - edgeWidth, outerEdge + edgeWidth, distance);
				outlineAlpha *= (1.0 - fillAlpha);
			}

			textColor = mix(frag.outerCol.rgb, frag.innerCol.rgb, fillAlpha / max(fillAlpha + outlineAlpha, 0.001));
		}

		float totalAlpha = clamp(fillAlpha + outlineAlpha, 0.0, 1.0);
		finalColor = vec4(textColor, frag.innerCol.a * totalAlpha);
	}

	// Apply scissor mask
	float scissor = scissorMask(inPosition);
	outColor = vec4(finalColor.rgb, finalColor.a * scissor);
}
