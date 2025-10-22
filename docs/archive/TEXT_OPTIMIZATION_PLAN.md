# Text Rendering Optimizations - Implementation Plan

## Current Architecture Analysis

### How Text is Currently Rendered

**Vertex Generation** (NanoVG Core):
- Each glyph generates 2 triangles = 6 vertices
- Vertices stored in single vertex buffer
- Format: position (x,y) + texcoord (u,v)

**Rendering** (Vulkan Backend):
- Type 2 (TRIANGLES) draw calls for text
- Pipeline selection based on texture type:
  - Standard text: `textPipeline`
  - SDF text: `textSDFPipeline`
  - MSDF text: `textMSDFPipeline`
  - Subpixel: `textSubpixelPipeline`
  - Color emoji: `textColorPipeline`
- One `vkCmdDraw()` per text call
- Font atlas managed by NanoVG core

**Performance Bottlenecks**:
1. **High vertex count**: 6 vertices per glyph = significant memory bandwidth
2. **No batching**: Each `nvgText()` call = separate draw call
3. **No instancing**: Repeated glyph shapes not reused
4. **CPU overhead**: Vertex generation on CPU
5. **Synchronous uploads**: Atlas updates block rendering

---

## Optimization 1: Glyph Instancing

**Goal**: Reduce vertex count from 6 vertices/glyph to 1 instance/glyph

### Architecture

**Instance Data** (per glyph):
```c
struct GlyphInstance {
	float position[2];      // Screen position
	float size[2];          // Glyph size
	float uvOffset[2];      // Atlas UV offset
	float uvSize[2];        // Atlas UV size
	uint32_t color;         // RGBA packed
};
```

**Vertex Shader Changes**:
```glsl
// Input: gl_InstanceIndex
// Generate quad corners in shader:
// - 4 vertices per instance (using gl_VertexIndex % 4)
// - Position = instance.position + corner * instance.size
// - UV = instance.uvOffset + corner * instance.uvSize
```

**Benefits**:
- **75% less vertex data**: 32 bytes/glyph vs 96 bytes/glyph
- **Better cache utilization**: Smaller instance buffer
- **GPU-side quad generation**: Reduces CPU load

**Implementation Files**:
- `src/nanovg_vk_text_instance.h` - New instance system
- `shaders/text_instanced.vert` - Instance vertex shader
- Modify `src/nanovg_vk_pipeline.h` - Add instanced pipeline

---

## Optimization 2: Batch Text Rendering

**Goal**: Combine multiple text calls into single draw call

### Architecture

**Batching Strategy**:
- Accumulate glyphs from multiple `nvgText()` calls
- Flush when:
  - Pipeline changes (SDF → MSDF)
  - Transform changes
  - Color changes (optional - can pass per-instance)
  - Instance buffer full (max 64K glyphs)

**Implementation**:
```c
typedef struct {
	GlyphInstance* instances;
	int count;
	int capacity;
	VkPipeline currentPipeline;
} VKNVGtextBatch;
```

**Benefits**:
- **10-100x fewer draw calls**: Typical UI has 10-100 text strings
- **Reduced CPU overhead**: Less validation and submission
- **Better GPU utilization**: Larger batches = better parallelism

**Implementation Files**:
- `src/nanovg_vk_text_batch.h` - Batching system
- Modify `src/nanovg_vk_render.h` - Integrate batching

---

## Optimization 3: Pre-Warmed Font Atlas

**Goal**: Eliminate first-frame stutters from glyph uploads

### Architecture

**Common Glyphs**:
- ASCII printable chars (32-126) = 95 glyphs
- Common punctuation and symbols
- Pre-rasterize at startup for common sizes (12px, 14px, 16px, 18px, 24px)

**Implementation**:
```c
static const char* preWarmGlyphs =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
	"0123456789 .,!?;:'\"-()[]{}";

void vknvg__preWarmAtlas(NVGcontext* ctx, int fontID) {
	float sizes[] = {12, 14, 16, 18, 24};
	for (each size) {
		nvgFontSize(ctx, size);
		nvgText(ctx, -1000, -1000, preWarmGlyphs, NULL); // Off-screen
	}
}
```

