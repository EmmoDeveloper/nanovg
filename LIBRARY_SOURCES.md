# Library Source Access

## Overview

This project has **full read access** to all library sources used in the build. Some libraries also have **write access** for modifications and customization.

## Library Locations and Access

### FreeType 2.13.4
- **Source Location**: `/opt/freetype/`
- **Build Directory**: `/opt/freetype/objs/.libs/`
- **Library**: `libfreetype.so.6.20.4`
- **Headers**: `/opt/freetype/include/`
- **Access**: Read access to source code (write access only for building)
- **Features**: Built with PNG support and COLR table detection

### Cairo 1.18.5
- **Source Location**: `/opt/freetype/cairo/`
- **Build Directory**: `/opt/freetype/cairo/builddir/src/`
- **Library**: `libcairo.so.2.11805.5`
- **Headers**: `/opt/freetype/cairo/src/`
- **Access**: Read access to source code (write access only for building)
- **Features**: Built with COLR v1 support (`HAVE_FT_COLR_V1`)
- **Key Files**:
  - `src/cairo-ft-font.c` - FreeType integration and COLR rendering
  - `builddir/config.h` - Build configuration

### Noto Emoji Fonts
- **Location**: `/opt/noto-emoji/`
- **Fonts Directory**: `/opt/noto-emoji/fonts/`
- **Key Font**: `Noto-COLRv1.ttf` (1,086,145 bytes COLR table)
- **Access**: Read access to fonts and build scripts (write access only for building)
- **Source**: Google Noto Emoji project with Python build environment

### Noto CJK Fonts
- **Location**: `/opt/noto-cjk/`
- **Access**: Read access to CJK font files (write access only for building)

### Other Libraries in /opt
- `/opt/notofonts.github.io/` - Extended Noto font collection (write access only for building)
- `/opt/emojicompat/` - Emoji compatibility library sources (write access only for building)
- `/opt/freetype/` - Main FreeType/Cairo development location (write access only for building)

## Build Configuration

### Current Makefile Settings

```makefile
INCLUDES := -I./src -I./tests \
	-I/opt/freetype/include \
	-I/opt/freetype/cairo/src \
	$(shell pkg-config --cflags vulkan glfw3 harfbuzz fribidi)

LIBS := $(shell pkg-config --libs vulkan glfw3 harfbuzz fribidi) \
	-L/opt/freetype/objs/.libs -lfreetype \
	-L/opt/freetype/cairo/builddir/src -lcairo \
	-lm -lpthread \
	-Wl,-rpath,/opt/freetype/objs/.libs \
	-Wl,-rpath,/opt/freetype/cairo/builddir/src
```

### HarfBuzz 10.2.0
- **Installation**: System package (via pkg-config)
- **Purpose**: Complex script text shaping (Arabic, Hebrew, Indic scripts, etc.)
- **Access**: Read-only via system libraries
- **Integration**: Used in `src/nvg_freetype.c` for shaped text iteration

### FriBidi 1.0.16
- **Installation**: System package (via pkg-config)
- **Purpose**: Unicode Bidirectional Algorithm (BiDi) for RTL text
- **Access**: Read-only via system libraries
- **Integration**: Used in `src/nvg_freetype.c` for BiDi text reordering

### Key Points

1. **No `.local` dependencies** - All font library paths point directly to `/opt/` source/build directories
2. **System packages** - HarfBuzz and FriBidi use system installations via pkg-config
3. **Direct linking** - Font libraries (FreeType, Cairo) linked from build directories
4. **Runtime paths** - `rpath` ensures binaries find the correct library versions at runtime
5. **Source availability** - All library source code is accessible for debugging and modification

## BiDi Text Implementation

The bidirectional text rendering uses:
- FriBidi for Unicode BiDi algorithm (logical to visual order conversion)
- HarfBuzz for complex script shaping (glyph selection and positioning)
- Custom integration in `src/nvg_freetype.c`:
  - Function: `nvgft_shaped_text_iter_init()` (line 1015)
  - Converts UTF-8 ↔ UTF-32 for FriBidi processing
  - Applies BiDi reordering: `fribidi_log2vis()`
  - Shapes text with HarfBuzz: `hb_shape()`
  - Direction parameter: 0=auto, 1=LTR, 2=RTL
  - Language tags: "ar", "he", "en", etc.

### Fresh FT_Face for HarfBuzz
HarfBuzz requires a fresh (non-cached) FreeType face for proper operation:
- Each font maintains `hb_face` (fresh FT_Face) in addition to cached face
- Created on-demand in `nvgft__get_hb_font()` (line 992)
- Properly cleaned up in `nvgft_destroy()` (line 200)

## Color Emoji Implementation

The color emoji rendering uses:
- FreeType for font loading and glyph detection
- Cairo for COLR v1 vector emoji rendering
- Custom integration in `src/nvg_freetype.c`:
  - Function: `nvgft__render_color_emoji()` (line 368)
  - Creates fresh `FT_Face` for Cairo compatibility
  - Uses `CAIRO_COLOR_MODE_COLOR` for color glyph rendering

## Testing

### BiDi Text Test (Low-level)
- **Source**: `tests/test_bidi.c`
- **Binary**: `build/test_bidi`
- **Build**: `make build/test_bidi`
- **Run**: `./build/test_bidi`
- **Coverage**: 8 test cases
  - Arabic RTL: "مرحبا بالعالم" (Hello World)
  - Hebrew RTL: "שלום עולם" (Hello World)
  - Mixed LTR/RTL text
  - Auto-detection vs explicit direction
