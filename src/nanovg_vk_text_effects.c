// nanovg_vk_text_effects.c - Text Effects Implementation

#include "nanovg_vk_text_effects.h"
#include "nanovg.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Create effect context
VKNVGtextEffect* vknvg__createTextEffect(void)
{
	VKNVGtextEffect* effect = (VKNVGtextEffect*)malloc(sizeof(VKNVGtextEffect));
	if (!effect) return NULL;

	memset(effect, 0, sizeof(VKNVGtextEffect));
	vknvg__resetTextEffect(effect);

	return effect;
}

// Destroy effect context
void vknvg__destroyTextEffect(VKNVGtextEffect* effect)
{
	if (effect) {
		free(effect);
	}
}

// Reset all effects
void vknvg__resetTextEffect(VKNVGtextEffect* effect)
{
	if (!effect) return;

	memset(effect, 0, sizeof(VKNVGtextEffect));

	// Set default values
	effect->glow.strength = 1.0f;
	effect->animation.speed = 1.0f;
	effect->animation.intensity = 1.0f;
	effect->animation.frequency = 1.0f;
	effect->gradient.radius = 100.0f;
}

// Outline functions
void vknvg__addOutline(VKNVGtextEffect* effect, float width, NVGcolor color)
{
	if (!effect || effect->outlineCount >= 4) return;

	int layer = effect->outlineCount;
	effect->outlines[layer].width = width;
	effect->outlines[layer].color = color;
	effect->outlines[layer].enabled = 1;
	effect->outlineCount++;
	effect->effectFlags |= VKNVG_EFFECT_OUTLINE;
}

void vknvg__setOutlineLayer(VKNVGtextEffect* effect, int layer,
                            float width, NVGcolor color)
{
	if (!effect || layer < 0 || layer >= 4) return;

	effect->outlines[layer].width = width;
	effect->outlines[layer].color = color;
	effect->outlines[layer].enabled = 1;

	// Update count if this extends the active layers
	if (layer >= (int)effect->outlineCount) {
		effect->outlineCount = layer + 1;
	}

	effect->effectFlags |= VKNVG_EFFECT_OUTLINE;
}

void vknvg__clearOutlines(VKNVGtextEffect* effect)
{
	if (!effect) return;

	for (int i = 0; i < 4; i++) {
		memset(&effect->outlines[i], 0, sizeof(VKNVGoutlineLayer));
	}
	effect->outlineCount = 0;
	effect->effectFlags &= ~VKNVG_EFFECT_OUTLINE;
}

void vknvg__removeOutlineLayer(VKNVGtextEffect* effect, int layer)
{
	if (!effect || layer < 0 || layer >= 4) return;

	effect->outlines[layer].enabled = 0;

	// Recalculate outline count
	effect->outlineCount = 0;
	for (int i = 0; i < 4; i++) {
		if (effect->outlines[i].enabled) {
			effect->outlineCount = i + 1;
		}
	}

	if (effect->outlineCount == 0) {
		effect->effectFlags &= ~VKNVG_EFFECT_OUTLINE;
	}
}

// Glow functions
void vknvg__setGlow(VKNVGtextEffect* effect, float radius,
                    NVGcolor color, float strength)
{
	if (!effect) return;

	effect->glow.radius = radius;
	effect->glow.color = color;
	effect->glow.strength = strength;
	effect->glow.enabled = 1;
	effect->effectFlags |= VKNVG_EFFECT_GLOW;
}

void vknvg__enableGlow(VKNVGtextEffect* effect, int enabled)
{
	if (!effect) return;

	effect->glow.enabled = enabled ? 1 : 0;
	if (enabled) {
		effect->effectFlags |= VKNVG_EFFECT_GLOW;
	} else {
		effect->effectFlags &= ~VKNVG_EFFECT_GLOW;
	}
}

// Shadow functions
void vknvg__setShadow(VKNVGtextEffect* effect, float offsetX, float offsetY,
                      float blur, NVGcolor color)
{
	if (!effect) return;

	effect->shadow.offsetX = offsetX;
	effect->shadow.offsetY = offsetY;
	effect->shadow.blur = blur;
	effect->shadow.color = color;
	effect->shadow.enabled = 1;
	effect->effectFlags |= VKNVG_EFFECT_SHADOW;
}

void vknvg__enableShadow(VKNVGtextEffect* effect, int enabled)
{
	if (!effect) return;

	effect->shadow.enabled = enabled ? 1 : 0;
	if (enabled) {
		effect->effectFlags |= VKNVG_EFFECT_SHADOW;
	} else {
		effect->effectFlags &= ~VKNVG_EFFECT_SHADOW;
	}
}

// Gradient functions
void vknvg__setLinearGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float angle)
{
	if (!effect) return;

	effect->gradient.start = start;
	effect->gradient.end = end;
	effect->gradient.angle = angle;
	effect->gradient.type = VKNVG_GRADIENT_LINEAR;
	effect->gradient.enabled = 1;
	effect->effectFlags |= VKNVG_EFFECT_GRADIENT;
}

