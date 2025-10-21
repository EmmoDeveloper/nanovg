# NanoVG Vulkan Backend - Testing Strategy

## Current State

**Total Code**: ~3,518 lines across 10 header files
**Current Test Coverage**: ~38% (54 tests across 4 categories)

### Existing Tests (54 Total)

#### Smoke Tests (3)
| Test | Lines | Purpose |
|------|-------|---------|
| `test_compile.c` | 201 | Verifies library compiles |
| `test_simple.c` | 881 | Verifies API accessible |
| `test_init.c` | 8.3K | Verifies basic Vulkan setup |

#### Unit Tests (30)
| Test | Lines | Tests | Purpose |
|------|-------|-------|---------|
| `test_unit_texture.c` | 5.5K | 6 | Texture creation, updates, flags |
| `test_unit_platform.c` | 4.2K | 7 | GPU detection, context creation, memory |
| `test_unit_memory.c` | 8.3K | 7 | Memory allocation, stress testing, large textures |
| `test_unit_memory_leak.c` | 7.8K | 10 | Memory leak detection across all operations |

#### Integration Tests (19)
| Test | Lines | Tests | Purpose |
|------|-------|-------|---------|
| `test_integration_render.c` | 7.0K | 10 | Full rendering pipeline, shapes, gradients, transforms |
| `test_integration_text.c` | 7.8K | 9 | Text API, font sizes, rendering modes, alignment |

#### Benchmark Tests (5)
| Test | Lines | Tests | Purpose |
|------|-------|-------|---------|
| `test_benchmark.c` | 7.2K | 5 | Performance: shapes, paths, textures, gradients, state changes |

**Test Infrastructure**:
- Custom lightweight framework (3.9K) - zero dependencies
- Vulkan test utilities (5.0K) - context creation helpers

---

## Proposed Testing Architecture

### 1. Unit Tests (Target: 60% code coverage)

**Framework**: Custom lightweight framework (minimal dependencies)

**Test Modules**:

#### A. Memory Management (`test_unit_memory.c`)
- Buffer allocation/deallocation
- Memory type selection
- Staging buffer operations
- Memory leak detection

#### B. Image/Texture Operations (`test_unit_texture.c`)
- Texture creation (RGBA, alpha-only)
- Image format handling
- Texture updates
- Sampler configuration

#### C. Pipeline Management (`test_unit_pipeline.c`)
- Pipeline variant creation
- Pipeline caching
- Blend mode handling
- Dynamic state management

#### D. Platform Optimizations (`test_unit_platform.c`)
- GPU vendor detection
- GPU tier classification
- Platform-specific optimization selection
- Buffer size calculation

#### E. Threading (`test_unit_threading.c`)
- Work queue operations
- Thread pool initialization
- Thread synchronization
- Command buffer allocation

### 2. Integration Tests (Target: Core workflows)

#### A. Rendering Pipeline (`test_integration_render.c`)
- Complete frame rendering cycle
- Path filling (convex and concave)
- Stroke rendering
- Gradient rendering
- Texture-based fills

#### B. Text Rendering (`test_integration_text.c`)
- Standard text rendering
- SDF text rendering
- MSDF text rendering
- Subpixel text rendering
- Color emoji rendering

#### C. Multi-threading (`test_integration_threading.c`)
- Parallel command buffer recording
- Work queue distribution
- Thread pool stress testing

#### D. Resource Management (`test_integration_resources.c`)
- Multiple texture allocation
- Pipeline variant caching
- Resource cleanup
- Frame buffer cycling

### 3. Visual Regression Tests (Target: Correctness)

**Approach**: Render known scenes, compare against reference images

#### Test Scenes:
1. **Basic Shapes** - Rectangle, circle, rounded rectangle, ellipse
2. **Paths** - Complex concave paths, self-intersecting paths
3. **Gradients** - Linear, radial, with transforms
4. **Text** - All text rendering modes, Unicode
5. **Blending** - All 11 blend modes
6. **Transforms** - Rotation, scaling, skewing
7. **Clipping** - Scissor rectangles

