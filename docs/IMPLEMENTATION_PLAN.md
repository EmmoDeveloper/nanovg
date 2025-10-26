# NanoVG Vulkan Backend Implementation Plan

## Architecture Overview

The Vulkan backend will be implemented as a modular architecture with clear separation of concerns. Each module has a single responsibility and can be developed/tested independently.

## File Structure

```
src/vulkan/
├── nvg_vk.h                    # Main public API (only imports, no logic)
├── nvg_vk_context.h/c          # Context creation/destruction, state management
├── nvg_vk_pipeline.h/c         # Graphics pipeline management
├── nvg_vk_buffer.h/c           # Vertex/index buffer management
├── nvg_vk_texture.h/c          # Texture/image management
├── nvg_vk_render.h/c           # Render commands (fill, stroke, triangles)
├── nvg_vk_shader.h/c           # Shader management
└── nvg_vk_types.h              # Internal types and structures
```

## Module Breakdown

### 1. nvg_vk_types.h
**Purpose**: Internal types and structures shared across modules

**Contents**:
- `NVGVkContext` - Main Vulkan backend context
  - VkDevice, VkQueue, VkCommandPool references
  - Pipeline cache
  - Buffer pools
  - Texture registry
  - Current frame state
- `NVGVkTexture` - Texture descriptor
  - VkImage, VkImageView, VkDeviceMemory
  - Width, height, format, flags
- `NVGVkBuffer` - Buffer descriptor
  - VkBuffer, VkDeviceMemory
  - Size, usage flags
- `NVGVkCall` - Render call structure
  - Call type (fill, stroke, triangles)
  - Uniforms, paths, vertex data
- `NVGVkUniforms` - Shader uniform structures
  - View size, scissor, paint data

**Dependencies**: None (foundation)

---

### 2. nvg_vk_context.h/c
**Purpose**: Context lifecycle and state management

**Functions**:
- `nvgvk_create(VkDevice, VkQueue, ...)` - Initialize Vulkan backend
  - Create command buffers
  - Initialize descriptor pools
  - Setup synchronization primitives
- `nvgvk_delete(NVGVkContext*)` - Cleanup resources
  - Destroy all pipelines
  - Free all buffers
  - Delete all textures
- `nvgvk_viewport(void*, float w, float h, float ratio)` - Handle viewport changes
  - Update view uniforms
  - Invalidate cached state if needed
- `nvgvk_cancel(void*)` - Cancel current frame
  - Reset command buffer
  - Clear call queue
- `nvgvk_flush(void*)` - Flush render commands
  - Record command buffer
  - Submit to queue

**Backend Callbacks**:
- Implements `renderCreate`, `renderViewport`, `renderCancel`, `renderFlush`, `renderDelete`

**Dependencies**: nvg_vk_types.h

---

### 3. nvg_vk_pipeline.h/c
**Purpose**: Graphics pipeline management

**Functions**:
- `nvgvk_pipeline_create_fill(...)` - Create pipeline for fill rendering
- `nvgvk_pipeline_create_stroke(...)` - Create pipeline for stroke rendering
- `nvgvk_pipeline_create_triangles(...)` - Create pipeline for triangle rendering
- `nvgvk_pipeline_get_or_create(state)` - Get cached or create new pipeline
- `nvgvk_pipeline_configure_blend(...)` - Setup blend modes
- `nvgvk_pipeline_destroy_all(...)` - Cleanup all pipelines

**Pipeline Variants**:
- Fill: stencil write + stencil test passes
- Stroke: direct rendering
- Triangles: textured/colored quads
- Different blend modes (source-over, multiply, etc.)

**Descriptor Sets**:
- Set 0: Uniform buffer (view, scissor)
- Set 1: Texture samplers

**Push Constants**:
- Paint uniforms (transform, colors, image index)

**Dependencies**: nvg_vk_types.h, nvg_vk_shader.h

---

### 4. nvg_vk_buffer.h/c
**Purpose**: Buffer management with dynamic growth

**Functions**:
- `nvgvk_buffer_create(size, usage)` - Allocate buffer
- `nvgvk_buffer_destroy(buffer)` - Free buffer
- `nvgvk_buffer_upload(buffer, data, size)` - Upload data to buffer
- `nvgvk_buffer_reserve(buffer, size)` - Grow buffer if needed
- `nvgvk_buffer_reset(buffer)` - Reset write pointer

**Buffer Types**:
- Vertex buffer (dynamic, host-visible)
- Index buffer (if needed for complex paths)
- Uniform buffer (per-frame)

