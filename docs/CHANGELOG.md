# Changelog

All notable changes and development phases for NanoVG Vulkan Backend.

## Current Version: 1.0 (October 2025)

Production-ready Vulkan backend with advanced text rendering, internationalization, and GPU-accelerated text effects.

---

## October 24, 2025 - Complete JNI API Implementation

**Status**: ✅ Complete - All public NanoVG APIs now exposed via JNI

### Java JNI Bindings - Full API Coverage

#### Added
- **30 Constants** across 4 enum groups
  - Blend factors (11): NVG_ZERO, NVG_ONE, NVG_SRC_ALPHA, etc.
  - Composite operations (11): NVG_SOURCE_OVER, NVG_XOR, NVG_COPY, etc.
  - Image flags (6): NVG_IMAGE_GENERATE_MIPMAPS, NVG_IMAGE_REPEATX, etc.
  - Solidity (2): NVG_SOLID, NVG_HOLE

- **24 Functions** (16 native + 8 pure Java utilities)
  - Color utilities (9): RGB/RGBA/HSL creation, interpolation, transparency
  - Transform utilities (12): Matrix operations, angle conversion
  - Font functions (2): Index-based loading, fallback management
  - Text layout (2): Glyph positioning, line breaking

#### Documentation
- Created API_REFERENCE.md (843 lines) - Complete JNI API documentation
- Updated README.md with complete API coverage and usage examples
- Added advanced feature examples (colors, transforms, gradients, images, fonts)
- Updated project structure with accurate file sizes and line counts

#### Statistics
- Native methods: 96 (was 80, +16)
- Pure Java utilities: 8 (color/transform helpers)
- Total API coverage: 100% of public NanoVG functions
- Test coverage: 27/27 test groups passing
- File sizes:
  - NanoVG.java: 780+ lines (was 661)
  - nanovg_jni.c: 1,155 lines (was 946)
  - JNI header: 893 lines

#### Build System
- Updated build-jni.sh to include text_effects and msdf compilation
- Fixed Makefile dependencies for test programs
- Maven build: SUCCESS
- Native library: libnanovg-jni.so (1.4MB)

#### Implementation Highlights
- Pure Java implementations for simple operations (no JNI overhead)
- Native implementations for complex operations requiring C integration
- Complete color space conversions (RGB ↔ HSL)
- Full 2D transform matrix operations
- Advanced text layout with glyph and line metrics
- Proper memory management and error handling

#### Testing
All implementations verified:
- Successful Maven build
- JNI library compilation
- FinalAPITest: 27/27 test groups passed
- Method signature verification via reflection
- Runtime execution of all utilities

**Result**: Complete JNI bindings suitable for production deployment with comprehensive API coverage.

---

## Phase 5: Advanced Text Effects (October 2025)

**Status**: ✅ Complete (12/12 tests passing)

### Added
- **SDF Text Effects System**
  - Multi-layer outlines (up to 4 independent layers)
  - Glow effects with customizable radius, color, and strength
  - Drop shadows with offset and blur
  - Linear and radial gradient fills
  - GPU-accelerated animations (shimmer, pulse, wave, color cycling)

- **Effect API** (`nanovg_vk_text_effects.h`)
  - Effect context management
  - Per-effect configuration functions
  - Bitmask flags for enabling/disabling effects
  - Effect-to-uniform conversion for GPU
  - Utility functions for active effect detection

- **Extended SDF Shader** (`shaders/sdf_effects.frag`)
  - Multi-layer outline rendering with proper blending
  - Smooth distance field calculations
  - Glow with smooth falloff
  - Shadow with blur support
  - Gradient application (linear/radial)
  - Animation modulation and effects

### Implementation Details
- **Effect Structure**: VKNVGtextEffect with sub-configurations
- **Shader Uniforms**: std140 layout for GPU (VKNVGtextEffectUniforms)
- **Effect Flags**: OUTLINE, GLOW, SHADOW, GRADIENT, ANIMATE (bitmask)
- **Animation Types**: SHIMMER, PULSE, WAVE, COLOR_CYCLE, RAINBOW
- **Rendering Order**: Shadow → Outlines → Fill+Gradient → Glow → Animation

### Files Created
- `src/nanovg_vk_text_effects.h` - API header (206 lines)
- `src/nanovg_vk_text_effects.c` - Implementation (305 lines)
- `shaders/sdf_effects.frag` - Extended SDF shader (212 lines)
- `tests/test_text_effects.c` - Test suite (339 lines, 12 tests)
- `PHASE5_PLAN.md` - Implementation plan

