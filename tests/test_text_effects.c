// test_text_effects.c - Text Effects API Tests
//
// Tests for Phase 5: Advanced Text Effects

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/nanovg_vk_text_effects.h"
#include "../src/nanovg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TEST_ASSERT(cond, msg) do { \
	if (!(cond)) { \
		printf("  ✗ FAIL at line %d: %s\n", __LINE__, msg); \
		return 0; \
	} \
} while(0)

#define TEST_PASS(name) do { \
	printf("  ✓ PASS %s\n", name); \
	return 1; \
} while(0)

// Test 1: Effect context creation/destruction
static int test_effect_creation() {
	printf("Test: Effect context creation/destruction\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();
	TEST_ASSERT(effect != NULL, "effect should not be NULL");
	TEST_ASSERT(effect->effectFlags == VKNVG_EFFECT_NONE, "flags should be NONE");
	TEST_ASSERT(effect->outlineCount == 0, "outline count should be 0");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_effect_creation");
}

// Test 2: Single outline
static int test_single_outline() {
	printf("Test: Single outline configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();
	TEST_ASSERT(effect != NULL, "effect creation");

	NVGcolor red = nvgRGBA(255, 0, 0, 255);
	vknvg__addOutline(effect, 2.0f, red);

	TEST_ASSERT(effect->outlineCount == 1, "outline count should be 1");
	TEST_ASSERT(effect->outlines[0].width == 2.0f, "width should be 2.0");
	TEST_ASSERT(effect->outlines[0].enabled == 1, "outline should be enabled");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_OUTLINE) != 0, "outline flag set");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_single_outline");
}

// Test 3: Multiple outlines
static int test_multiple_outlines() {
	printf("Test: Multiple outline layers\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	NVGcolor colors[4] = {
		nvgRGBA(255, 0, 0, 255),    // Red
		nvgRGBA(0, 255, 0, 255),    // Green
		nvgRGBA(0, 0, 255, 255),    // Blue
		nvgRGBA(255, 255, 0, 255)   // Yellow
	};

	for (int i = 0; i < 4; i++) {
		vknvg__addOutline(effect, (i + 1) * 1.0f, colors[i]);
	}

	TEST_ASSERT(effect->outlineCount == 4, "should have 4 outlines");
	TEST_ASSERT(effect->outlines[0].width == 1.0f, "first width");
	TEST_ASSERT(effect->outlines[3].width == 4.0f, "last width");

	// Test clearing
	vknvg__clearOutlines(effect);
	TEST_ASSERT(effect->outlineCount == 0, "outlines cleared");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_OUTLINE) == 0, "outline flag cleared");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_multiple_outlines");
}

// Test 4: Glow effect
static int test_glow_effect() {
	printf("Test: Glow effect configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	NVGcolor glowColor = nvgRGBA(255, 200, 0, 128);
	vknvg__setGlow(effect, 10.0f, glowColor, 0.8f);

	TEST_ASSERT(effect->glow.enabled == 1, "glow enabled");
	TEST_ASSERT(effect->glow.radius == 10.0f, "glow radius");
	TEST_ASSERT(effect->glow.strength == 0.8f, "glow strength");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GLOW) != 0, "glow flag set");

	// Test disable
	vknvg__enableGlow(effect, 0);
	TEST_ASSERT(effect->glow.enabled == 0, "glow disabled");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GLOW) == 0, "glow flag cleared");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_glow_effect");
}

// Test 5: Shadow effect
static int test_shadow_effect() {
	printf("Test: Shadow effect configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	NVGcolor shadowColor = nvgRGBA(0, 0, 0, 180);
	vknvg__setShadow(effect, 3.0f, 3.0f, 2.0f, shadowColor);

	TEST_ASSERT(effect->shadow.enabled == 1, "shadow enabled");
	TEST_ASSERT(effect->shadow.offsetX == 3.0f, "shadow offsetX");
	TEST_ASSERT(effect->shadow.offsetY == 3.0f, "shadow offsetY");
	TEST_ASSERT(effect->shadow.blur == 2.0f, "shadow blur");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_SHADOW) != 0, "shadow flag set");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_shadow_effect");
}

// Test 6: Linear gradient
static int test_linear_gradient() {
	printf("Test: Linear gradient configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	NVGcolor start = nvgRGBA(255, 0, 0, 255);
	NVGcolor end = nvgRGBA(0, 0, 255, 255);
	float angle = M_PI / 4.0f;  // 45 degrees

	vknvg__setLinearGradient(effect, start, end, angle);

	TEST_ASSERT(effect->gradient.enabled == 1, "gradient enabled");
	TEST_ASSERT(effect->gradient.type == VKNVG_GRADIENT_LINEAR, "linear type");
	TEST_ASSERT(fabs(effect->gradient.angle - angle) < 0.001f, "gradient angle");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GRADIENT) != 0, "gradient flag set");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_linear_gradient");
}

// Test 7: Radial gradient
static int test_radial_gradient() {
	printf("Test: Radial gradient configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	NVGcolor start = nvgRGBA(255, 255, 255, 255);
	NVGcolor end = nvgRGBA(0, 0, 0, 255);
	float radius = 50.0f;

	vknvg__setRadialGradient(effect, start, end, radius);

	TEST_ASSERT(effect->gradient.enabled == 1, "gradient enabled");
	TEST_ASSERT(effect->gradient.type == VKNVG_GRADIENT_RADIAL, "radial type");
	TEST_ASSERT(effect->gradient.radius == radius, "gradient radius");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GRADIENT) != 0, "gradient flag set");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_radial_gradient");
}