**Memory Management**:
- Pool allocator for small buffers
- Dynamic growth strategy
- Reuse across frames

**Dependencies**: nvg_vk_types.h

---

### 5. nvg_vk_texture.h/c
**Purpose**: Texture and image operations

**Functions**:
- `nvgvk_create_texture(type, w, h, flags, data)` - Create texture from data
  - ALPHA or RGBA format
  - Optional mipmaps
  - Image flags (repeat, flip, etc.)
- `nvgvk_delete_texture(image)` - Delete texture
  - Free VkImage, VkImageView, memory
  - Remove from descriptor sets
- `nvgvk_update_texture(image, x, y, w, h, data)` - Update texture region
  - Staging buffer upload
  - Transition image layouts
- `nvgvk_get_texture_size(image, w, h)` - Query texture dimensions

**Font Atlas**:
- Manage font texture atlas
- Handle atlas growth/updates
- Integration with fontstash

**Descriptor Management**:
- Update descriptor sets when textures change
- Maintain texture ID → VkImageView mapping

**Backend Callbacks**:
- Implements `renderCreateTexture`, `renderDeleteTexture`, `renderUpdateTexture`, `renderGetTextureSize`

**Dependencies**: nvg_vk_types.h, nvg_vk_buffer.h

---

### 6. nvg_vk_render.h/c
**Purpose**: Core rendering commands implementation

**Functions**:
- `nvgvk_fill(paint, scissor, paths, npaths)` - Fill paths
  - Stencil-based algorithm:
    1. Clear stencil
    2. Draw fill geometry to stencil
    3. Draw bounding quad with stencil test
  - Convert NVGpaint to shader uniforms
- `nvgvk_stroke(paint, scissor, width, paths, npaths)` - Stroke paths
  - Direct rendering of stroke geometry
  - Handle line caps and joins
- `nvgvk_triangles(paint, scissor, verts, nverts)` - Draw triangles
  - Used for text rendering
  - Textured quads

**Call Queue**:
- Buffer rendering calls
- Batch compatible calls
- Flush on state changes

**Uniforms**:
- Convert NVGpaint to shader-friendly format
- Handle gradient transforms
- Scissor rectangle conversion

**Backend Callbacks**:
- Implements `renderFill`, `renderStroke`, `renderTriangles`

**Dependencies**: nvg_vk_types.h, nvg_vk_pipeline.h, nvg_vk_buffer.h

---

### 7. nvg_vk_shader.h/c
**Purpose**: Shader compilation and management

**Functions**:
- `nvgvk_shader_load(device, filename)` - Load SPIR-V shader
  - Reuse/extend existing vk_shader functionality
- `nvgvk_shader_create_module(device, code, size)` - Create shader module
- `nvgvk_shader_compile_variants()` - Compile all needed variants

**Shader Variants**:

**Vertex Shaders**:
- `fill.vert` - Vertex shader for fill geometry
- `stroke.vert` - Vertex shader for stroke geometry
- `triangles.vert` - Vertex shader for textured triangles

**Fragment Shaders**:
- `fill_color.frag` - Solid color fill
- `fill_gradient.frag` - Linear/radial gradient
- `fill_image.frag` - Image pattern fill
- `stroke_color.frag` - Solid color stroke
- `stroke_gradient.frag` - Gradient stroke
- `triangles.frag` - Textured triangles (for text)

**Uniform Structures**:
```glsl
// Vertex shader uniforms (push constants)
layout(push_constant) uniform PaintUniforms {
    vec2 viewSize;
    mat3 scissorMat;
    mat3 paintMat;
    vec4 innerCol;
    vec4 outerCol;
    vec2 scissorExt;
    vec2 extent;
    float radius;
    float feather;
    float strokeMult;
    int texType;
    int type;
};
```

**Dependencies**: nvg_vk_types.h, vk_shader.h

---

### 8. nvg_vk.h (Main Header)
**Purpose**: Public API with minimal logic - only imports and thin wrappers

**Contents**:
```c
#ifndef NVG_VK_H
#define NVG_VK_H

#include <vulkan/vulkan.h>
#include "nanovg.h"

// Import all modules
#include "nvg_vk_types.h"
#include "nvg_vk_context.h"
#include "nvg_vk_pipeline.h"
#include "nvg_vk_buffer.h"
#include "nvg_vk_texture.h"
#include "nvg_vk_render.h"
#include "nvg_vk_shader.h"

// Public API - thin wrappers only
NVGcontext* nvgCreateVk(VkDevice device, VkQueue queue,
                        VkCommandPool commandPool, int flags);
void nvgDeleteVk(NVGcontext* ctx);

#endif
```