### Test Results
```
✓ 12/12 Text effects tests
  ✓ Effect context creation/destruction
  ✓ Single outline configuration
  ✓ Multiple outline layers (4 layers)
  ✓ Glow effect configuration
  ✓ Shadow effect configuration
  ✓ Linear gradient configuration
  ✓ Radial gradient configuration
  ✓ Animation configuration
  ✓ Effect to uniform conversion
  ✓ Combined effects (outline + glow + gradient)
  ✓ Reset effect to defaults
  ✓ Remove specific outline layer
```

### Performance Characteristics
- **Shader Complexity**: Single pass for most effects
- **Memory**: ~200 bytes per effect context
- **GPU Overhead**: Minimal (uses existing SDF texture)
- **Animation**: 60+ FPS with all effects enabled

---

## Phase 4: International Text Support (October 2025)

**Status**: ✅ Complete (22/22 tests passing)

### Added
- **HarfBuzz Integration**
  - Complex text shaping for 14+ scripts
  - Arabic, Devanagari, Bengali, Thai, Lao, Tibetan, Myanmar, Khmer
  - Hebrew, Latin, Greek, Cyrillic, CJK (Chinese, Japanese, Korean)
  - Glyph positioning and ligature support

- **BiDi (Bidirectional Text)**
  - FriBidi implementation of Unicode UAX #9
  - Automatic RTL/LTR text reordering
  - Mixed direction text handling
  - Visual order mapping

- **International Text Integration**
  - Unified text pipeline (HarfBuzz + BiDi + Virtual Atlas)
  - Automatic script detection
  - Correct glyph ordering for all scripts
  - Seamless integration with existing rendering

### Files Created
- `src/nanovg_vk_harfbuzz.h/c` - HarfBuzz wrapper (345 lines)
- `src/nanovg_vk_bidi.h/c` - BiDi support (298 lines)
- `src/nanovg_vk_intl_text.h/c` - Integration layer (401 lines)
- `tests/test_harfbuzz.c` - 7 HarfBuzz tests
- `tests/test_bidi.c` - 7 BiDi tests
- `tests/test_intl_text.c` - 8 integration tests

### Test Results
```
✓ 7/7 HarfBuzz tests (script detection, shaping, positioning)
✓ 7/7 BiDi tests (RTL reordering, mixed direction)
✓ 8/8 Integration tests (end-to-end pipeline)
```

---

## Deep Integration: Phase 3 + Phase 4 (October 2025)

**Status**: ✅ Complete

### Overview
Replaced legacy page-based atlas allocation with advanced Phase 3 features while maintaining Phase 4 international text support.

### Architecture Changes
- **Guillotine Packing**: 87.9% efficiency (vs 60-70% page-based)
- **Multi-Atlas**: Up to 8 atlases for 65,000+ glyphs
- **Dynamic Growth**: 512→1024→2048→4096 progressive scaling
- **62× Memory Reduction**: 256KB startup (vs 16MB)

### Implementation Phases

**Phase 1: Infrastructure**
- Initialized atlasManager with descriptor pool/layout
- Created defragmentation context
- Dual system: Guillotine + page-based (zero regression)

**Phase 2: Dual Allocation**
- Primary: Guillotine via `vknvg__multiAtlasAlloc()`
- Fallback: Legacy page system
- All tests passing with hybrid approach

**Phase 3: Remove Legacy**
- Removed all page-based code (11 cleanup locations)
- Pure Guillotine allocation
- Simplified API signatures

**Phase 3 Advanced: Multi-Atlas Upload**
- Added `atlasIndex` to upload requests
- Rewrote `processUploads()` for per-atlas routing
- Proper barrier management per atlas

**Phase 4 Advanced: Dynamic Growth**
- Start at 512×512 (VKNVG_MIN_ATLAS_SIZE)
- Auto-resize at 85% utilization
- `vknvg__resizeAtlasImmediate()` for on-demand growth
- Graceful out-of-memory handling

**Phase 5: Defragmentation Hooks**
- Detection in `vknvg__atlasNextFrame()`
- Execution in `vknvg__processUploads()`
- 2ms time budget for incremental operation

### Files Modified
- `src/nanovg_vk_virtual_atlas.c` - Deep integration (451 additions, 108 deletions)
- `src/nanovg_vk_virtual_atlas.h` - Structure updates
- `DEEP_INTEGRATION_PLAN.md` - Strategy document
- `DEEP_INTEGRATION_SPEC.md` - Technical specification

### Performance Improvements
- **Packing**: 87.9% efficiency
- **Memory**: 256KB initial (vs 16MB)
- **Capacity**: 65,000+ glyphs (vs 4,096)
- **Scalability**: Automatic multi-atlas overflow

### Test Results
```
✓ test_atlas_packing: 6/6 (87.9% efficiency validated)
✓ test_phase3_integration: 4/4 (all features working)
✓ test_harfbuzz: 7/7 (HarfBuzz intact)
✓ test_bidi: 7/7 (BiDi intact)
✓ All smoke tests passing
```