**Benefits**:
- **No first-frame stutter**: Atlas ready immediately
- **Predictable performance**: No runtime allocations for common text
- **Better memory layout**: Pre-packed atlas has better locality

**Implementation Files**:
- `src/nanovg_vk_atlas.h` - Atlas management
- Add to `nvgCreateVk()` initialization

---

## Optimization 4: Text Run Caching

**Goal**: Cache frequently rendered text as textures

### Architecture

**Cache Strategy**:
- Hash text string + font + size + color → cache key
- Render to texture once, reuse for subsequent frames
- LRU eviction policy (max 256 cached strings)
- Invalidate on font/size/color change

**Cache Entry**:
```c
typedef struct {
	uint64_t hash;
	VkImage texture;
	VkImageView view;
	float bounds[4];        // x, y, width, height
	uint32_t lastFrame;     // LRU tracking
	uint32_t refCount;
} VKNVGtextCacheEntry;
```

**Benefits**:
- **100x faster**: Texture blit vs full text render
- **Consistent perf**: No per-glyph processing
- **Great for UI**: Labels, buttons render identical text every frame

**Limitations**:
- Memory cost: ~100KB per cached string (1024x64 RGBA)
- Best for static text, not animated/changing text

**Implementation Files**:
- `src/nanovg_vk_text_cache.h` - Cache system
- Modify `src/nanovg_vk_render.h` - Integrate cache lookup

---

## Optimization 5: Compute-Based Glyph Rasterization

**Goal**: Offload glyph rasterization from CPU to GPU compute shaders

### Architecture

**Compute Pipeline**:
1. Input: Glyph outline (Bezier curves)
2. Compute: Rasterize to SDF in parallel
3. Output: Atlas texture (direct write)

**Compute Shader**:
```glsl
layout(local_size_x = 8, local_size_y = 8) in;

// Input: glyph outline buffer
layout(binding = 0) buffer GlyphOutlines {
	vec2 curves[];
};

// Output: atlas texture
layout(binding = 1, r8) uniform image2D atlasImage;

void main() {
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	float sdf = computeSDF(pixel, curves);
	imageStore(atlasImage, pixel, vec4(sdf));
}
```

**Benefits**:
- **GPU parallelism**: 1000s of pixels processed simultaneously
- **Frees CPU**: No CPU rasterization overhead
- **Better for SDF/MSDF**: GPU-optimized distance field computation

**Challenges**:
- Complex implementation: Need to port FreeType/stb_truetype logic
- Outline data management: Upload curves to GPU
- Synchronization: Ensure atlas ready before use

**Implementation Files**:
- `shaders/glyph_rasterize.comp` - Compute shader
- `src/nanovg_vk_compute_raster.h` - Compute pipeline
- Modify `src/nanovg_vk_image.h` - Integrate compute uploads

---

## Optimization 6: Async Glyph Uploads

**Goal**: Use async compute queue for non-blocking atlas updates

### Architecture

**RTX 3080 Laptop GPU**:
- 1 Graphics queue
- 1 Async compute queue (independent)
- Can execute compute while graphics renders previous frame

**Async Upload Flow**:
```
Frame N:
  Graphics queue: Render frame N-1 (uses atlas from frame N-2)
  Compute queue:  Upload glyphs for frame N+1 (async)

Frame N+1:
  Graphics queue: Render frame N (uses atlas from frame N-1)
  Compute queue:  Upload glyphs for frame N+2 (async)
```

**Implementation**:
```c
typedef struct {
	VkQueue asyncQueue;
	VkCommandPool asyncCmdPool;
	VkFence uploadFence;
	VKNVGbuffer* stagingBuffers[3];  // Triple buffering
	int currentBuffer;
} VKNVGasyncUpload;
```