void vknvg__setRadialGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float radius)
{
	if (!effect) return;

	effect->gradient.start = start;
	effect->gradient.end = end;
	effect->gradient.radius = radius;
	effect->gradient.type = VKNVG_GRADIENT_RADIAL;
	effect->gradient.enabled = 1;
	effect->effectFlags |= VKNVG_EFFECT_GRADIENT;
}

void vknvg__enableGradient(VKNVGtextEffect* effect, int enabled)
{
	if (!effect) return;

	effect->gradient.enabled = enabled ? 1 : 0;
	if (enabled) {
		effect->effectFlags |= VKNVG_EFFECT_GRADIENT;
	} else {
		effect->effectFlags &= ~VKNVG_EFFECT_GRADIENT;
	}
}

// Animation functions
void vknvg__setAnimation(VKNVGtextEffect* effect, VKNVGanimationType type,
                         float speed, float intensity)
{
	if (!effect) return;

	effect->animation.type = type;
	effect->animation.speed = speed;
	effect->animation.intensity = intensity;
	effect->animation.enabled = (type != VKNVG_ANIM_NONE) ? 1 : 0;

	if (effect->animation.enabled) {
		effect->effectFlags |= VKNVG_EFFECT_ANIMATE;
	} else {
		effect->effectFlags &= ~VKNVG_EFFECT_ANIMATE;
	}
}

void vknvg__setAnimationFrequency(VKNVGtextEffect* effect, float frequency)
{
	if (!effect) return;
	effect->animation.frequency = frequency;
}

void vknvg__updateAnimation(VKNVGtextEffect* effect, float deltaTime)
{
	if (!effect || !effect->animation.enabled) return;
	effect->animation.time += deltaTime * effect->animation.speed;
}

void vknvg__enableAnimation(VKNVGtextEffect* effect, int enabled)
{
	if (!effect) return;

	effect->animation.enabled = enabled ? 1 : 0;
	if (enabled) {
		effect->effectFlags |= VKNVG_EFFECT_ANIMATE;
	} else {
		effect->effectFlags &= ~VKNVG_EFFECT_ANIMATE;
	}
}

// Convert effect to shader uniforms
void vknvg__effectToUniforms(const VKNVGtextEffect* effect,
                             VKNVGtextEffectUniforms* uniforms)
{
	if (!effect || !uniforms) return;

	memset(uniforms, 0, sizeof(VKNVGtextEffectUniforms));

	// Outlines
	uniforms->outlineCount = effect->outlineCount;
	for (int i = 0; i < 4; i++) {
		uniforms->outlineWidths[i] = effect->outlines[i].width;
		uniforms->outlineColors[i * 4 + 0] = effect->outlines[i].color.r;
		uniforms->outlineColors[i * 4 + 1] = effect->outlines[i].color.g;
		uniforms->outlineColors[i * 4 + 2] = effect->outlines[i].color.b;
		uniforms->outlineColors[i * 4 + 3] = effect->outlines[i].color.a;
	}

	// Glow
	uniforms->glowRadius = effect->glow.radius;
	uniforms->glowColor[0] = effect->glow.color.r;
	uniforms->glowColor[1] = effect->glow.color.g;
	uniforms->glowColor[2] = effect->glow.color.b;
	uniforms->glowColor[3] = effect->glow.color.a;
	uniforms->glowStrength = effect->glow.strength;

	// Shadow
	uniforms->shadowOffset[0] = effect->shadow.offsetX;
	uniforms->shadowOffset[1] = effect->shadow.offsetY;
	uniforms->shadowBlur = effect->shadow.blur;
	uniforms->shadowColor[0] = effect->shadow.color.r;
	uniforms->shadowColor[1] = effect->shadow.color.g;
	uniforms->shadowColor[2] = effect->shadow.color.b;
	uniforms->shadowColor[3] = effect->shadow.color.a;

	// Gradient
	uniforms->gradientStart[0] = effect->gradient.start.r;
	uniforms->gradientStart[1] = effect->gradient.start.g;
	uniforms->gradientStart[2] = effect->gradient.start.b;
	uniforms->gradientStart[3] = effect->gradient.start.a;
	uniforms->gradientEnd[0] = effect->gradient.end.r;
	uniforms->gradientEnd[1] = effect->gradient.end.g;
	uniforms->gradientEnd[2] = effect->gradient.end.b;
	uniforms->gradientEnd[3] = effect->gradient.end.a;
	uniforms->gradientAngle = effect->gradient.angle;
	uniforms->gradientRadius = effect->gradient.radius;
	uniforms->gradientType = effect->gradient.type;

	// Animation
	uniforms->animationTime = effect->animation.time;
	uniforms->animationType = effect->animation.type;
	uniforms->animationSpeed = effect->animation.speed;
	uniforms->animationIntensity = effect->animation.intensity;
	uniforms->animationFrequency = effect->animation.frequency;

	// Flags
	uniforms->effectFlags = effect->effectFlags;
}

// Utility functions
int vknvg__hasActiveEffects(const VKNVGtextEffect* effect)
{
	return effect && (effect->effectFlags != VKNVG_EFFECT_NONE);
}

uint32_t vknvg__getActiveEffectFlags(const VKNVGtextEffect* effect)
{
	return effect ? effect->effectFlags : VKNVG_EFFECT_NONE;
}
