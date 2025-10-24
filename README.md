# NanoVG Vulkan Backend

High-performance Vulkan rendering backend for [NanoVG](https://github.com/memononen/nanovg) with advanced text rendering and internationalization support.

## Features

### Core Rendering
- Vector graphics with antialiasing
- Stencil-based path filling
- Gradient and texture fills
- All NanoVG blend modes
- Scissor clipping

### Advanced Text Rendering
- **Virtual Atlas System**: Support for 65,000+ glyphs (CJK fonts)
- **Guillotine Packing**: 87.9% atlas efficiency
- **Dynamic Growth**: Progressive scaling (512→1024→2048→4096)
- **Multi-Atlas**: Up to 8 atlases with automatic overflow
- **LRU Eviction**: Intelligent cache management
- **HarfBuzz Integration**: Complex script shaping (14+ scripts)
- **BiDi Support**: Right-to-left text (Arabic, Hebrew)
- **Text Effects**: GPU-accelerated SDF effects
  - Multi-layer outlines (up to 4 layers)
  - Glow with customizable radius and color
  - Drop shadows with blur
  - Linear and radial gradients
  - Animations (shimmer, pulse, wave, color cycling)

### Performance
- GPU-accelerated rendering
- Text instancing and caching
- Atlas prewarming
- Batch rendering optimizations
- 62× memory reduction (256KB vs 16MB startup)

### Platform Support
- Vulkan 1.3+ with dynamic rendering
- Traditional render passes (Vulkan 1.0+)
- Multi-platform (Linux, Windows, macOS via MoltenVK)

## Quick Start

### Prerequisites
- Vulkan SDK 1.3+
- C11 compiler
- HarfBuzz (for international text)
- FriBidi (for BiDi support)

### Build
```bash
make
```

### Run Tests
```bash
make smoke-tests        # Basic functionality
make check-deps         # Verify dependencies
./build/test_init       # Complete test
```

### Usage Example
```c
#include "nanovg.h"
#include "nanovg_vk.h"

// Create context
NVGcontext* vg = nvgCreateVk(params);

// Render
nvgBeginFrame(vg, width, height, pixelRatio);
nvgBeginPath(vg);
nvgRect(vg, 100, 100, 200, 150);
nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
nvgFill(vg);
nvgEndFrame(vg);

// Cleanup
nvgDeleteVk(vg);
```

### International Text
```c
// Load CJK font
int font = nvgCreateFont(vg, "cjk", "NotoSansCJK.ttf");

// Render with automatic caching
nvgFontFace(vg, "cjk");
nvgFontSize(vg, 20.0f);
nvgText(vg, x, y, "你好世界", NULL);      // Chinese
nvgText(vg, x, y, "مرحبا بالعالم", NULL); // Arabic (RTL)
nvgText(vg, x, y, "हैलो वर्ल्ड", NULL);  // Devanagari
```

### Text Effects
```c
#include "nanovg_vk_text_effects.h"

// Create effect context
VKNVGtextEffect* effect = vknvg__createTextEffect();

// Add multi-layer outline
vknvg__addOutline(effect, 2.0f, nvgRGBA(0, 0, 0, 255));      // Inner black
vknvg__addOutline(effect, 4.0f, nvgRGBA(255, 255, 255, 255)); // Outer white

// Add glow effect
vknvg__setGlow(effect, 10.0f, nvgRGBA(255, 200, 0, 128), 0.8f);

// Add gradient fill
vknvg__setLinearGradient(effect,
    nvgRGBA(255, 0, 0, 255),   // Red start
    nvgRGBA(0, 0, 255, 255),   // Blue end
    0.0f);                      // Horizontal

// Add shimmer animation
vknvg__setAnimation(effect, VKNVG_ANIM_SHIMMER, 1.5f, 0.7f);

// Cleanup
vknvg__destroyTextEffect(effect);
```

## Recent Updates

### Complete JNI API Implementation (October 2025)

**Status**: ✅ Complete - All public NanoVG APIs now exposed via JNI

The Java bindings have been enhanced with complete API coverage:

#### What Was Added
- **30 Constants**: Blend factors, composite operations, image flags, solidity
- **24 Functions**: 16 native JNI implementations + 8 pure Java utilities
  - Color utilities (9): RGB/RGBA/HSL creation, interpolation, transparency
  - Transform utilities (12): Matrix operations, angle conversion
  - Font functions (2): Index-based loading, fallback management
  - Text layout (2): Glyph positioning, line breaking

#### Statistics
- **Before**: 80 native methods, ~76% API coverage
- **After**: 96 native methods, 100% API coverage
- **Build Status**: All tests passing (27/27 test groups)
- **Maven Build**: SUCCESS

#### Implementation Highlights
- Pure Java implementations for simple operations (no JNI overhead)
- Native implementations for complex operations requiring C integration
- Complete color space conversions (RGB, HSL)
- Full 2D transform matrix operations
- Advanced text layout with glyph and line metrics

See detailed API reference below for complete usage examples.

---

## Java JNI Bindings

Complete JNI bindings for Java applications (requires Java 25+).

### Build Native Library
```bash
./build-jni.sh
# Produces: build/libnanovg-jni.so (1.3MB)
```

### Maven Integration
```xml
<build>
  <plugins>
    <plugin>
      <groupId>org.codehaus.mojo</groupId>
      <artifactId>exec-maven-plugin</artifactId>
      <version>3.1.0</version>
      <executions>
        <execution>
          <id>build-jni</id>
          <phase>compile</phase>
          <goals>
            <goal>exec</goal>
          </goals>
          <configuration>
            <executable>${project.basedir}/build-jni.sh</executable>
          </configuration>
        </execution>
      </executions>
    </plugin>
  </plugins>
</build>
```

### Usage Example
```java
import org.emmo.ai.nanovg.NanoVG;

// Load native library
System.load("/path/to/libnanovg-jni.so");

// Create context with MSDF text support
long ctx = NanoVG.nvgCreateVk(instance, physicalDevice, device, queue,
    queueFamilyIndex, renderPass, commandPool, descriptorPool, 3,
    NanoVG.NVG_ANTIALIAS | NanoVG.NVG_MSDF_TEXT);

// Begin frame
NanoVG.nvgBeginFrame(ctx, 800, 600, 1.0f);

// Draw circle with transform
NanoVG.nvgSave(ctx);
NanoVG.nvgTranslate(ctx, 400, 300);
NanoVG.nvgRotate(ctx, (float)(Math.PI / 4));
NanoVG.nvgBeginPath(ctx);
NanoVG.nvgCircle(ctx, 0, 0, 50);
NanoVG.nvgFillColor(ctx, 255, 192, 0, 255);
NanoVG.nvgFill(ctx);
NanoVG.nvgRestore(ctx);

// Draw MSDF text
int font = NanoVG.nvgCreateFont(ctx, "sans", "/path/to/font.ttf");
NanoVG.nvgSetFontMSDF(ctx, font, NanoVG.MSDF_MODE_MSDF);
NanoVG.nvgFontFace(ctx, "sans");
NanoVG.nvgFontSize(ctx, 48.0f);
NanoVG.nvgFillColor(ctx, 255, 255, 255, 255);
NanoVG.nvgText(ctx, 100, 100, "High Quality Text!");

// End frame
NanoVG.nvgEndFrame(ctx);

// Cleanup
NanoVG.nvgDeleteVk(ctx);
```

### Color Emoji Support
```java
import org.emmo.ai.nanovg.NanoVG;

// Load native library
System.load("/path/to/libnanovg-jni.so");

// Create context with color emoji support (from file)
long ctx = NanoVG.nvgCreateVk(instance, physicalDevice, device, queue,
    queueFamilyIndex, renderPass, commandPool, descriptorPool, 3,
    NanoVG.NVG_ANTIALIAS | NanoVG.NVG_COLOR_EMOJI,
    "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf");

// Or load emoji font from memory
byte[] emojiFont = Files.readAllBytes(Paths.get("/path/to/emoji.ttf"));
long ctx2 = NanoVG.nvgCreateVk(instance, physicalDevice, device, queue,
    queueFamilyIndex, renderPass, commandPool, descriptorPool, 3,
    NanoVG.NVG_ANTIALIAS | NanoVG.NVG_COLOR_EMOJI,
    emojiFont);

// Render text with emoji - automatic color emoji rendering
NanoVG.nvgBeginFrame(ctx, 800, 600, 1.0f);
NanoVG.nvgFontSize(ctx, 32.0f);
NanoVG.nvgFillColor(ctx, 255, 255, 255, 255);
NanoVG.nvgText(ctx, 100, 100, "Hello 👋 World 🌍!");  // Emoji automatically rendered in color
NanoVG.nvgEndFrame(ctx);

// Cleanup
NanoVG.nvgDeleteVk(ctx);
```

### Advanced Features

#### Color Operations
```java
// Create colors in various formats
float[] red = NanoVG.nvgRGB(255, 0, 0);           // Byte RGB
float[] blue = NanoVG.nvgRGBf(0.0f, 0.0f, 1.0f);  // Float RGB
float[] orange = NanoVG.nvgRGBA(255, 128, 0, 200); // With alpha
float[] cyan = NanoVG.nvgHSL(0.5f, 1.0f, 0.5f);   // HSL color space

// Color interpolation
float[] purple = NanoVG.nvgLerpRGBA(red, blue, 0.5f);

// Adjust transparency
float[] semiTransparent = NanoVG.nvgTransRGBA(red, 128);
```

#### Gradients and Patterns
```java
// Linear gradient
long linearPaint = NanoVG.nvgLinearGradient(ctx,
    0, 0, 200, 200,           // Start x,y to end x,y
    255, 0, 0, 255,           // Start color (red)
    0, 0, 255, 255);          // End color (blue)
NanoVG.nvgFillPaint(ctx, linearPaint);

// Radial gradient
long radialPaint = NanoVG.nvgRadialGradient(ctx,
    100, 100,                 // Center x,y
    10, 100,                  // Inner and outer radius
    255, 255, 0, 255,         // Inner color (yellow)
    255, 0, 0, 255);          // Outer color (red)
NanoVG.nvgFillPaint(ctx, radialPaint);

// Box gradient (rounded rectangle gradient)
long boxPaint = NanoVG.nvgBoxGradient(ctx,
    50, 50, 200, 100,         // x, y, width, height
    10, 20,                   // Corner radius, feather
    200, 200, 255, 255,       // Inner color
    100, 100, 200, 255);      // Outer color
NanoVG.nvgFillPaint(ctx, boxPaint);

// Image pattern
int image = NanoVG.nvgCreateImage(ctx, "texture.png", 0);
long imagePaint = NanoVG.nvgImagePattern(ctx,
    0, 0, 200, 200,           // Origin and size
    0.0f,                     // Rotation angle
    image,                    // Image handle
    1.0f);                    // Alpha
NanoVG.nvgFillPaint(ctx, imagePaint);
```

#### Transform Operations
```java
// Basic transforms
NanoVG.nvgTranslate(ctx, 100, 100);
NanoVG.nvgRotate(ctx, NanoVG.nvgDegToRad(45));
NanoVG.nvgScale(ctx, 2.0f, 2.0f);

// Matrix operations
float[] transform = new float[6];
NanoVG.nvgTransformIdentity(transform);
NanoVG.nvgTransformRotate(transform, NanoVG.nvgDegToRad(30));
NanoVG.nvgTransformTranslate(transform, 50, 50);

// Apply custom transform
NanoVG.nvgTransform(ctx, transform[0], transform[1], transform[2],
                          transform[3], transform[4], transform[5]);

// Get current transform
float[] current = NanoVG.nvgCurrentTransform(ctx);

// Transform a point
float[] point = new float[2];
NanoVG.nvgTransformPoint(point, transform, 100, 100);
```

#### Image Loading
```java
// Load from file
int img1 = NanoVG.nvgCreateImage(ctx, "photo.jpg",
    NanoVG.NVG_IMAGE_GENERATE_MIPMAPS | NanoVG.NVG_IMAGE_REPEATX);

// Load from memory
byte[] imageData = Files.readAllBytes(Paths.get("photo.png"));
int img2 = NanoVG.nvgCreateImageMem(ctx,
    NanoVG.NVG_IMAGE_GENERATE_MIPMAPS, imageData);

// Create from RGBA pixel data
int width = 256, height = 256;
byte[] pixels = new byte[width * height * 4];
// ... fill pixels ...
int img3 = NanoVG.nvgCreateImageRGBA(ctx, width, height, 0, pixels);

// Update image
NanoVG.nvgUpdateImage(ctx, img1, newPixelData);

// Query size
int[] size = NanoVG.nvgImageSize(ctx, img1);
System.out.println("Size: " + size[0] + "x" + size[1]);

// Cleanup
NanoVG.nvgDeleteImage(ctx, img1);
```

#### Advanced Font Loading
```java
// Load font from specific index in TTC/OTC file
int font1 = NanoVG.nvgCreateFontAtIndex(ctx, "CJK", "fonts.ttc", 2);

// Load from memory
byte[] fontData = Files.readAllBytes(Paths.get("font.ttf"));
int font2 = NanoVG.nvgCreateFontMem(ctx, "custom", fontData, false);

// Font fallback chains (for multi-language support)
int latinFont = NanoVG.nvgCreateFont(ctx, "latin", "Roboto.ttf");
int arabicFont = NanoVG.nvgCreateFont(ctx, "arabic", "NotoSansArabic.ttf");
int emojiFont = NanoVG.nvgCreateFont(ctx, "emoji", "NotoColorEmoji.ttf");

// Chain fallbacks
NanoVG.nvgAddFallbackFont(ctx, "latin", "arabic");
NanoVG.nvgAddFallbackFont(ctx, "latin", "emoji");

// Use font by ID
NanoVG.nvgFontFaceId(ctx, latinFont);

// Reset fallback chain
NanoVG.nvgResetFallbackFontsId(ctx, latinFont);
```

#### Advanced Text Layout
```java
// Get glyph positions for custom layout
float[] positions = new float[100 * 4]; // Up to 100 glyphs (x, minx, maxx, reserved)
int glyphCount = NanoVG.nvgTextGlyphPositions(ctx, 0, 0, "Hello World", positions, 100);

for (int i = 0; i < glyphCount; i++) {
    float x = positions[i * 4 + 0];
    float minx = positions[i * 4 + 1];
    float maxx = positions[i * 4 + 2];
    System.out.println("Glyph " + i + ": x=" + x + " width=" + (maxx - minx));
}

// Break text into lines
float[] rows = new float[10 * 4]; // Up to 10 rows (start, end, width, minx)
int rowCount = NanoVG.nvgTextBreakLines(ctx,
    "This is a long text that needs to be wrapped into multiple lines",
    200.0f,  // Max width
    rows, 10);

for (int i = 0; i < rowCount; i++) {
    int start = (int)rows[i * 4 + 0];
    int end = (int)rows[i * 4 + 1];
    float width = rows[i * 4 + 2];
    System.out.println("Row " + i + ": chars " + start + "-" + end + " width=" + width);
}
```

#### Custom Blend Modes
```java
// Use predefined composite operation
NanoVG.nvgGlobalCompositeOperation(ctx, NanoVG.NVG_SOURCE_OVER);
NanoVG.nvgGlobalCompositeOperation(ctx, NanoVG.NVG_XOR);

// Custom blend factors
NanoVG.nvgGlobalCompositeBlendFunc(ctx,
    NanoVG.NVG_SRC_ALPHA,
    NanoVG.NVG_ONE_MINUS_SRC_ALPHA);

// Separate RGB and alpha blending
NanoVG.nvgGlobalCompositeBlendFuncSeparate(ctx,
    NanoVG.NVG_SRC_ALPHA, NanoVG.NVG_ONE_MINUS_SRC_ALPHA,  // RGB
    NanoVG.NVG_ONE, NanoVG.NVG_ZERO);                        // Alpha
```

### API Coverage (96 Native Methods + 50+ Constants)

**Coverage**: 100% of public NanoVG API

#### Constants (50+ values)
- **Creation Flags**: `NVG_ANTIALIAS`, `NVG_STENCIL_STROKES`, `NVG_DEBUG`, `NVG_MSDF_TEXT`, `NVG_COLOR_EMOJI`, etc.
- **Blend Factors** (11): `NVG_ZERO`, `NVG_ONE`, `NVG_SRC_COLOR`, `NVG_SRC_ALPHA`, `NVG_DST_ALPHA`, etc.
- **Composite Operations** (11): `NVG_SOURCE_OVER`, `NVG_SOURCE_IN`, `NVG_DESTINATION_OVER`, `NVG_XOR`, `NVG_COPY`, etc.
- **Image Flags** (6): `NVG_IMAGE_GENERATE_MIPMAPS`, `NVG_IMAGE_REPEATX`, `NVG_IMAGE_REPEATY`, `NVG_IMAGE_FLIPY`, etc.
- **Alignment**: `NVG_ALIGN_LEFT`, `NVG_ALIGN_CENTER`, `NVG_ALIGN_RIGHT`, `NVG_ALIGN_TOP`, `NVG_ALIGN_BOTTOM`, etc.
- **Winding**: `NVG_CCW`, `NVG_CW`, `NVG_SOLID`, `NVG_HOLE`
- **Line Caps/Joins**: `NVG_BUTT`, `NVG_ROUND`, `NVG_SQUARE`, `NVG_MITER`, `NVG_BEVEL`

#### Context Management (6 methods)
- `nvgCreateVk` (3 overloads: basic, emoji from file, emoji from memory)
- `nvgDeleteVk`, `nvgBeginFrame`, `nvgEndFrame`, `nvgCancelFrame`

#### State Management (4 methods)
- `nvgSave`, `nvgRestore`, `nvgReset`, `nvgShapeAntiAlias`

#### Paths & Shapes (13 methods)
- **Paths**: `nvgBeginPath`, `nvgMoveTo`, `nvgLineTo`, `nvgBezierTo`, `nvgQuadTo`, `nvgArcTo`, `nvgClosePath`, `nvgPathWinding`
- **Shapes**: `nvgRect`, `nvgRoundedRect`, `nvgRoundedRectVarying`, `nvgEllipse`, `nvgCircle`, `nvgArc`

#### Transforms (12 methods)
- **Basic**: `nvgTranslate`, `nvgRotate`, `nvgScale`, `nvgSkewX`, `nvgSkewY`, `nvgResetTransform`, `nvgTransform`
- **Utilities**: `nvgCurrentTransform`
- **Matrix Operations**: `nvgTransformIdentity`, `nvgTransformTranslate`, `nvgTransformScale`, `nvgTransformRotate`, `nvgTransformSkewX`, `nvgTransformSkewY`, `nvgTransformMultiply`, `nvgTransformPremultiply`, `nvgTransformInverse`, `nvgTransformPoint`
- **Angle Conversion**: `nvgDegToRad`, `nvgRadToDeg`

#### Color Utilities (9 methods)
- **RGB/RGBA**: `nvgRGB`, `nvgRGBf`, `nvgRGBA`, `nvgRGBAf`
- **HSL/HSLA**: `nvgHSL`, `nvgHSLA`
- **Manipulation**: `nvgLerpRGBA`, `nvgTransRGBA`, `nvgTransRGBAf`

#### Paint & Gradients (10 methods)
- **Colors**: `nvgFillColor`, `nvgStrokeColor`
- **Gradients**: `nvgLinearGradient`, `nvgBoxGradient`, `nvgRadialGradient`
- **Patterns**: `nvgImagePattern`
- **Apply**: `nvgFillPaint`, `nvgStrokePaint`
- **Alpha**: `nvgGlobalAlpha`
- **Compositing**: `nvgGlobalCompositeOperation`, `nvgGlobalCompositeBlendFunc`, `nvgGlobalCompositeBlendFuncSeparate`

#### Rendering (5 methods)
- `nvgFill`, `nvgStroke`
- **Stroke Style**: `nvgStrokeWidth`, `nvgLineCap`, `nvgLineJoin`, `nvgMiterLimit`

#### Scissoring (3 methods)
- `nvgScissor`, `nvgIntersectScissor`, `nvgResetScissor`

#### Images (6 methods)
- **Creation**: `nvgCreateImage`, `nvgCreateImageMem`, `nvgCreateImageRGBA`
- **Management**: `nvgUpdateImage`, `nvgImageSize`, `nvgDeleteImage`

#### Fonts (10 methods)
- **Loading**: `nvgCreateFont`, `nvgCreateFontAtIndex`, `nvgCreateFontMem`, `nvgCreateFontMemAtIndex`
- **Fallback**: `nvgAddFallbackFont`, `nvgAddFallbackFontId`, `nvgResetFallbackFonts`, `nvgResetFallbackFontsId`
- **Selection**: `nvgFontFace`, `nvgFontFaceId`, `nvgFindFont`

#### Text Rendering (11 methods)
- **Styling**: `nvgFontSize`, `nvgFontBlur`, `nvgTextAlign`, `nvgTextLetterSpacing`, `nvgTextLineHeight`
- **Rendering**: `nvgText`, `nvgTextBox`
- **Measurement**: `nvgTextBounds`, `nvgTextBoxBounds`, `nvgTextMetrics`
- **Advanced Layout**: `nvgTextGlyphPositions`, `nvgTextBreakLines`
- **MSDF**: `nvgSetFontMSDF`

## Project Structure

```
nanovg/
├── src/                    # Core NanoVG + Vulkan backend
│   ├── nanovg.c           # NanoVG core
│   ├── nanovg_vk.h        # Vulkan backend API
│   ├── nanovg_vk_virtual_atlas.c  # Virtual atlas system
│   ├── nanovg_vk_harfbuzz.c       # HarfBuzz integration
│   ├── nanovg_vk_bidi.c           # BiDi support
│   ├── nanovg_vk_text_effects.c   # SDF text effects
│   ├── nanovg_vk_emoji_tables.c   # Emoji font parsing
│   ├── nanovg_vk_color_atlas.c    # RGBA color atlas
│   ├── nanovg_vk_bitmap_emoji.c   # Bitmap emoji rendering
│   ├── nanovg_vk_colr_render.c    # COLR vector emoji
│   ├── nanovg_vk_emoji.c          # Emoji detection
│   └── nanovg_vk_text_emoji.c     # Text-emoji integration
│   └── main/              # Java JNI bindings
│       ├── java/org/emmo/ai/nanovg/
│       │   └── NanoVG.java         # Java API (96 native methods + 8 utilities, 780+ lines)
│       └── native/
│           ├── nanovg_jni.c        # JNI implementation (1,155 lines)
│           └── org_emmo_ai_nanovg_NanoVG.h  # JNI header (893 lines)
├── tests/                 # Test suite (81+ tests)
│   ├── test_*.c          # C test suite
│   └── src/test/java/org/emmo/ai/nanovg/
│       ├── CompleteAPITest.java    # API verification
│       ├── FinalAPITest.java       # Complete API test
│       ├── EmojiJNITest.java       # Emoji support test
│       └── NanoVGTest.java         # Basic JNI test
├── build-jni.sh          # JNI build script (builds libnanovg-jni.so)
├── pom.xml               # Maven configuration
├── Makefile              # Main build system
├── shaders/              # GLSL shaders
│   ├── sdf_effects.frag  # Extended SDF shader
│   ├── text_dual.vert    # Dual-mode text vertex shader
│   └── text_dual.frag    # Dual-mode text fragment shader (SDF + color)
└── docs/                 # Documentation
    ├── ARCHITECTURE.md   # Technical design
    └── CHANGELOG.md      # Version history
```

## Performance Characteristics

### Virtual Atlas
- **Capacity**: 4,096 glyphs resident, 65,000+ via eviction
- **Memory**: 256KB startup (grows to 16MB max)
- **Cache**: O(1) lookups, 87.9% packing efficiency
- **Scalability**: Unlimited glyphs via LRU eviction

### Text Rendering
- **Cache hit**: ~100-200ns (hash lookup)
- **Cache miss**: ~1-5ms (rasterization + upload)
- **GPU upload**: ~50-100µs per glyph (batched)

### Supported Scripts
Arabic, Devanagari, Bengali, Thai, Lao, Tibetan, Myanmar, Khmer, Hebrew, Latin, Greek, Cyrillic, CJK (Chinese, Japanese, Korean)

## Testing

```bash
# Run all available tests
make test

# Specific test suites
make smoke-tests          # Basic functionality
make unit-tests          # All unit tests (includes Phase 3-6)
make integration-tests   # Integration tests
make fun-tests           # Bad Apple!! demo tests
```

### Test Coverage
- **Phase 0-2**: Core Vulkan backend (36 tests)
- **Phase 3**: Atlas management (20 tests)
- **Phase 4**: International text (22 tests)
- **Phase 5**: Text effects (12 tests)
- **Phase 6**: Color emoji (60 tests)

## Documentation

- [API_REFERENCE.md](docs/API_REFERENCE.md) - Complete Java JNI API reference
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Technical design and implementation
- [CHANGELOG.md](docs/CHANGELOG.md) - Version history and completed phases
- [Archive](docs/archive/) - Historical phase documentation

## Attribution

Based on NanoVG by Mikko Mononen (memon@inside.org)
- Original: https://github.com/memononen/nanovg
- License: zlib (see LICENSE)

Vulkan backend created October 2025.

## Phase 6: Color Emoji Support (October 2025)

**Status**: ✅ Complete (60/60 tests passing)

Full color emoji rendering with multiple format support:

### Backend Components
- **Emoji Table Parsing** (`nanovg_vk_emoji_tables.c/h`)
  - SBIX (Apple Color Emoji) - bitmap format parsing
  - CBDT/CBLC (Google Noto Color Emoji) - bitmap table parsing
  - COLR/CPAL (Microsoft) - vector color glyph tables
  - Font table detection and extraction

- **RGBA Color Atlas** (`nanovg_vk_color_atlas.c/h`)
  - Separate color texture atlas (distinct from SDF atlas)
  - LRU cache management for color glyphs
  - Upload queue for asynchronous GPU updates
  - Multi-atlas support for large emoji sets

- **Bitmap Emoji Rendering** (`nanovg_vk_bitmap_emoji.c/h`)
  - PNG extraction from SBIX/CBDT tables
  - BGRA to RGBA conversion
  - Premultiplied alpha handling
  - Bitmap scaling and size selection

- **COLR Vector Rendering** (`nanovg_vk_colr_render.c/h`)
  - Layered vector glyph rendering
  - CPAL palette application
  - Layer compositing with alpha blending
  - Foreground color substitution

- **Emoji Detection** (`nanovg_vk_emoji.c/h`)
  - Unicode emoji codepoint detection
  - Emoji modifier support (skin tones, gender, hair)
  - ZWJ (Zero Width Joiner) sequence detection
  - Format capability checking

- **Text-Emoji Integration** (`nanovg_vk_text_emoji.c/h`)
  - Seamless text + emoji rendering
  - Automatic mode switching (SDF vs color)
  - Render mode determination per glyph
  - Statistics and metrics tracking

### Dual-Mode Shader Pipeline
- **Shaders** (`shaders/text_dual.vert`, `shaders/text_dual.frag`)
  - Single pipeline for both SDF and color emoji
  - Dual texture sampling (SDF atlas + color atlas)
  - Per-glyph mode selection via vertex attributes
  - Proper alpha blending for color emoji

### Integration
- Full integration in `nanovg_vk_render.h`
- Automatic emoji format detection
- Fallback to SDF rendering when emoji unavailable
- Zero overhead when emoji not used

### Test Results
```
✓ 60/60 Phase 6 tests passing (100%)
  ✓ test_emoji_tables (10/10) - Table parsing
  ✓ test_color_atlas (10/10) - RGBA atlas system
  ✓ test_bitmap_emoji (10/10) - Bitmap rendering
  ✓ test_colr_render (10/10) - Vector rendering
  ✓ test_emoji_integration (10/10) - Emoji detection
  ✓ test_text_emoji_integration (10/10) - Text integration
```

### Files Created
- 6 implementation modules (12 files: .c + .h)
- 2 dual-mode shaders (compiled to SPIR-V)
- 6 comprehensive test suites
- Complete documentation and integration

## License

zlib license (same as NanoVG)