**Implementation** (nvg_vk.c):
- Wire together all modules
- Register callbacks with NanoVG
- Minimal glue code only

**Dependencies**: All other modules

---

## Implementation Phases

### Phase 1: Foundation (Week 1)
**Goal**: Basic infrastructure and types

1. **nvg_vk_types.h**
   - Define all structures
   - No implementation needed
   - Review and finalize API

2. **nvg_vk_context.c**
   - Basic context creation/deletion
   - Minimal Vulkan setup
   - Register stub callbacks

3. **nvg_vk_buffer.c**
   - Simple buffer allocation
   - Fixed-size buffers initially
   - No pooling yet

**Milestone**: Can create/destroy context without crashes

---

### Phase 2: Resource Management (Week 2)
**Goal**: Handle textures and shaders

4. **nvg_vk_texture.c**
   - Texture creation from data
   - Texture deletion
   - Simple descriptor management
   - No atlas updates yet

5. **nvg_vk_shader.c**
   - Load compiled SPIR-V shaders
   - Create shader modules
   - Basic shader variants (solid color only)

**Milestone**: Can create textures and load shaders

---

### Phase 3: Pipeline (Week 3)
**Goal**: Graphics pipeline creation

6. **nvg_vk_pipeline.c**
   - Create fill pipeline (stencil-based)
   - Create stroke pipeline
   - Create triangles pipeline
   - Basic blend modes
   - No caching yet

**Milestone**: Can create all pipeline types

---

### Phase 4: Basic Rendering (Week 4)
**Goal**: Draw simple shapes

7. **nvg_vk_render.c** (Basic)
   - Implement solid color fills
   - Implement solid color strokes
   - Simple triangle rendering
   - No gradients/images yet

8. **nvg_vk.h/c**
   - Wire everything together
   - Test with simple shapes

**Milestone**: Can draw colored rectangles and circles

---

### Phase 5: Advanced Rendering (Week 5-6)
**Goal**: Complete feature set

- Gradient fills
- Image patterns
- Text rendering
- Scissoring
- All blend modes
- Buffer pooling
- Pipeline caching

**Milestone**: Full NanoVG feature parity

---

## Required Backend Callbacks

From NVGparams structure, must implement:

| Callback | Module | Phase |
|----------|--------|-------|
| `renderCreate` | nvg_vk_context.c | 1 |
| `renderDelete` | nvg_vk_context.c | 1 |
| `renderViewport` | nvg_vk_context.c | 1 |
| `renderCancel` | nvg_vk_context.c | 1 |
| `renderFlush` | nvg_vk_context.c | 4 |
| `renderCreateTexture` | nvg_vk_texture.c | 2 |
| `renderDeleteTexture` | nvg_vk_texture.c | 2 |
| `renderUpdateTexture` | nvg_vk_texture.c | 2 |
| `renderGetTextureSize` | nvg_vk_texture.c | 2 |
| `renderFill` | nvg_vk_render.c | 4 |
| `renderStroke` | nvg_vk_render.c | 4 |
| `renderTriangles` | nvg_vk_render.c | 4 |

---

## Testing Strategy

### Unit Tests
- Each module tested independently
- Mock Vulkan objects where needed
- Focus on logic, not graphics

### Integration Tests
- Test module interactions
- Use actual Vulkan context
- Verify resource cleanup

### Visual Tests
- Compare against reference images
- Test all NanoVG demo scenes
- Verify text rendering

---

## Benefits of This Architecture

1. **Modularity**
   - Each file has single responsibility
   - Clear module boundaries
   - Easy to understand

2. **Testability**
   - Each module can be tested independently
   - Mock interfaces between modules
   - Isolated unit tests

3. **Maintainability**
   - Easy to locate functionality
   - Clear where to add features
   - Minimal interdependencies

4. **Scalability**
   - Easy to add new features
   - Can optimize individual modules
   - Future: MSDF text, compute shaders, etc.

5. **Clean Main File**
   - nvg_vk.h is just imports
   - Minimal glue code in nvg_vk.c
   - All logic in specific modules

---

## Next Steps

1. Review and approve this plan
2. Start Phase 1: Foundation
   - Create nvg_vk_types.h
   - Implement nvg_vk_context.c
   - Implement nvg_vk_buffer.c
3. Set up test framework
4. Proceed through phases sequentially