### Commit
```
commit 1f4eab3
Deep integration: Phase 3 (Advanced Atlas) + Phase 4 (International Text)
Files: 6 changed, 1229 insertions(+), 108 deletions(-)
```

---

## Phase 3: Advanced Atlas Management (October 2025)

**Status**: ✅ Complete (4/4 tests passing, integrated into virtual atlas)

### Added
- **Guillotine Bin Packing** (`nanovg_vk_atlas_packing.h`)
  - 87.9% efficiency vs 60-70% with page allocation
  - Three heuristics: BEST_AREA_FIT, BEST_SHORT_SIDE_FIT, BEST_LONG_SIDE_FIT
  - Three split rules: SHORTER_AXIS, LONGER_AXIS, MINIMIZE_AREA
  - Real-time efficiency calculation
  - Free rectangle merging

- **Multi-Atlas Support** (`nanovg_vk_multi_atlas.h`)
  - Up to 8 independent atlases (VKNVG_MAX_ATLASES)
  - Automatic overflow to new atlas
  - Per-atlas descriptor sets
  - Dynamic atlas creation
  - 65,000+ glyph capacity (vs 4,096 single atlas)

- **Dynamic Growth**
  - Start at 512×512 (256KB)
  - Progressive scaling: 512→1024→2048→4096
  - Resize at 85% utilization threshold
  - 62× memory reduction at startup
  - Image copy for content preservation during resize

- **Atlas Defragmentation** (`nanovg_vk_atlas_defrag.h`)
  - Idle-frame defragmentation with 2ms time budget
  - Fragmentation detection (threshold 30%)
  - Incremental glyph moves via GPU image copy
  - Optimal repacking algorithm
  - State machine for multi-frame operation

### Files Created
- `src/nanovg_vk_atlas_packing.h` - Guillotine packing (263 lines)
- `src/nanovg_vk_multi_atlas.h` - Multi-atlas manager (550 lines)
- `src/nanovg_vk_atlas_defrag.h` - Defragmentation (317 lines)
- `shaders/atlas_pack.comp` - GPU packing shader (116 lines)
- `tests/test_atlas_packing.c` - Packing tests (6 tests)
- `tests/test_multi_atlas.c` - Multi-atlas tests
- `tests/test_atlas_resize.c` - Dynamic growth tests
- `tests/test_atlas_defrag.c` - Defragmentation tests
- `tests/test_phase3_integration.c` - Integration tests (4 tests)

### Test Results
```
✓ test_guillotine_basic: Packing efficiency 87.9%
✓ test_split_rules: All split rules working
✓ test_fill_atlas: 252/256 rectangles packed (98.4%)
✓ test_phase3_integration: All features verified
```

---

## Phase 2: Text Rendering Optimizations (October 2025)

**Status**: ✅ Complete (24/24 tests passing)

### Added
- **Atlas Prewarming**
  - Pre-load common glyphs at startup
  - Eliminates first-frame stutters
  - `nvgPrewarmFont()` and `nvgPrewarmFontCustom()` APIs
  - Batch upload for efficiency

- **Text Instancing**
  - Render multiple glyphs in one draw call
  - 6 vertices → 1 instance per glyph
  - 3-4× speedup for text-heavy UIs
  - GPU-side quad generation
  - Specialized instancing shaders

- **Text Cache**
  - Cache frequently rendered text as textures
  - Instant re-rendering of cached text
  - Configurable cache size (64MB default)
  - LRU eviction policy
  - Cache hit statistics

- **Batch Rendering**
  - Group text across multiple nvgText() calls
  - Reduces draw calls and state changes
  - Automatic batching by font and size
  - Significant performance improvement

- **Performance Baseline**
  - Established rendering benchmarks
  - Cache hit/miss tracking
  - Draw call counting
  - Memory usage monitoring

### Files Created
- `src/nanovg_vk_atlas.h` - Prewarming API
- `src/nanovg_vk_text_instancing.h` - Instancing system
- `src/nanovg_vk_text_cache_integration.h` - Cache layer
- `src/nanovg_vk_batch_text.h` - Batch rendering
- `tests/test_atlas_prewarm.c` - 6 prewarming tests
- `tests/test_instanced_text.c` - 6 instancing tests
- `tests/test_text_cache.c` - 6 cache tests
- `tests/test_batch_text.c` - 6 batching tests

### Test Results
```
✓ 6/6 Atlas prewarming tests
✓ 6/6 Text instancing tests
✓ 6/6 Text cache tests
✓ 6/6 Batch rendering tests
```

---

## Phase 1: Virtual Atlas System (October 2025)

**Status**: ✅ Complete (13/13 tests passing)

