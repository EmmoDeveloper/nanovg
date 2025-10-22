# Deep Integration Technical Specification

## Problem Statement

The virtual atlas header (`nanovg_vk_virtual_atlas.h`) was updated during Phase 3 integration to include:
- `VKNVGatlasManager* atlasManager` - for Guillotine packing and multi-atlas
- `VKNVGdefragContext defragContext` - for defragmentation

However, the implementation (`nanovg_vk_virtual_atlas.c`) still uses the old page-based system with:
- `freePageList[]` - array of free page indices
- `pages[]` - array of page structures
- `vknvg__allocatePage()` - page allocation function

**These old page fields don't even exist in the current header**, indicating a mismatch between header and implementation.

## Root Cause

During Phase 3 integration, only the header was updated with placeholder structures. The actual implementation was never refactored to use them. This is technical debt that needs resolution for production use.

## Solution Approach

### Option 1: Incremental Refactoring (RECOMMENDED)
Gradually replace page system with Phase 3/4 features while keeping tests passing.

**Steps:**
1. Add atlasManager initialization to `vknvg__createVirtualAtlas()`
2. Keep both systems running in parallel temporarily
3. Replace `vknvg__allocatePage()` calls with `vknvg__multiAtlasAlloc()`
4. Remove page system once Guillotine is working
5. Add Phase 4 contexts
6. Wire everything together

**Pros:** Lower risk, tests keep passing
**Cons:** Temporary code duplication

### Option 2: Complete Rewrite
Rewrite virtual_atlas.c from scratch using Phase 3/4.

**Pros:** Clean implementation
**Cons:** High risk, all tests break initially

## Recommended Implementation Plan

### Phase 1: Add atlasManager (Keep page system)
```c
// In vknvg__createVirtualAtlas()
atlas->atlasManager = (VKNVGatlasManager*)calloc(1, sizeof(VKNVGatlasManager));
vknvg__initAtlasManager(atlas->atlasManager,
                         4096, 4096,  // Start with current size
                         VKNVG_PACK_BEST_AREA_FIT,
                         VKNVG_SPLIT_SHORTER_AXIS);
```

### Phase 2: Dual allocation (Test Guillotine)
```c
static uint16_t vknvg__allocateSpace(VKNVGvirtualAtlas* atlas,
                                     uint16_t width, uint16_t height,
                                     uint16_t* outX, uint16_t* outY)
{
    // Try Guillotine first
    uint32_t atlasIndex;
    VKNVGrect rect;
    if (vknvg__multiAtlasAlloc(atlas->atlasManager, width, height,
                               &atlasIndex, &rect)) {
        *outX = rect.x;
        *outY = rect.y;
        return 1;
    }

    // Fallback to page system (temporary)
    return vknvg__allocatePage(atlas);
}
```

### Phase 3: Remove page system
Once Guillotine is proven working, remove:
- `freePageList`, `pages[]`
- `vknvg__allocatePage()`
- Page-based calculations

### Phase 4: Add Phase 4 contexts
```c
// Add to structure (already in header as placeholders):
VKNVGharfbuzzContext* harfbuzz;
VKNVGbidiContext* bidi;
VKNVGintlTextContext* intlText;

// Initialize in create:
if (enableInternationalText) {
    atlas->harfbuzz = vknvg__createHarfBuzzContext();
    atlas->bidi = vknvg__createBidiContext();
    atlas->intlText = vknvg__createIntlTextContext();
}
```

### Phase 5: Wire defragmentation
```c
// In vknvg__atlasNextFrame():
if (atlas->enableDefrag && atlas->atlasManager) {
    if (vknvg__shouldDefragmentAtlas(&atlas->atlasManager->atlases[0])) {
        vknvg__startDefragmentation(&atlas->defragContext, ...);
    }
    vknvg__updateDefragmentation(&atlas->defragContext, ...);
}
```

## Key Functions to Modify

### 1. `vknvg__createVirtualAtlas()`
- Initialize `atlasManager`
- Initialize `defragContext`
- Initialize Phase 4 contexts (optional)
- Remove page system initialization

### 2. `vknvg__destroyVirtualAtlas()`
- Destroy `atlasManager`
- Destroy Phase 4 contexts
- Remove page cleanup

### 3. `vknvg__allocatePage()` → REPLACE WITH
`vknvg__allocateGlyphSpace(atlas, width, height, &atlasIndex, &x, &y)`

### 4. `vknvg__lookupGlyph()` / `vknvg__requestGlyph()`
- Update to use `atlasIndex` instead of page
- Update atlas coordinates

### 5. `vknvg__atlasNextFrame()`
- Add defragmentation check
- Update defragmentation state

### 6. `vknvg__processUploads()`
- Handle multi-atlas uploads
- Target correct atlas image

## Testing Strategy

### Existing Tests Must Pass
- All 98 current tests must continue passing
- No regressions in functionality

### New Integration Tests
```c
// test_deep_integration.c
void test_guillotine_allocation()
{
    // Test that Guillotine packing works with virtual atlas
}

void test_multi_atlas_overflow()
{
    // Test atlas creation when primary is full
}

void test_dynamic_growth()
{
    // Test resizing from 512→1024→2048→4096
}

void test_defragmentation_integration()
{
    // Test defrag runs without breaking glyph cache
}

void test_shaped_text_rendering()
{
    // Test HarfBuzz integration
}

void test_bidi_text_rendering()
{
    // Test BiDi reordering
}
```

## Success Criteria

- ✅ All 98 existing tests pass
- ✅ Packing efficiency > 85% (vs ~60% with pages)
- ✅ Memory starts at 256KB (with dynamic growth)
- ✅ Multi-atlas scales to 65,000+ glyphs
- ✅ Defragmentation runs without blocking (< 2ms/frame)
- ✅ Arabic/Hebrew text renders correctly
- ✅ No memory leaks (Valgrind clean)

## Estimated Effort

- Header additions: 30 minutes
- Create function refactoring: 1 hour
- Allocation replacement: 2 hours
- Upload logic update: 1 hour
- Defragmentation wiring: 1 hour
- Phase 4 integration: 2 hours
- Testing and debugging: 2-3 hours

**Total: 9-10 hours**

## Next Steps

1. Review this spec
2. Begin Phase 1 (Add atlasManager initialization)
3. Test incrementally
4. Continue through phases
5. Final integration testing

Would you like me to proceed with Phase 1?
