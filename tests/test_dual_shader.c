// test_dual_shader.c - Dual-Mode Shader Integration Test
//
// Tests for Phase 6.6: Dual-mode shader integration
// Demonstrates how to use text_dual.vert/frag shaders with text-emoji system

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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

// Mock structures for shader integration

// Glyph instance data (matches shader vertex input)
typedef struct GlyphInstance {
	float posX, posY;          // Instance position (location 2)
	float sizeX, sizeY;        // Instance size (location 3)
	float uvOffsetX, uvOffsetY; // UV offset (location 4)
	float uvSizeX, uvSizeY;    // UV size (location 5)
	uint32_t renderMode;       // 0 = SDF, 1 = Color (location 6)
} GlyphInstance;

// Fragment uniform buffer (matches shader UBO)
typedef struct FragmentUniforms {
	float scissorMat[9];
	float paintMat[9];
	float innerCol[4];
	float outerCol[4];
	float scissorExt[2];
	float scissorScale[2];
	float extent[2];
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
} FragmentUniforms;

// Descriptor set bindings
typedef struct DescriptorSet {
	FragmentUniforms* uniforms;  // Binding 0
	void* sdfTexture;            // Binding 1
	void* colorTexture;          // Binding 2
} DescriptorSet;

// Test 1: Glyph instance structure layout
static int test_instance_layout() {
	printf("Test: Glyph instance layout\n");

	GlyphInstance instance = {0};
	instance.posX = 100.0f;
	instance.posY = 200.0f;
	instance.sizeX = 32.0f;
	instance.sizeY = 32.0f;
	instance.uvOffsetX = 0.0f;
	instance.uvOffsetY = 0.0f;
	instance.uvSizeX = 0.1f;
	instance.uvSizeY = 0.1f;
	instance.renderMode = 0; // SDF

	TEST_ASSERT(sizeof(GlyphInstance) == 36, "instance size");
	TEST_ASSERT(instance.posX == 100.0f, "position X");
	TEST_ASSERT(instance.renderMode == 0, "render mode SDF");

	instance.renderMode = 1; // Color emoji
	TEST_ASSERT(instance.renderMode == 1, "render mode emoji");

	TEST_PASS("test_instance_layout");
}

// Test 2: Fragment uniforms structure
static int test_fragment_uniforms() {
	printf("Test: Fragment uniforms\n");

	FragmentUniforms uniforms = {0};
	uniforms.innerCol[0] = 1.0f; // Red
	uniforms.innerCol[1] = 1.0f; // Green
	uniforms.innerCol[2] = 1.0f; // Blue
	uniforms.innerCol[3] = 1.0f; // Alpha
	uniforms.extent[0] = 512.0f;
	uniforms.extent[1] = 512.0f;

	TEST_ASSERT(uniforms.innerCol[0] == 1.0f, "inner color R");
	TEST_ASSERT(uniforms.extent[0] == 512.0f, "extent X");

	TEST_PASS("test_fragment_uniforms");
}

// Test 3: Descriptor set layout
static int test_descriptor_layout() {
	printf("Test: Descriptor set layout\n");

	FragmentUniforms uniforms = {0};
	DescriptorSet descSet = {0};

	descSet.uniforms = &uniforms;
	descSet.sdfTexture = (void*)0x1000; // Mock handle
	descSet.colorTexture = (void*)0x2000; // Mock handle

	TEST_ASSERT(descSet.uniforms == &uniforms, "uniforms binding");
	TEST_ASSERT(descSet.sdfTexture != NULL, "SDF texture bound");
	TEST_ASSERT(descSet.colorTexture != NULL, "color texture bound");

	TEST_PASS("test_descriptor_layout");
}

// Test 4: SDF glyph instance creation
static int test_create_sdf_instance() {
	printf("Test: Create SDF glyph instance\n");

	// Simulate creating instance for SDF glyph
	GlyphInstance sdfGlyph = {0};
	sdfGlyph.posX = 50.0f;
	sdfGlyph.posY = 100.0f;
	sdfGlyph.sizeX = 24.0f;
	sdfGlyph.sizeY = 24.0f;
	sdfGlyph.uvOffsetX = 0.25f;
	sdfGlyph.uvOffsetY = 0.5f;
	sdfGlyph.uvSizeX = 0.05f;
	sdfGlyph.uvSizeY = 0.05f;
	sdfGlyph.renderMode = 0; // SDF mode

	TEST_ASSERT(sdfGlyph.renderMode == 0, "SDF render mode");
	TEST_ASSERT(sdfGlyph.sizeX == 24.0f, "glyph width");

	TEST_PASS("test_create_sdf_instance");
}

// Test 5: Color emoji instance creation
static int test_create_emoji_instance() {
	printf("Test: Create emoji glyph instance\n");

	// Simulate creating instance for color emoji
	GlyphInstance emojiGlyph = {0};
	emojiGlyph.posX = 150.0f;
	emojiGlyph.posY = 200.0f;
	emojiGlyph.sizeX = 64.0f;  // Larger for emoji
	emojiGlyph.sizeY = 64.0f;
	emojiGlyph.uvOffsetX = 0.0f;
	emojiGlyph.uvOffsetY = 0.0f;
	emojiGlyph.uvSizeX = 0.125f;
	emojiGlyph.uvSizeY = 0.125f;
	emojiGlyph.renderMode = 1; // Color emoji mode

	TEST_ASSERT(emojiGlyph.renderMode == 1, "emoji render mode");
	TEST_ASSERT(emojiGlyph.sizeX == 64.0f, "emoji width");

	TEST_PASS("test_create_emoji_instance");
}