// Test 8: Animation configuration
static int test_animation() {
	printf("Test: Animation configuration\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	vknvg__setAnimation(effect, VKNVG_ANIM_SHIMMER, 1.5f, 0.7f);

	TEST_ASSERT(effect->animation.enabled == 1, "animation enabled");
	TEST_ASSERT(effect->animation.type == VKNVG_ANIM_SHIMMER, "animation type");
	TEST_ASSERT(effect->animation.speed == 1.5f, "animation speed");
	TEST_ASSERT(effect->animation.intensity == 0.7f, "animation intensity");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_ANIMATE) != 0, "animate flag set");

	// Test time update
	vknvg__updateAnimation(effect, 0.016f);  // 16ms (60 FPS)
	TEST_ASSERT(effect->animation.time > 0.0f, "time should advance");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_animation");
}

// Test 9: Uniform conversion
static int test_uniform_conversion() {
	printf("Test: Effect to uniform conversion\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();
	VKNVGtextEffectUniforms uniforms;

	// Setup complex effect
	vknvg__addOutline(effect, 2.0f, nvgRGBA(255, 0, 0, 255));
	vknvg__addOutline(effect, 4.0f, nvgRGBA(0, 255, 0, 255));
	vknvg__setGlow(effect, 8.0f, nvgRGBA(255, 255, 0, 128), 0.9f);
	vknvg__setShadow(effect, 2.0f, 2.0f, 1.5f, nvgRGBA(0, 0, 0, 200));

	// Convert to uniforms
	vknvg__effectToUniforms(effect, &uniforms);

	TEST_ASSERT(uniforms.outlineCount == 2, "uniform outline count");
	TEST_ASSERT(uniforms.outlineWidths[0] == 2.0f, "uniform outline width 0");
	TEST_ASSERT(uniforms.outlineWidths[1] == 4.0f, "uniform outline width 1");
	TEST_ASSERT(uniforms.glowRadius == 8.0f, "uniform glow radius");
	TEST_ASSERT(uniforms.shadowOffset[0] == 2.0f, "uniform shadow offsetX");
	TEST_ASSERT((uint32_t)uniforms.effectFlags == effect->effectFlags, "uniform flags match");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_uniform_conversion");
}

// Test 10: Combined effects
static int test_combined_effects() {
	printf("Test: Combined effects (outline + glow + gradient)\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	// Add outline
	vknvg__addOutline(effect, 2.0f, nvgRGBA(0, 0, 0, 255));

	// Add glow
	vknvg__setGlow(effect, 10.0f, nvgRGBA(255, 200, 0, 128), 0.8f);

	// Add gradient
	vknvg__setLinearGradient(effect, nvgRGBA(255, 0, 0, 255),
	                         nvgRGBA(0, 0, 255, 255), 0.0f);

	// Verify all flags set
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_OUTLINE) != 0, "outline flag");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GLOW) != 0, "glow flag");
	TEST_ASSERT((effect->effectFlags & VKNVG_EFFECT_GRADIENT) != 0, "gradient flag");

	TEST_ASSERT(vknvg__hasActiveEffects(effect) == 1, "has active effects");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_combined_effects");
}

// Test 11: Reset effect
static int test_reset_effect() {
	printf("Test: Reset effect to defaults\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	// Configure complex effect
	vknvg__addOutline(effect, 2.0f, nvgRGBA(255, 0, 0, 255));
	vknvg__setGlow(effect, 10.0f, nvgRGBA(255, 255, 0, 128), 0.8f);
	vknvg__setAnimation(effect, VKNVG_ANIM_PULSE, 1.0f, 1.0f);

	TEST_ASSERT(effect->effectFlags != VKNVG_EFFECT_NONE, "has effects before reset");

	// Reset
	vknvg__resetTextEffect(effect);

	TEST_ASSERT(effect->effectFlags == VKNVG_EFFECT_NONE, "no effects after reset");
	TEST_ASSERT(effect->outlineCount == 0, "no outlines after reset");
	TEST_ASSERT(effect->glow.enabled == 0, "glow disabled after reset");
	TEST_ASSERT(effect->animation.enabled == 0, "animation disabled after reset");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_reset_effect");
}

// Test 12: Remove outline layer
static int test_remove_outline_layer() {
	printf("Test: Remove specific outline layer\n");

	VKNVGtextEffect* effect = vknvg__createTextEffect();

	// Add 3 outlines
	vknvg__addOutline(effect, 1.0f, nvgRGBA(255, 0, 0, 255));
	vknvg__addOutline(effect, 2.0f, nvgRGBA(0, 255, 0, 255));
	vknvg__addOutline(effect, 3.0f, nvgRGBA(0, 0, 255, 255));

	TEST_ASSERT(effect->outlineCount == 3, "3 outlines initially");

	// Remove middle layer
	vknvg__removeOutlineLayer(effect, 1);

	TEST_ASSERT(effect->outlines[1].enabled == 0, "layer 1 disabled");
	TEST_ASSERT(effect->outlines[0].enabled == 1, "layer 0 still enabled");
	TEST_ASSERT(effect->outlines[2].enabled == 1, "layer 2 still enabled");

	vknvg__destroyTextEffect(effect);

	TEST_PASS("test_remove_outline_layer");
}

int main() {
	printf("==========================================\n");
	printf("  Text Effects Tests (Phase 5)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_effect_creation()) passed++;
	total++; if (test_single_outline()) passed++;
	total++; if (test_multiple_outlines()) passed++;
	total++; if (test_glow_effect()) passed++;
	total++; if (test_shadow_effect()) passed++;
	total++; if (test_linear_gradient()) passed++;
	total++; if (test_radial_gradient()) passed++;
	total++; if (test_animation()) passed++;
	total++; if (test_uniform_conversion()) passed++;
	total++; if (test_combined_effects()) passed++;
	total++; if (test_reset_effect()) passed++;
	total++; if (test_remove_outline_layer()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
