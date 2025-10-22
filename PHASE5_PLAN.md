# Phase 5: Advanced Text Effects - Implementation Plan

## Overview

Implement GPU-accelerated visual effects for text rendering using extended SDF shaders.

## Goals

1. **Multi-layer SDF Effects**: Outlines, glows, shadows with customizable parameters
2. **Gradient Fills**: Linear and radial gradients on text
3. **Animated Effects**: Time-based effects (shimmer, pulse, wave, color cycling)

## Architecture

### Effect Parameter Structure

```c
// Text effect parameters (passed via uniforms)
typedef struct VKNVGtextEffect {
	// Outline layers (up to 4)
	float outlineWidth[4];      // Width in pixels
	NVGcolor outlineColor[4];   // RGBA colors
	int outlineCount;           // Number of active outlines

	// Glow effect
	float glowRadius;           // Glow spread radius
	NVGcolor glowColor;         // Glow color
	float glowStrength;         // Intensity (0-1)

	// Shadow
	float shadowOffsetX;        // Shadow offset X
	float shadowOffsetY;        // Shadow offset Y
	float shadowBlur;           // Shadow blur radius
	NVGcolor shadowColor;       // Shadow color

	// Gradient
	NVGcolor gradientStart;     // Start color
	NVGcolor gradientEnd;       // End color
	float gradientAngle;        // Angle in radians
	int gradientType;           // LINEAR or RADIAL

	// Animation
	float animationTime;        // Time in seconds
	int effectType;             // NONE, SHIMMER, PULSE, WAVE, etc.
	float animationSpeed;       // Speed multiplier
	float animationIntensity;   // Effect intensity

	// General
	int effectFlags;            // Bitmask of enabled effects
} VKNVGtextEffect;
```

### Effect Flags

```c
#define VKNVG_EFFECT_OUTLINE    (1 << 0)
#define VKNVG_EFFECT_GLOW       (1 << 1)
#define VKNVG_EFFECT_SHADOW     (1 << 2)
#define VKNVG_EFFECT_GRADIENT   (1 << 3)
#define VKNVG_EFFECT_ANIMATE    (1 << 4)
```

### Animation Types

```c
typedef enum VKNVGanimationType {
	VKNVG_ANIM_NONE = 0,
	VKNVG_ANIM_SHIMMER,      // Horizontal shimmer effect
	VKNVG_ANIM_PULSE,        // Pulsing glow
	VKNVG_ANIM_WAVE,         // Wave distortion
	VKNVG_ANIM_COLOR_CYCLE,  // Color cycling
	VKNVG_ANIM_RAINBOW,      // Rainbow gradient
} VKNVGanimationType;
```

## Implementation Plan

### Phase 5.1: SDF Shader Extensions (Week 1-2)

**Files to Create:**
- `shaders/sdf_effects.frag` - Extended SDF shader with effects

**Features:**
- Multi-layer outline rendering
- Smooth distance field calculations
- Per-layer color and width control
- Proper alpha blending between layers

**Shader Structure:**
```glsl
// Calculate base SDF distance
float dist = texture(fontAtlas, texCoord).r;

// Apply multiple outlines
vec4 color = vec4(0.0);
for (int i = 0; i < outlineCount; i++) {
    float edge = smoothstep(0.5 - outlineWidth[i], 0.5 + fwidth(dist), dist);
    color = mix(outlineColor[i], color, edge);
}

// Apply glow
if (enableGlow) {
    float glow = smoothstep(0.5, 0.5 - glowRadius, dist);
    color.rgb = mix(glowColor.rgb, color.rgb, glow);
}
```

### Phase 5.2: Gradient System (Week 2-3)

**Files to Create:**
- `src/nanovg_vk_text_gradient.h` - Gradient calculation helpers

**Features:**
- Linear gradient mapping to text bounds
- Radial gradient from center point
- Per-glyph or per-text gradient modes
- Gradient stop interpolation

**Algorithm:**
```c
// Linear gradient
vec2 gradientVec = vec2(cos(angle), sin(angle));
float t = dot(glyphPos, gradientVec);
vec4 gradColor = mix(gradStart, gradEnd, t);

// Radial gradient
float dist = length(glyphPos - gradCenter);
float t = clamp(dist / gradRadius, 0.0, 1.0);
vec4 gradColor = mix(gradStart, gradEnd, t);
```

### Phase 5.3: Animation System (Week 3-4)

**Files to Create:**
- `src/nanovg_vk_text_animation.h` - Animation time tracking
- `src/nanovg_vk_text_animation.c` - Animation implementation

**Features:**
- Time uniform passed to shaders
- Animation state management
- Easing functions (linear, ease-in-out, bounce)
- Per-effect animation parameters