**Benefits**:
- **Zero stall**: Uploads don't block rendering
- **Better GPU utilization**: Both queues active
- **Smoother framerate**: Eliminates upload spikes

**Challenges**:
- Synchronization: Fence + semaphore management
- Resource lifetimes: Staging buffers must persist
- Queue family transfer: If async queue in different family

**Implementation Files**:
- `src/nanovg_vk_async_upload.h` - Async system
- Modify `src/nanovg_vk_platform.h` - Queue creation
- Modify `src/nanovg_vk_image.h` - Async upload path

---

## Implementation Order

### Phase 1: Foundation (High Impact, Low Risk)
1. **Pre-warmed Atlas** (2-3 hours)
   - Simple, immediate benefit
   - No architecture changes

2. **Glyph Instancing** (1-2 days)
   - Core optimization, enables other improvements
   - Medium complexity, well-defined

### Phase 2: Performance (Medium Impact, Medium Risk)
3. **Batch Text Rendering** (1 day)
   - Builds on instancing
   - Significant draw call reduction

4. **Text Run Caching** (1-2 days)
   - Orthogonal to other optimizations
   - Great for UI rendering

### Phase 3: Advanced (High Impact, High Risk)
5. **Compute-Based Rasterization** (3-5 days)
   - Complex implementation
   - GPU-specific optimizations

6. **Async Glyph Uploads** (2-3 days)
   - Requires compute rasterization
   - Tricky synchronization

**Total Estimated Time**: 10-16 days

---

## Performance Targets

### Current Baseline (from benchmarks)
- Text rendering: Unknown (not separately benchmarked)
- Estimated: ~100-200 glyphs/ms on RTX 3080

### Target Performance (Post-Optimization)
- **Instancing**: 3-4x faster (300-800 glyphs/ms)
- **Batching**: 2-3x additional (600-2400 glyphs/ms)
- **Caching**: 100x for cached text (instant blit)
- **Async**: Eliminates upload stalls (smoother frame times)

### Success Metrics
- 10,000 glyphs at 60 FPS (166 µs budget = ~60 glyphs/µs)
- Zero visible stutters on first-time glyph loads
- < 5ms for full atlas regeneration (cache clear)

---

## Testing Strategy

### New Tests Required
1. **Instance Rendering Tests**
   - Verify quad generation in shader
   - Compare output with current triangle approach
   - Test edge cases (zero-size glyphs, negative positions)

2. **Batch Accumulation Tests**
   - Verify batching across multiple calls
   - Test flush conditions
   - Verify ordering preservation

3. **Cache Tests**
   - Hash collision handling
   - LRU eviction correctness
   - Memory limits

4. **Compute Tests**
   - SDF accuracy vs CPU version
   - Performance benchmarks
   - Synchronization correctness

5. **Async Upload Tests**
   - Fence/semaphore correctness
   - Resource lifetime verification
   - Multi-threaded safety

### Performance Benchmarks
- Add to `tests/test_benchmark.c`:
  - `test_benchmark_text_instanced`
  - `test_benchmark_text_batched`
  - `test_benchmark_text_cached`
  - `test_benchmark_text_10k_glyphs`

---

## Risk Mitigation

### Compatibility Risks
- **Older GPUs**: Instancing requires Vulkan 1.0 (widely supported)
- **Compute shaders**: Requires Vulkan 1.0 (widely supported)
- **Async compute**: Not all GPUs have separate queues (fallback to graphics queue)

### Fallback Strategy
- Feature flags for each optimization
- Graceful degradation if unsupported
- Keep current path as fallback

### Validation
- Enable Vulkan validation layers during development
- Test on multiple GPU vendors (NVIDIA, AMD, Intel)
- Verify with RenderDoc frame captures

---

## Next Steps

1. Get user confirmation on approach
2. Start with Phase 1: Pre-warmed Atlas + Glyph Instancing
3. Benchmark at each step to measure improvement
4. Iterate based on profiling results
