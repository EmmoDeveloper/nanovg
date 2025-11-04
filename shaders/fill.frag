#version 450
#extension GL_GOOGLE_include_directive : require

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

// Texture sampler
layout(set = 0, binding = 1) uniform sampler2D tex;

// Color space conversion support
#include "color_space_ubo.glsl"

// Signed distance to rounded rectangle (optimized)
float sdroundrect(vec2 pt, vec2 ext, float rad) {
	vec2 d = abs(pt) - (ext - rad);
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

// Scissoring mask (optimized)
float scissorMask(vec2 p) {
	vec2 sc = (frag.scissorMat * vec3(p, 1.0)).xy;
	sc = (vec2(0.5) - abs(sc) + frag.scissorExt) * frag.scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

// Stroke antialiasing mask (optimized)
float strokeMask() {
	float tx = abs(inTexCoord.x * 2.0 - 1.0);
	return min(1.0, (1.0 - tx) * frag.strokeMult) * min(1.0, inTexCoord.y);
}

void main() {
	// Early discard for stroke threshold
	float strokeAlpha = strokeMask();
	if (strokeAlpha < frag.strokeThr) {
		discard;
	}

	// Calculate scissor mask
	float scissor = scissorMask(inPosition);
	float alphaMask = strokeAlpha * scissor;

	vec4 result;

	if (frag.type == 0) {
		// Gradient rendering (optimized)
		vec2 pt = (frag.paintMat * vec3(inPosition, 1.0)).xy;
		float d = (sdroundrect(pt, frag.extent, frag.radius) + frag.feather * 0.5) / frag.feather;
		d = clamp(d, 0.0, 1.0);
		result = mix(frag.innerCol, frag.outerCol, d) * alphaMask;
	} else if (frag.type == 1) {
		// Image/texture rendering (optimized)
		vec2 pt = (frag.paintMat * vec3(inPosition, 1.0)).xy / frag.extent;
		vec4 color = texture(tex, pt);

		// Handle different texture types
		if (frag.texType == 1) {
			// Premultiplied alpha - already premultiplied, just mask alpha
			color.a *= color.a;
		} else if (frag.texType == 2) {
			// Alpha only (for fonts)
			color = vec4(frag.innerCol.rgb, frag.innerCol.a * color.r);
		} else {
			// Standard RGBA
			color *= frag.innerCol;
		}

		result = color * alphaMask;
	} else if (frag.type == 2) {
		// Stencil fill (solid color) - optimized
		result = frag.innerCol * alphaMask;
	} else {
		// Fallback
		result = vec4(1.0, 0.0, 1.0, 1.0);
	}

	// Apply color space conversion (sRGB → target color space)
	outColor = applyColorSpace(result);
}
