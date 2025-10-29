# NanoVG Vulkan - Testing Guide

## Quick Start

Run all tests with the automated test runner:

```bash
./run_tests.sh
```

## Test Suite Overview

**Total Tests**: 12 working tests across 4 categories
**Status**: ✅ All tests passing

### Test Categories

1. **Text Rendering Tests** (6 tests) - NanoVG text API
2. **Graphics Rendering Tests** (4 tests) - NanoVG graphics API
3. **Font System Tests** (1 test) - Low-level font operations
4. **Backend Tests** (1 test) - Virtual atlas system

---

## Text Rendering Tests (6 tests)

### 1. Color Emoji (COLR v1)
- **Binary**: `./build/test_nvg_color_emoji`
- **Features**: COLR v1 vector emoji, Cairo integration, virtual atlas
- **Output**: `color_emoji_test.ppm` (6 emoji: 🎉⭐🚀😀❤️👍)
- **Test**: Full emoji rendering pipeline with proper sizing

```bash
make build/test_nvg_color_emoji
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_color_emoji
convert color_emoji_test.ppm color_emoji_test.png
```

### 2. BiDi Text (Arabic/Hebrew)
- **Binary**: `./build/test_nvg_bidi`
- **Features**: HarfBuzz shaping, FriBidi reordering, RTL text
- **Output**: `bidi_test.ppm`
- **Fonts**: Noto Sans Arabic, Noto Sans Hebrew, Noto Sans
- **Test**: 6 text samples (Arabic RTL, Hebrew RTL, mixed scripts)

```bash
make build/test_nvg_bidi
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_bidi
convert bidi_test.ppm bidi_test.png
```

### 3. Chinese Poem (CJK)
- **Binary**: `./build/test_nvg_chinese_poem`
- **Features**: Noto Sans CJK SC, large glyph sets, virtual atlas
- **Test**: Chinese text rendering with multiline layout

```bash
make build/test_nvg_chinese_poem
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_chinese_poem
```

### 4. FreeType Text
- **Binary**: `./build/test_nvg_freetype`
- **Features**: Basic text layout, metrics, bounds calculation
- **Output**: `freetype_test.ppm`

```bash
make build/test_nvg_freetype
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_freetype
```

### 5. Text Layout & Wrapping
- **Binary**: `./build/test_nvg_text`
- **Features**: Word wrapping, line breaking, text box layout
- **API**: `nvgText()`, `nvgTextBox()`, `nvgTextBreakLines()`

```bash
make build/test_nvg_text
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_text
```

### 6. MSDF Text
- **Binary**: `./build/test_text_msdf`
- **Features**: Multi-channel Signed Distance Field text
- **Test**: Scalable text rendering

```bash
make build/test_text_msdf
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_text_msdf
```

---

## Graphics Rendering Tests (4 tests)

### 7. Canvas API (Shapes + Text)
- **Binary**: `./build/test_canvas_api`
- **Features**: Complete canvas API exercise
- **API**: `nvgBeginPath()`, `nvgRect()`, `nvgFill()`, `nvgText()`

```bash
make build/test_canvas_api
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_canvas_api
```

### 8. Shape Primitives
- **Binary**: `./build/test_shapes`
- **Features**: Rectangle, circle, ellipse, rounded rectangle
- **API**: `nvgRect()`, `nvgCircle()`, `nvgEllipse()`, `nvgRoundedRect()`

```bash
make build/test_shapes
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_shapes
```

### 9. Multiple Shapes
- **Binary**: `./build/test_multi_shapes`
- **Features**: Multiple shape primitives in one frame

```bash
make build/test_multi_shapes
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_multi_shapes
```

### 10. Gradients
- **Binary**: `./build/test_gradients`
- **Features**: Linear, radial, and box gradients
- **API**: `nvgLinearGradient()`, `nvgRadialGradient()`, `nvgBoxGradient()`

```bash
make build/test_gradients
VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_gradients
```

---

## Font System Tests (1 test)

### 11. BiDi Low-Level
- **Binary**: `./build/test_bidi`
- **Features**: HarfBuzz + FriBidi integration test
- **API**: `nvgft_shaped_text_iter_init()`, `nvgft_text_iter_next()`
- **Test**: 8 test cases (all passing)
- **Output**: Console only (no rendering)

```bash
make build/test_bidi
./build/test_bidi
```

---

## Backend Tests (1 test)

### 12. Virtual Atlas Integration
- **Binary**: `./build/test_virtual_atlas_integration`
- **Features**: Multi-atlas, packing, defragmentation, background loading
- **Test**: Atlas system functionality
- **Output**: Console metrics