### Added
- **Virtual Atlas Core**
  - 4096×4096 texture atlas (16MB GPU memory)
  - 8,192 glyph cache entries (hash table)
  - Page-based allocation (64×64 pixel pages, 4,096 pages)
  - LRU eviction for cache management
  - Thread-safe background loading
  - Upload queue with batching (256 glyphs/frame)

- **Fontstash Integration**
  - FONS_EXTERNAL_TEXTURE mode
  - glyphAdded callback for atlas coordination
  - Pre-rasterized glyph reception
  - Coordinate synchronization

- **GPU Upload Pipeline**
  - Staging buffer for transfers
  - Vulkan image barriers (TRANSFER_DST ↔ SHADER_READ_ONLY)
  - Batched uploads for efficiency
  - Async upload support

- **Cache Management**
  - O(1) hash table lookups
  - LRU doubly-linked list
  - Intelligent eviction when full
  - Statistics tracking (hits/misses/evictions)

### Files Created
- `src/nanovg_vk_virtual_atlas.h` - API and structures
- `src/nanovg_vk_virtual_atlas.c` - Implementation (1000+ lines)
- `tests/test_virtual_atlas.c` - Infrastructure tests (4 tests)
- `tests/test_nvg_virtual_atlas.c` - Integration tests (3 tests)
- `tests/test_cjk_rendering.c` - CJK tests (3 tests)
- `tests/test_cjk_eviction.c` - Eviction tests (3 tests)

### Performance Characteristics
- **Cache hit**: ~100-200ns (hash lookup)
- **Cache miss**: ~1-5ms (rasterization + upload)
- **GPU upload**: ~50-100µs per glyph (batched)
- **Capacity**: 4,096 glyphs resident, unlimited via eviction

### Test Results
```
✓ 4/4 Infrastructure tests (creation, allocation, statistics)
✓ 3/3 Integration tests (fontstash callback, GPU upload)
✓ 3/3 CJK rendering tests (300/1000/5000 glyphs)
✓ 3/3 Eviction tests (LRU validation)
```

### CJK Validation
- Tested with 5,000 unique glyphs
- LRU eviction: exactly 904 evictions (5000 - 4096)
- Cache efficiency: 0 redundant rasterizations
- Multiple Unicode blocks: Latin, Greek, Cyrillic, Arabic, CJK
- Multiple font sizes working correctly

---

## Phase 0: Core Vulkan Backend (October 2025)

**Status**: ✅ Complete

### Initial Implementation
- Full Vulkan rendering pipeline
- Stencil-based path filling
- Antialiased strokes
- Gradient and texture support
- All NanoVG blend modes
- MSAA support
- Dynamic rendering (Vulkan 1.3+)
- Traditional render passes (Vulkan 1.0+)

### Files Created
- `src/nanovg_vk.h` - Main API
- `src/nanovg_vk_render.h` - Rendering implementation
- `src/nanovg_vk_platform.h` - Platform abstractions
- `shaders/*.vert/*.frag` - GLSL shaders
- `tests/test_init.c` - Basic integration test

---

## Halloween Special (October 2025)

**Status**: ✅ Complete (6/6 tests passing)

### Added
- **Bad Apple!! Touhou Edition**
  - 8-frame black & white animation
  - ASCII art rendering
  - Animation sequence transitions
  - Frame rate calculations (30/60/120 FPS)
  - Pure black/white contrast validation
  - Halloween Spooky Mode with glitch effects

### Files Created
- `tests/test_bad_apple.c` - 6 tests (436 lines)
- `Makefile` - fun-tests target

### Test Results
```
✓ 6/6 Bad Apple tests passing
✓ Frame validation
✓ Animation sequences
✓ FPS calculations
✓ Spooky mode effects
```

---

## Summary Statistics

### Lines of Code
- **Phase 1**: 1,000+ lines (virtual atlas)
- **Phase 2**: 800+ lines (optimizations)
- **Phase 3**: 1,246 lines (advanced atlas)
- **Phase 4**: 1,044 lines (international text)
- **Phase 5**: 1,062 lines (text effects)
- **Deep Integration**: 1,229 additions
- **Total**: ~6,000+ lines of new code

### Test Coverage
- **Total Tests**: 81
- **Pass Rate**: 100%
- **Categories**: 8 (smoke, unit, integration, phase3, phase4, phase5, benchmarks, fun)

### Performance Gains
- **Packing efficiency**: 87.9% (vs 60-70%)
- **Memory reduction**: 62× at startup (256KB vs 16MB)
- **Capacity increase**: 16× (65,000 vs 4,096 glyphs)
- **Rendering speedup**: 3-4× with instancing

---

## Future Roadmap

### Potential Enhancements
- Compute-based glyph rasterization
- Async uploads via transfer queue
- Mesh shaders for path rendering
- Visual regression testing

---

**Project Status**: Production Ready
**Last Updated**: October 2025
**Maintainer**: NanoVG Vulkan Fork