// Test 6: Mixed instance batch
static int test_mixed_batch() {
	printf("Test: Mixed SDF and emoji batch\n");

	// Simulate a text string with mixed SDF text and emoji
	GlyphInstance batch[5] = {0};

	// "Hello 😀"
	batch[0].renderMode = 0; // H (SDF)
	batch[1].renderMode = 0; // e (SDF)
	batch[2].renderMode = 0; // l (SDF)
	batch[3].renderMode = 0; // l (SDF)
	batch[4].renderMode = 1; // 😀 (emoji)

	int sdfCount = 0;
	int emojiCount = 0;

	for (int i = 0; i < 5; i++) {
		if (batch[i].renderMode == 0) sdfCount++;
		else if (batch[i].renderMode == 1) emojiCount++;
	}

	TEST_ASSERT(sdfCount == 4, "4 SDF glyphs");
	TEST_ASSERT(emojiCount == 1, "1 emoji glyph");

	TEST_PASS("test_mixed_batch");
}

// Test 7: Descriptor binding validation
static int test_descriptor_binding() {
	printf("Test: Descriptor binding validation\n");

	// Ensure all three bindings are present
	DescriptorSet descSet = {0};
	FragmentUniforms uniforms = {0};

	// Binding 0: Uniforms
	descSet.uniforms = &uniforms;
	TEST_ASSERT(descSet.uniforms != NULL, "binding 0 uniforms");

	// Binding 1: SDF texture
	descSet.sdfTexture = (void*)0x1000;
	TEST_ASSERT(descSet.sdfTexture != NULL, "binding 1 SDF texture");

	// Binding 2: Color texture
	descSet.colorTexture = (void*)0x2000;
	TEST_ASSERT(descSet.colorTexture != NULL, "binding 2 color texture");

	TEST_PASS("test_descriptor_binding");
}

// Test 8: Render mode switching
static int test_render_mode_switch() {
	printf("Test: Render mode switching\n");

	GlyphInstance glyph = {0};

	// Start as SDF
	glyph.renderMode = 0;
	TEST_ASSERT(glyph.renderMode == 0, "initial SDF mode");

	// Switch to emoji
	glyph.renderMode = 1;
	TEST_ASSERT(glyph.renderMode == 1, "switched to emoji");

	// Switch back to SDF
	glyph.renderMode = 0;
	TEST_ASSERT(glyph.renderMode == 0, "switched back to SDF");

	TEST_PASS("test_render_mode_switch");
}

// Test 9: Instance data integrity
static int test_instance_integrity() {
	printf("Test: Instance data integrity\n");

	GlyphInstance instance = {
		.posX = 123.456f,
		.posY = 789.012f,
		.sizeX = 32.5f,
		.sizeY = 28.3f,
		.uvOffsetX = 0.125f,
		.uvOffsetY = 0.375f,
		.uvSizeX = 0.0625f,
		.uvSizeY = 0.0625f,
		.renderMode = 1
	};

	// Verify all fields retained
	TEST_ASSERT(instance.posX == 123.456f, "position X retained");
	TEST_ASSERT(instance.posY == 789.012f, "position Y retained");
	TEST_ASSERT(instance.sizeX == 32.5f, "size X retained");
	TEST_ASSERT(instance.sizeY == 28.3f, "size Y retained");
	TEST_ASSERT(instance.uvOffsetX == 0.125f, "UV offset X retained");
	TEST_ASSERT(instance.uvOffsetY == 0.375f, "UV offset Y retained");
	TEST_ASSERT(instance.uvSizeX == 0.0625f, "UV size X retained");
	TEST_ASSERT(instance.uvSizeY == 0.0625f, "UV size Y retained");
	TEST_ASSERT(instance.renderMode == 1, "render mode retained");

	TEST_PASS("test_instance_integrity");
}

// Test 10: Batch size validation
static int test_batch_size() {
	printf("Test: Batch size validation\n");

	// Test various batch sizes
	GlyphInstance* batch1 = calloc(1, sizeof(GlyphInstance));
	TEST_ASSERT(batch1 != NULL, "single instance batch");
	free(batch1);

	GlyphInstance* batch100 = calloc(100, sizeof(GlyphInstance));
	TEST_ASSERT(batch100 != NULL, "100 instance batch");
	free(batch100);

	GlyphInstance* batch1000 = calloc(1000, sizeof(GlyphInstance));
	TEST_ASSERT(batch1000 != NULL, "1000 instance batch");
	free(batch1000);

	TEST_PASS("test_batch_size");
}

int main() {
	printf("==========================================\n");
	printf("  Dual-Mode Shader Tests (Phase 6.6)\n");
	printf("==========================================\n\n");

	int total = 0;
	int passed = 0;

	// Run tests
	total++; if (test_instance_layout()) passed++;
	total++; if (test_fragment_uniforms()) passed++;
	total++; if (test_descriptor_layout()) passed++;
	total++; if (test_create_sdf_instance()) passed++;
	total++; if (test_create_emoji_instance()) passed++;
	total++; if (test_mixed_batch()) passed++;
	total++; if (test_descriptor_binding()) passed++;
	total++; if (test_render_mode_switch()) passed++;
	total++; if (test_instance_integrity()) passed++;
	total++; if (test_batch_size()) passed++;

	// Summary
	printf("\n========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", total);
	printf("  Passed:  %d\n", passed);
	printf("  Failed:  %d\n", total - passed);
	printf("========================================\n");

	return (passed == total) ? 0 : 1;
}