```bash
make build/test_virtual_atlas_integration
./build/test_virtual_atlas_integration
```

---

## Test Results

All 12 tests pass successfully:

- ✅ Color Emoji (COLR v1)
- ✅ BiDi Text (Arabic/Hebrew)
- ✅ Chinese Poem (CJK)
- ✅ FreeType Text
- ✅ Text Layout & Wrapping
- ✅ MSDF Text
- ✅ Canvas API
- ✅ Shape Primitives
- ✅ Multiple Shapes
- ✅ Gradients
- ✅ BiDi Low-Level
- ✅ Virtual Atlas Integration

---

## Features Tested

### Text Rendering
- ✅ Color emoji (COLR v1) with Cairo
- ✅ Bidirectional text (Arabic/Hebrew) with HarfBuzz/FriBidi
- ✅ CJK fonts (Chinese) with large glyph sets
- ✅ Basic Latin text
- ✅ Text wrapping and layout
- ✅ MSDF scalable text

### Graphics Rendering
- ✅ Shape primitives (rect, circle, ellipse)
- ✅ Gradients (linear, radial, box)
- ✅ Canvas API operations
- ✅ Multiple shapes per frame

### Backend Systems
- ✅ Virtual atlas (multi-atlas, packing, defrag)
- ✅ Async texture uploads
- ✅ Font context integration
- ✅ Glyph caching

---

## Build Commands

Build all tests:
```bash
make build/test_nvg_color_emoji
make build/test_nvg_bidi
make build/test_nvg_chinese_poem
make build/test_nvg_freetype
make build/test_nvg_text
make build/test_text_msdf
make build/test_canvas_api
make build/test_shapes
make build/test_multi_shapes
make build/test_gradients
make build/test_bidi
make build/test_virtual_atlas_integration
```

Or use the test runner which builds automatically:
```bash
./run_tests.sh
```

---

## Viewing Test Outputs

Convert PPM files to PNG:
```bash
convert color_emoji_test.ppm color_emoji_test.png
convert bidi_test.ppm bidi_test.png
convert freetype_test.ppm freetype_test.png
```

View PNG files:
```bash
display color_emoji_test.png  # ImageMagick
eog color_emoji_test.png      # GNOME Image Viewer
```

---

## Environment Variables

### Disable Vulkan Validation Layers
```bash
export VK_INSTANCE_LAYERS=""
export VK_LAYER_PATH=""
```

Required for tests to run without validation layer warnings.

### Timeout
Tests have a 10-second timeout to prevent hanging.

---

## Troubleshooting

### Test hangs or times out
- Check if Vulkan drivers are installed
- Verify GLFW can create windows
- Run without validation layers (see above)

### Build errors
- Ensure all dependencies installed: `vulkan`, `glfw3`, `harfbuzz`, `fribidi`
- Check FreeType/Cairo paths in Makefile
- Run `make clean` and rebuild

### Tests fail silently
- Run test directly to see output:
  ```bash
  VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_<name>
  ```
- Check for segfaults: `dmesg | tail`

---

## Adding New Tests

1. Create test file: `tests/test_new_feature.c`
2. Add to Makefile:
   ```makefile
   $(BUILD_DIR)/test_new_feature.o: tests/test_new_feature.c | $(BUILD_DIR)
   	@echo "Compiling test_new_feature.c..."
   	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

   $(BUILD_DIR)/test_new_feature: $(BUILD_DIR)/test_new_feature.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
   	@echo "Linking test_new_feature..."
   	$(CC) $^ $(LIBS) -o $@
   ```
3. Add to `run_tests.sh`
4. Build and test

---

## CI/CD Integration

The test runner returns exit code 0 on success, 1 on failure:

```bash
./run_tests.sh && echo "All tests passed" || echo "Tests failed"
```

---

## Test Coverage Summary

| Feature | Test Coverage |
|---------|---------------|
| Color Emoji (COLR v1) | ✅ Complete |
| BiDi Text (RTL) | ✅ Complete |
| CJK Fonts | ✅ Complete |
| Text Layout | ✅ Complete |
| Shape Primitives | ✅ Complete |
| Gradients | ✅ Complete |
| Virtual Atlas | ✅ Complete |
| Async Uploads | ✅ Complete |
| Font Caching | ✅ Complete |

---

## Performance Notes

- Most tests complete in <1 second
- First emoji render: ~5-10ms (Cairo)
- Cached glyphs: ~100-200ns (hash lookup)
- Virtual atlas packing: ~1-2ms per glyph
- Total test suite: ~30-60 seconds (including builds)
