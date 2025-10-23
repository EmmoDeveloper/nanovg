# Phase 6 Backend Integration Plan

## Overview
Integrate Phase 6 color emoji rendering into the NanoVG Vulkan backend.

## Current State Analysis

### Existing Text Rendering
- **Location**: `src/nanovg_vk_render.h:1720-1815`
- **Pipeline Selection**: Based on texture type and flags
  - `textColorPipeline` for RGBA textures (type == 3) with `NVG_COLOR_TEXT`
  - `textSDFPipeline` for alpha-only (type == 2) with `NVG_SDF_TEXT`
  - `textMSDFPipeline` with `NVG_MSDF_TEXT`
  - `textSubpixelPipeline` with `NVG_SUBPIXEL_TEXT`
  - Instanced variants: `textInstancedColorPipeline`, etc.

### Context Structure
- **File**: `src/nanovg_vk_internal.h`
- **Added Fields** (lines 297-305):
  ```c
  VKNVGtextEmojiState* textEmojiState;
  VKNVGcolorAtlas* colorAtlas;
  VkBool32 useColorEmoji;
  VkPipeline textDualModePipeline;
  VkShaderModule textDualModeVertShaderModule;
  VkShaderModule textDualModeFragShaderModule;
  VkDescriptorSetLayout dualModeDescriptorSetLayout;
  VkDescriptorSet* dualModeDescriptorSets;
  ```

## Integration Tasks

### 1. Shader Integration ✅
**Status**: Complete (Phase 6.6)
- `shaders/text_dual.vert` - Passes render mode (location 6)
- `shaders/text_dual.frag` - Dual texture binding (SDF + color atlas)
- Compiled SPIR-V available

### 2. Context Initialization
**File**: `src/nanovg_vk_render.h:45` (vknvg__renderCreate)
**Tasks**:
- [ ] Load dual-mode shader modules (around line 138)
- [ ] Create color atlas (after line 200)
- [ ] Initialize text-emoji state with emoji font
- [ ] Create dual-mode descriptor set layout (3 bindings)
- [ ] Create dual-mode pipeline
- [ ] Allocate dual-mode descriptor sets

### 3. Emoji Font Loading
**Approach**: Optional emoji font parameter in NVGVkCreateInfo
**Tasks**:
- [ ] Add `emojiFont` field to `NVGVkCreateInfo` structure
- [ ] Load emoji font in `vknvg__renderCreate`
- [ ] Detect font format (SBIX/CBDT/COLR)
- [ ] Initialize emoji renderer

### 4. Descriptor Set Layout
**Current**: 2 bindings (uniform buffer + texture sampler)
**New**: 3 bindings for dual-mode
- Binding 0: FragmentUniforms (UBO)
- Binding 1: SDF texture sampler
- Binding 2: Color atlas texture sampler

### 5. Pipeline Selection Logic
**File**: `src/nanovg_vk_render.h:1720-1748`
**Modification**:
```c
if (call->image > 0) {
    VKNVGtexture* tex = vknvg__findTexture(vk, call->image);
    if (tex != NULL) {
        // Check if we should use dual-mode pipeline
        if (vk->useColorEmoji && vk->textEmojiState) {
            trianglesPipeline = vk->textDualModePipeline;
            // Use dual-mode descriptor set
            // Bind both SDF texture and color atlas
        } else if (tex->type == 3 && (vk->flags & NVG_COLOR_TEXT)) {
            // Existing color text pipeline
            trianglesPipeline = useInstancedPipeline ? vk->textInstancedColorPipeline : vk->textColorPipeline;
        } else if (tex->type == 2) {
            // Existing SDF/MSDF/Subpixel
            // ...
        }
    }
}
```

### 6. Instance Data Update
**File**: `src/nanovg_vk_text_instance.h`
**Current Instance Data**:
```c
struct VKNVGglyphInstance {
    vec2 position;
    vec2 size;
    vec2 uvOffset;
    vec2 uvSize;
    // Need to add: uint32_t renderMode;
};
```

### 7. Glyph Rendering Integration
**File**: `src/fontstash.h` or virtual atlas callback
**Modification**: Use `vknvg__getGlyphRenderInfo()` to determine render mode
- If emoji → upload to color atlas, set renderMode = 1
- If SDF → upload to SDF atlas, set renderMode = 0

### 8. Cleanup
**File**: `src/nanovg_vk_cleanup.h`
**Tasks**:
- [ ] Destroy text-emoji state
- [ ] Destroy color atlas
- [ ] Destroy dual-mode shader modules
- [ ] Destroy dual-mode pipeline
- [ ] Destroy dual-mode descriptor set layout

## Flag Addition

Add new NanoVG flag for emoji support:
```c
#define NVG_COLOR_EMOJI (1 << 16)  // Enable color emoji rendering
```

## Emoji Font Path

**Options**:
1. Add to NVGVkCreateInfo:
   ```c
   typedef struct NVGVkCreateInfo {
       // existing fields...
       const char* emojiFont Path;  // Path to emoji font (NULL = no emoji)
   } NVGVkCreateInfo;
   ```

2. Or load emoji font data separately:
   ```c
   void nvgVkSetEmojiFont(NVGcontext* ctx, const uint8_t* fontData, uint32_t size);
   ```

## Testing Strategy

1. **Unit Test**: Verify context initialization with emoji font
2. **Integration Test**: Render "Hello 😀🔥" with mixed SDF+emoji
3. **Visual Test**: Compare with reference image
4. **Performance Test**: Measure overhead vs SDF-only

## Phased Rollout

### Phase A: Minimal Integration (Current)
- Add context fields ✅
- Add forward declarations ✅

### Phase B: Initialization
- Load dual-mode shaders
- Create color atlas
- Create dual-mode pipeline

### Phase C: Rendering Integration
- Modify pipeline selection
- Update instance data
- Bind color atlas

### Phase D: Glyph Handling
- Integrate with virtual atlas
- Route emoji to color atlas
- Set render mode in instances

### Phase E: Testing & Polish
- Run all tests
- Fix any issues
- Performance profiling

## Notes

- Keep backward compatibility: emoji rendering is optional
- Fall back gracefully if emoji font not provided
- Reuse existing text instancing infrastructure
- Color atlas is separate from SDF atlas (different formats)

## Files to Modify

1. `src/nanovg_vk_internal.h` - Context structure ✅
2. `src/nanovg_vk_render.h` - Pipeline selection
3. `src/nanovg_vk_cleanup.h` - Resource cleanup
4. `src/nanovg_vk_text_instance.h` - Instance data
5. `src/nanovg_vk_types.h` - Add NVG_COLOR_EMOJI flag
6. `src/nanovg_vk.h` - Include emoji headers
