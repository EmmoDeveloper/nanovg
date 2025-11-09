#version 450

// Extended SDF Text Shader with Effects
// Phase 5: Advanced Text Effects

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D tex;

// Text effect uniforms
layout(std140, binding = 1) uniform TextEffects {
	// Outlines (4 layers)
	vec4 outlineWidths;          // x,y,z,w for layers 0-3
	vec4 outlineColors[4];       // RGBA for each layer
	int outlineCount;
	int _pad0[3];

	// Glow
	float glowRadius;
	vec4 glowColor;
	float glowStrength;
	int _pad1[3];

	// Shadow
	vec2 shadowOffset;
	float shadowBlur;
	vec4 shadowColor;
	int _pad2;

	// Gradient
	vec4 gradientStart;
	vec4 gradientEnd;
	float gradientAngle;
	float gradientRadius;
	int gradientType;            // 0=LINEAR, 1=RADIAL
	int _pad3;

	// Animation
	float animationTime;
	int animationType;           // 0=NONE, 1=SHIMMER, 2=PULSE, 3=WAVE, etc.
	float animationSpeed;
	float animationIntensity;
	float animationFrequency;
	int _pad4[3];

	// Flags
	int effectFlags;             // Bitmask of active effects
	int _pad5[3];
} effects;

// Effect flags
const int EFFECT_OUTLINE  = 1;
const int EFFECT_GLOW     = 2;
const int EFFECT_SHADOW   = 4;
const int EFFECT_GRADIENT = 8;
const int EFFECT_ANIMATE  = 16;

// Animation types
const int ANIM_NONE        = 0;
const int ANIM_SHIMMER     = 1;
const int ANIM_PULSE       = 2;
const int ANIM_WAVE        = 3;
const int ANIM_COLOR_CYCLE = 4;
const int ANIM_RAINBOW     = 5;

// Helper functions

// Smooth SDF edge
float sdfSmooth(float dist, float width) {
	float delta = fwidth(dist);
	return smoothstep(0.5 - width - delta, 0.5 - width + delta, dist);
}

// Calculate gradient color
vec4 applyGradient(vec2 pos) {
	float t;

	if (effects.gradientType == 0) {
		// Linear gradient
		vec2 gradVec = vec2(cos(effects.gradientAngle), sin(effects.gradientAngle));
		t = dot(pos, gradVec);
		t = t * 0.5 + 0.5;  // Normalize to 0-1
	} else {
		// Radial gradient
		float dist = length(pos);
		t = clamp(dist / effects.gradientRadius, 0.0, 1.0);
	}

	return mix(effects.gradientStart, effects.gradientEnd, t);
}

// Apply animation effects
vec2 applyAnimation(vec2 pos, float dist) {
	if ((effects.effectFlags & EFFECT_ANIMATE) == 0) {
		return pos;
	}

	if (effects.animationType == ANIM_WAVE) {
		// Wave distortion
		float wave = sin(effects.animationTime * effects.animationSpeed + pos.y * effects.animationFrequency);
		pos.x += wave * effects.animationIntensity * 10.0;
	}

	return pos;
}

float getAnimationModulator() {
	if ((effects.effectFlags & EFFECT_ANIMATE) == 0) {
		return 1.0;
	}

	if (effects.animationType == ANIM_SHIMMER) {
		// Horizontal shimmer
		return 0.8 + 0.2 * sin(effects.animationTime * effects.animationSpeed + fpos.x * effects.animationFrequency);
	} else if (effects.animationType == ANIM_PULSE) {
		// Pulsing
		return 0.5 + 0.5 * sin(effects.animationTime * effects.animationSpeed);
	}

	return 1.0;
}

vec4 getAnimatedColor(vec4 baseColor) {
	if ((effects.effectFlags & EFFECT_ANIMATE) == 0) {
		return baseColor;
	}

	if (effects.animationType == ANIM_COLOR_CYCLE) {
		// Color cycling
		float phase = effects.animationTime * effects.animationSpeed;
		vec3 cycle = vec3(
			0.5 + 0.5 * sin(phase),
			0.5 + 0.5 * sin(phase + 2.094),
			0.5 + 0.5 * sin(phase + 4.189)
		);
		baseColor.rgb = mix(baseColor.rgb, cycle, effects.animationIntensity);
	} else if (effects.animationType == ANIM_RAINBOW) {
		// Rainbow effect
		float hue = mod(fpos.x * 0.01 + effects.animationTime * effects.animationSpeed, 1.0);
		vec3 rgb = vec3(
			abs(hue * 6.0 - 3.0) - 1.0,
			2.0 - abs(hue * 6.0 - 2.0),
			2.0 - abs(hue * 6.0 - 4.0)
		);
		rgb = clamp(rgb, 0.0, 1.0);
		baseColor.rgb = mix(baseColor.rgb, rgb, effects.animationIntensity);
	}

	return baseColor;
}

void main() {
	// Sample distance field
	float dist = texture(tex, ftcoord).r;

	// Apply animation (wave distortion)
	vec2 animPos = applyAnimation(fpos, dist);

	// Get animation modulator
	float animMod = getAnimationModulator();

	// Start with transparent
	vec4 color = vec4(0.0);

	// Apply shadow (if enabled)
	if ((effects.effectFlags & EFFECT_SHADOW) != 0) {
		vec2 shadowCoord = ftcoord - effects.shadowOffset;
		float shadowDist = texture(tex, shadowCoord).r;
		float shadowAlpha = sdfSmooth(shadowDist, effects.shadowBlur);
		color = mix(effects.shadowColor, color, shadowAlpha);
	}

	// Apply outlines (from outermost to innermost)
	if ((effects.effectFlags & EFFECT_OUTLINE) != 0) {
		for (int i = effects.outlineCount - 1; i >= 0; i--) {
			float width = effects.outlineWidths[i];
			vec4 outlineColor = effects.outlineColors[i];
			float alpha = sdfSmooth(dist, width);
			color = mix(outlineColor, color, alpha);
		}
	}

	// Apply base text (fill)
	float baseAlpha = sdfSmooth(dist, 0.0);
	vec4 fillColor = vec4(1.0);

	// Apply gradient (if enabled)
	if ((effects.effectFlags & EFFECT_GRADIENT) != 0) {
		fillColor = applyGradient(fpos);
	}

	color = mix(fillColor, color, baseAlpha);

	// Apply glow (if enabled)
	if ((effects.effectFlags & EFFECT_GLOW) != 0) {
		float glowDist = 1.0 - smoothstep(0.5 - effects.glowRadius, 0.5, dist);
		vec4 glow = effects.glowColor * glowDist * effects.glowStrength * animMod;
		color.rgb += glow.rgb * glow.a;
	}

	// Apply animation modulation
	color *= animMod;

	// Apply animated color effects
	color = getAnimatedColor(color);

	// Output final color
	outColor = color;
}