**Implementation**:
```c
// Render to offscreen buffer
// Save as PNG
// Compare with reference PNG (pixel-by-pixel or perceptual hash)
```

### 4. Performance/Benchmark Tests

#### Metrics to Track:
- Frame time (CPU and GPU)
- Draw call count
- Vertices per frame
- Memory usage (vertex buffers, textures)
- Pipeline switches
- Thread utilization

#### Benchmark Scenarios:
1. **Simple UI** - 100 rectangles, 50 circles, 20 text labels
2. **Complex Paths** - 1000 Bezier curves
3. **Text Heavy** - 10,000 glyphs rendered
4. **Many Textures** - 500 textured quads
5. **Parallel Recording** - Multi-threaded command buffer generation

### 5. Stress Tests

#### A. Memory Pressure
- Allocate maximum textures
- Large vertex buffers
- Memory allocation failures

#### B. Long Running
- 10,000 frames without restart
- Check for memory leaks
- Resource exhaustion

#### C. Thread Safety
- Concurrent texture uploads
- Parallel pipeline creation
- Race condition detection

### 6. Error Handling Tests

- Invalid Vulkan handles
- Out of memory scenarios
- Missing Vulkan extensions
- Invalid function parameters
- Null pointer handling

---

## Testing Framework Design

### Lightweight Custom Framework

```c
// test_framework.h
#define TEST(name) static void test_##name(void)
#define ASSERT_EQ(a, b) if((a) != (b)) { test_fail(__LINE__, #a, #b); }
#define ASSERT_TRUE(x) if(!(x)) { test_fail(__LINE__, #x, "true"); }
#define ASSERT_NOT_NULL(x) if((x) == NULL) { test_fail(__LINE__, #x, "not NULL"); }

// Simple test runner
int run_tests(TestFunc* tests, int count);
```

**Benefits**:
- No external dependencies
- Easy to integrate
- Fast compilation
- Cross-platform

### Test Utilities

```c
// test_utils.h
typedef struct {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	VkCommandPool commandPool;
} TestVulkanContext;

// Setup/teardown helpers
TestVulkanContext* test_create_vulkan_context(void);
void test_destroy_vulkan_context(TestVulkanContext* ctx);
NVGcontext* test_create_nanovg_context(TestVulkanContext* vk);
```

---

## Implementation Plan

### Phase 1: Framework Setup (Immediate)
- [ ] Create test framework header (`tests/test_framework.h`)
- [ ] Create test utilities (`tests/test_utils.c/h`)
- [ ] Update Makefile/CMake for test builds
- [ ] Create test runner (`tests/run_all_tests.c`)

### Phase 2: Unit Tests (Week 1)
- [ ] Memory management tests
- [ ] Texture operation tests
- [ ] Pipeline management tests
- [ ] Platform optimization tests

### Phase 3: Integration Tests (Week 2)
- [ ] Rendering pipeline tests
- [ ] Text rendering tests
- [ ] Resource management tests

### Phase 4: Visual Regression (Week 3)
- [ ] Create reference images
- [ ] Implement image comparison
- [ ] Build test scene library

### Phase 5: Performance Tests (Week 4)
- [ ] Implement benchmark harness
- [ ] Create benchmark scenarios
- [ ] Performance regression detection

---

## CI/CD Integration

### Automated Testing

```yaml
# GitHub Actions example
test:
  - name: Compile tests
    run: make tests

  - name: Run unit tests
    run: ./run_unit_tests

  - name: Run integration tests (if GPU available)
    run: ./run_integration_tests

  - name: Generate coverage report
    run: gcov -r *.c
```

### Test Environments
- Linux (Mesa/NVIDIA/AMD)
- Windows (NVIDIA/AMD)
- macOS (MoltenVK)
- CI headless mode (SwiftShader)

---