**Shader Effects:**
```glsl
// Shimmer
float shimmer = sin(time * speed + glyphPos.x * frequency);
color.rgb += shimmer * intensity;

// Pulse
float pulse = 0.5 + 0.5 * sin(time * speed);
glowStrength *= pulse;

// Wave
float wave = sin(time * speed + glyphPos.y * frequency);
glyphPos.x += wave * intensity;
```

### Phase 5.4: Effect API (Week 4-5)

**Files to Create:**
- `src/nanovg_vk_text_effects.h` - Public API
- `src/nanovg_vk_text_effects.c` - Implementation

**API Functions:**
```c
// Create effect context
VKNVGtextEffect* vknvg__createTextEffect();
void vknvg__destroyTextEffect(VKNVGtextEffect* effect);

// Outline configuration
void vknvg__setOutline(VKNVGtextEffect* effect, int layer,
                       float width, NVGcolor color);
void vknvg__clearOutlines(VKNVGtextEffect* effect);

// Glow configuration
void vknvg__setGlow(VKNVGtextEffect* effect, float radius,
                    NVGcolor color, float strength);

// Shadow configuration
void vknvg__setShadow(VKNVGtextEffect* effect, float offsetX, float offsetY,
                      float blur, NVGcolor color);

// Gradient configuration
void vknvg__setLinearGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float angle);
void vknvg__setRadialGradient(VKNVGtextEffect* effect,
                              NVGcolor start, NVGcolor end, float radius);

// Animation configuration
void vknvg__setAnimation(VKNVGtextEffect* effect, VKNVGanimationType type,
                         float speed, float intensity);
void vknvg__updateAnimation(VKNVGtextEffect* effect, float deltaTime);

// Apply effect to text
void vknvg__applyTextEffect(NVGcontext* ctx, VKNVGtextEffect* effect);
```

### Phase 5.5: Integration (Week 5-6)

**Tasks:**
1. Add effect uniforms to shader
2. Update pipeline creation for effects
3. Integrate with existing text rendering
4. Add effect state to VKNVGcontext
5. Wire API calls to rendering pipeline

**Uniform Structure:**
```c
struct VKNVGtextEffectUniforms {
	float outlineWidths[4];
	float outlineColors[16];  // 4 colors × RGBA
	int outlineCount;

	float glowRadius;
	float glowColor[4];
	float glowStrength;

	float shadowOffset[2];
	float shadowBlur;
	float shadowColor[4];

	float gradientStart[4];
	float gradientEnd[4];
	float gradientAngle;
	int gradientType;

	float animationTime;
	int effectType;
	float animationSpeed;
	float animationIntensity;

	int effectFlags;
};
```

### Phase 5.6: Testing (Week 6)

**Test Files:**
- `tests/test_text_outline.c` - Outline rendering tests
- `tests/test_text_glow.c` - Glow effect tests
- `tests/test_text_shadow.c` - Shadow tests
- `tests/test_text_gradient.c` - Gradient tests
- `tests/test_text_animation.c` - Animation tests
- `tests/test_text_effects_integration.c` - Combined effects

**Test Cases:**
1. Single outline rendering
2. Multi-layer outline (2-4 layers)
3. Glow with varying radius
4. Shadow with offset and blur
5. Linear gradient (0°, 45°, 90°)
6. Radial gradient
7. Shimmer animation
8. Pulse animation
9. Wave animation
10. Combined effects (outline + glow + gradient)

## Performance Considerations

### Shader Complexity
- Use dynamic branching for effect flags
- Compile shader variants for common combinations
- Minimize texture lookups

### Memory Usage
- Effect uniforms: ~200 bytes per draw call
- No additional texture memory needed
- Reuse existing SDF texture atlas

### Rendering Performance
- Single pass for most effects
- Multi-pass only for complex shadow blur
- Batch text with same effects together

## Estimated Effort

- **Week 1-2**: SDF shader extensions (40 hours)
- **Week 2-3**: Gradient system (30 hours)
- **Week 3-4**: Animation system (30 hours)
- **Week 4-5**: Effect API (35 hours)
- **Week 5-6**: Integration (25 hours)
- **Week 6**: Testing (20 hours)

**Total**: ~180 hours (6 weeks at 30 hours/week)

## Success Criteria

- [ ] Multi-layer outlines render correctly
- [ ] Glow effect with smooth falloff
- [ ] Shadow with blur
- [ ] Linear and radial gradients
- [ ] Smooth animations at 60+ FPS
- [ ] Combined effects work together
- [ ] All tests passing
- [ ] Performance: <2ms for 1000 glyphs with effects

## Deliverables

1. Extended SDF shader with effects
2. Effect parameter structures
3. Animation system
4. Public API (10+ functions)
5. Test suite (6+ test files, 30+ tests)
6. Documentation updates
7. Example code demonstrating all effects

---

**Status**: Ready to begin implementation
**Target Completion**: 6 weeks from start
**Dependencies**: Phase 1-4 complete ✓
