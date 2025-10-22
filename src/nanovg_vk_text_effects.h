// nanovg_vk_text_effects.h - Advanced Text Effects for NanoVG Vulkan
//
// Phase 5: GPU-accelerated text effects using extended SDF shaders
//
// Features:
// - Multi-layer outlines with independent colors
// - Glow effects with customizable radius and color
// - Drop shadows with blur
// - Linear and radial gradients
// - Animated effects (shimmer, pulse, wave)

#ifndef NANOVG_VK_TEXT_EFFECTS_H
#define NANOVG_VK_TEXT_EFFECTS_H

#include <stdint.h>
#include "nanovg.h"

// Forward declarations
typedef struct VKNVGtextEffect VKNVGtextEffect;

// Effect flags (bitmask)
#define VKNVG_EFFECT_NONE       0
#define VKNVG_EFFECT_OUTLINE    (1 << 0)
#define VKNVG_EFFECT_GLOW       (1 << 1)
#define VKNVG_EFFECT_SHADOW     (1 << 2)
#define VKNVG_EFFECT_GRADIENT   (1 << 3)
#define VKNVG_EFFECT_ANIMATE    (1 << 4)

// Animation types
typedef enum VKNVGanimationType {
	VKNVG_ANIM_NONE = 0,
	VKNVG_ANIM_SHIMMER,      // Horizontal shimmer effect
	VKNVG_ANIM_PULSE,        // Pulsing glow
	VKNVG_ANIM_WAVE,         // Wave distortion
	VKNVG_ANIM_COLOR_CYCLE,  // Color cycling
	VKNVG_ANIM_RAINBOW,      // Rainbow gradient
} VKNVGanimationType;

// Gradient types
typedef enum VKNVGgradientType {
	VKNVG_GRADIENT_LINEAR = 0,
	VKNVG_GRADIENT_RADIAL = 1,
} VKNVGgradientType;

// Outline layer configuration
typedef struct VKNVGoutlineLayer {
	float width;             // Width in pixels
	NVGcolor color;          // RGBA color
	uint8_t enabled;         // Is this layer active?
	uint8_t padding[3];
} VKNVGoutlineLayer;

// Glow configuration
typedef struct VKNVGglowConfig {
	float radius;            // Glow spread radius
	NVGcolor color;          // Glow color
	float strength;          // Intensity (0-1)
	uint8_t enabled;
	uint8_t padding[3];
} VKNVGglowConfig;

// Shadow configuration
typedef struct VKNVGshadowConfig {
	float offsetX;           // Shadow offset X
	float offsetY;           // Shadow offset Y
	float blur;              // Shadow blur radius
	NVGcolor color;          // Shadow color
	uint8_t enabled;
	uint8_t padding[3];
} VKNVGshadowConfig;

// Gradient configuration
typedef struct VKNVGgradientConfig {
	NVGcolor start;          // Start color
	NVGcolor end;            // End color
	float angle;             // Angle in radians (for linear)
	float radius;            // Radius (for radial)
	VKNVGgradientType type;  // LINEAR or RADIAL
	uint8_t enabled;
	uint8_t padding[3];
} VKNVGgradientConfig;

// Animation configuration
typedef struct VKNVGanimationConfig {
	VKNVGanimationType type; // Animation type
	float time;              // Current time in seconds
	float speed;             // Speed multiplier
	float intensity;         // Effect intensity (0-1)
	float frequency;         // Oscillation frequency
	uint8_t enabled;
	uint8_t padding[3];
} VKNVGanimationConfig;

// Text effect context
struct VKNVGtextEffect {
	// Outline layers (up to 4)
	VKNVGoutlineLayer outlines[4];
	uint32_t outlineCount;

	// Glow effect
	VKNVGglowConfig glow;

	// Shadow effect
	VKNVGshadowConfig shadow;

	// Gradient fill
	VKNVGgradientConfig gradient;

	// Animation
	VKNVGanimationConfig animation;

	// Effect flags
	uint32_t effectFlags;
};

// Shader uniform structure (matches GLSL layout)
typedef struct VKNVGtextEffectUniforms {
	// Outlines (4 layers max)
	float outlineWidths[4];
	float outlineColors[16];  // 4 colors × RGBA
	int outlineCount;
	int _pad0[3];

	// Glow
	float glowRadius;
	float glowColor[4];
	float glowStrength;
	int _pad1[3];

	// Shadow
	float shadowOffset[2];
	float shadowBlur;
	float shadowColor[4];
	int _pad2;

	// Gradient
	float gradientStart[4];
	float gradientEnd[4];
	float gradientAngle;
	float gradientRadius;
	int gradientType;
	int _pad3;

	// Animation
	float animationTime;
	int animationType;
	float animationSpeed;
	float animationIntensity;
	float animationFrequency;
	int _pad4[3];

	// Flags
	int effectFlags;
	int _pad5[3];
} VKNVGtextEffectUniforms;

// API Functions

// Create/destroy effect context
VKNVGtextEffect* vknvg__createTextEffect(void);
void vknvg__destroyTextEffect(VKNVGtextEffect* effect);

// Reset all effects to default
void vknvg__resetTextEffect(VKNVGtextEffect* effect);

// Outline configuration
void vknvg__addOutline(VKNVGtextEffect* effect, float width, NVGcolor color);
void vknvg__setOutlineLayer(VKNVGtextEffect* effect, int layer,
                            float width, NVGcolor color);
void vknvg__clearOutlines(VKNVGtextEffect* effect);
void vknvg__removeOutlineLayer(VKNVGtextEffect* effect, int layer);

// Glow configuration
void vknvg__setGlow(VKNVGtextEffect* effect, float radius,
                    NVGcolor color, float strength);
void vknvg__enableGlow(VKNVGtextEffect* effect, int enabled);

// Shadow configuration
void vknvg__setShadow(VKNVGtextEffect* effect, float offsetX, float offsetY,
                      float blur, NVGcolor color);
void vknvg__enableShadow(VKNVGtextEffect* effect, int enabled);

// Gradient configuration
void vknvg__setLinearGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float angle);
void vknvg__setRadialGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float radius);
void vknvg__enableGradient(VKNVGtextEffect* effect, int enabled);

// Animation configuration
void vknvg__setAnimation(VKNVGtextEffect* effect, VKNVGanimationType type,
                         float speed, float intensity);
void vknvg__setAnimationFrequency(VKNVGtextEffect* effect, float frequency);
void vknvg__updateAnimation(VKNVGtextEffect* effect, float deltaTime);
void vknvg__enableAnimation(VKNVGtextEffect* effect, int enabled);

// Uniform conversion
void vknvg__effectToUniforms(const VKNVGtextEffect* effect,
                             VKNVGtextEffectUniforms* uniforms);

// Utility functions
int vknvg__hasActiveEffects(const VKNVGtextEffect* effect);
uint32_t vknvg__getActiveEffectFlags(const VKNVGtextEffect* effect);

#endif // NANOVG_VK_TEXT_EFFECTS_H