## Success Metrics

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| Code Coverage | ~38% | 60%+ | 🟡 63% |
| Unit Tests | 30 | 50+ | 🟡 60% |
| Integration Tests | 19 | 20+ | 🟢 95% |
| Visual Tests | 0 | 15+ | 🔴 0% |
| Benchmark Tests | 5 | 10+ | 🟢 50% |
| Test Execution Time | ~20ms | <30s | 🟢 |
| **Total Tests** | **54** | **95+** | **🟡 57%** |

## Test Results (Latest Run)

**All 54 tests passing!**

### Unit Tests: Texture (6/6 ✓)
- ✓ texture_create_rgba
- ✓ texture_create_different_sizes
- ✓ texture_update_region
- ✓ texture_multiple_textures
- ✓ texture_flags
- ✓ texture_invalid_operations

### Unit Tests: Platform (7/7 ✓)
- ✓ platform_vulkan_available
- ✓ platform_physical_device_detected (RTX 3080 Laptop GPU detected)
- ✓ platform_graphics_queue_found
- ✓ platform_nanovg_context_creation
- ✓ platform_multiple_contexts
- ✓ platform_creation_flags
- ✓ platform_memory_properties

### Unit Tests: Memory (7/7 ✓)
- ✓ memory_context_lifecycle (10 cycles)
- ✓ memory_texture_allocation (50 textures)
- ✓ memory_large_textures (up to 2048x2048, 16MB)
- ✓ memory_vertex_buffer_stress (50 frames × 200 shapes)
- ✓ memory_texture_updates (100 updates)
- ✓ memory_multiple_contexts (5 simultaneous)
- ✓ memory_vulkan_properties (16GB device-local, 23.4GB system)

### Unit Tests: Memory Leak (10/10 ✓)
- ✓ leak_context_lifecycle (10 create/destroy cycles)
- ✓ leak_texture_operations (100 texture create/delete in 5 cycles)
- ✓ leak_texture_updates (100 updates to same texture)
- ✓ leak_rendering_operations (100 frames with 80 shapes each)
- ✓ leak_multiple_contexts (15 contexts across 3 cycles)
- ✓ leak_font_operations (50 frames with text API)
- ✓ leak_path_operations (1000 complex Bezier paths)
- ✓ leak_gradient_operations (1000 linear/radial gradients)
- ✓ leak_transform_operations (1000 transform operations)
- ✓ leak_scissor_operations (500 scissor clip operations)

### Integration Tests: Render (10/10 ✓)
- ✓ render_begin_end_frame
- ✓ render_rectangle
- ✓ render_circle
- ✓ render_stroke
- ✓ render_multiple_shapes
- ✓ render_linear_gradient
- ✓ render_radial_gradient
- ✓ render_transforms
- ✓ render_multiple_frames
- ✓ render_scissor

### Integration Tests: Text (9/9 ✓)
- ✓ text_font_loading
- ✓ text_different_sizes (5 sizes tested)
- ✓ text_rendering_modes (all 5 modes: standard, SDF, MSDF, subpixel, color)
- ✓ text_alignment (9 alignment combinations)
- ✓ text_metrics
- ✓ text_transformations
- ✓ text_multiple_calls (10 frames × 50 calls)
- ✓ text_blur_effect (5 blur levels)
- ✓ text_colors (10 different colors)

### Benchmark Tests (5/5 ✓)
| Benchmark | Frames | Items/Frame | Avg Frame | FPS |
|-----------|--------|-------------|-----------|-----|
| Simple Shapes | 100 | 100 shapes | 0.04 ms | 27,473 |
| Complex Paths | 50 | 50 paths | 0.04 ms | 22,486 |
| Textures | 100 | 50 quads | 0.02 ms | 47,518 |
| Gradients | 100 | 50 fills | 0.02 ms | 47,550 |
| State Changes | 100 | 100 changes | 0.04 ms | 24,788 |

**Hardware**: NVIDIA GeForce RTX 3080 Laptop GPU

---

## Notes

- **GPU Required**: Integration and visual tests require GPU access
- **Headless Testing**: Use SwiftShader or LavaPipe for CI without GPU
- **Visual Tests**: May need tolerance for minor GPU driver differences
- **Performance**: Baseline on specific hardware (e.g., RTX 3080)