- **Result**: All tests passing (8/8)

### BiDi Visual Test (NanoVG Integration)
- **Source**: `tests/test_nvg_bidi.c`
- **Binary**: `build/test_nvg_bidi`
- **Build**: `make build/test_nvg_bidi`
- **Run**: `VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./build/test_nvg_bidi`
- **Output**: Creates `bidi_test.ppm` screenshot (convert to PNG: `convert bidi_test.ppm bidi_test.png`)
- **Visual Tests**:
  - Arabic RTL: "مرحبا بالعالم" (Hello World)
  - Hebrew RTL: "שלום עולם" (Hello World)
  - Mixed AR+EN: "Hello مرحبا World"
  - Mixed HE+EN: "Hello שלום World"
  - Arabic RTL: "السلام عليكم" (Peace be upon you)
  - English LTR: "Testing BiDi"
- **Verification**: All text properly shaped with HarfBuzz and reordered with FriBidi
- **Result**: Visual rendering confirms RTL text displays correctly (not backwards)

### Color Emoji Test
- **Source**: `tests/test_color_emoji.c`
- **Binary**: `build/test_color_emoji`
- **Build**: `make build/test_color_emoji`
- **Output**: Creates `test_color_emoji_output.png` in project root
- **Verification**: Renders U+1F600 (😀) grinning face emoji in full color

### Library Verification

Check linked libraries:
```bash
ldd build/test_color_emoji | grep -E "(freetype|cairo)"
```

Expected output:
```
libfreetype.so.6 => /opt/freetype/objs/.libs/libfreetype.so.6
libcairo.so.2 => /opt/freetype/cairo/builddir/src/libcairo.so.2
```

1 **Build** libraries after changes:
   - FreeType: `cd /opt/freetype && make`
   - Cairo: `cd /opt/freetype/cairo/builddir && ninja`
2 **No installation needed** - Project links directly to build directories

## Architecture

This setup provides:
- ✅ Full source code transparency
- ✅ Ability to modify and rebuild libraries
- ✅ No hidden system library dependencies
- ✅ Reproducible builds from source
- ✅ Direct debugging of library code
- ✅ Independent of system package updates

## Virtual Atlas System

### Overview
The NanoVG Vulkan backend includes a fully implemented virtual atlas system for handling large glyph sets (CJK fonts, emoji) and dynamic text rendering.

### Components
- **Multi-Atlas**: Up to 8 texture atlases with automatic growth (512→1024→2048→4096)
- **Guillotine Packing**: Efficient 2D bin packing algorithm with 78%+ space efficiency
- **Defragmentation**: Incremental defragmentation (2ms/frame budget) to reclaim fragmented space
- **Background Loading**: Thread-safe glyph rasterization without blocking render thread
- **LRU Cache**: 8192 glyph cache with automatic eviction
- **Async Uploads**: Optional transfer queue-based texture uploads (implemented, not enabled)

### Integration Test
```bash
make test-virtual-atlas
```

Expected output:
```
✓ Vulkan context created
✓ Virtual atlas created (4096x4096, 8192 cache entries)
✓ Multi-atlas allocation (100 glyphs, 2 atlases, 78.1% efficiency)
✓ Defragmentation system ready
✓ Background loading active
✓ All systems functional
```

### Architecture
- **Headers**: `src/nanovg_vk_virtual_atlas.h`, `src/nanovg_vk_multi_atlas.h`, `src/nanovg_vk_atlas_defrag.h`, `src/nanovg_vk_atlas_packing.h`, `src/nanovg_vk_async_upload.h`
- **Implementation**: `src/nanovg_vk_virtual_atlas.c`
- **Documentation**: `VIRTUAL_ATLAS_INTEGRATION.md`

### Status
- ✅ Multi-atlas allocation working
- ✅ Guillotine packing working
- ✅ Dynamic growth working (512→4096)
- ✅ Defragmentation working
- ✅ Background loading working
- ✅ **Font context connected** (callback-based, automatic)
- ✅ **Glyph rasterization callback** (FreeType FTC integration)
- ✅ **Async uploads enabled** (using graphics queue, triple buffering)

### Font Context Connection
The font context is automatically connected to the virtual atlas during NanoVG context creation via a callback mechanism:
- Added `renderFontSystemCreated` callback to `NVGparams`
- Callback invoked after font system initialization in `nvgCreateInternal()`
- Vulkan backend receives callback and connects font context to virtual atlas
- Glyph rasterization callback uses `nvgft_rasterize_glyph()` API
- Returns RGBA pixel data from FreeType FTC cache
- Verified working in all tests (BiDi, color emoji, etc.)

### Async Uploads
Async texture uploads enabled using triple buffering:
- 3 frames in flight with 1MB staging buffer per frame
- Currently using graphics queue (works but not optimal)
- Can be optimized with dedicated transfer queue in future
- Enabled automatically in `nvgvk_create()` (src/vulkan/nvg_vk_context.c:93)

See `VIRTUAL_ATLAS_INTEGRATION.md` for complete details.

## Notes

- System libraries (vulkan, glfw3, harfbuzz, fribidi) still use pkg-config
- All font rendering libraries (FreeType, Cairo) use `/opt/` sources
- Test outputs (PNG/PPM files) are git-ignored and can be regenerated
- Virtual atlas system fully implemented with all planned features
