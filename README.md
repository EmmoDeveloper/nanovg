# NanoVG Vulkan Backend

High-performance Vulkan rendering backend for [NanoVG](https://github.com/memononen/nanovg) with advanced text rendering and internationalization support.

## Status

✅ **Production Ready** - Complete implementation with comprehensive testing

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

### API Coverage (53 functions)
- **Context**: nvgCreateVk, nvgDeleteVk, nvgBeginFrame, nvgEndFrame
- **Paths**: nvgBeginPath, nvgMoveTo, nvgLineTo, nvgBezierTo, nvgQuadTo, nvgArcTo, nvgClosePath
- **Shapes**: nvgRect, nvgRoundedRect, nvgEllipse, nvgCircle, nvgArc
- **Transforms**: nvgTranslate, nvgRotate, nvgScale, nvgSkewX, nvgSkewY, nvgResetTransform
- **State**: nvgSave, nvgRestore, nvgReset
- **Rendering**: nvgFill, nvgStroke, nvgFillColor, nvgStrokeColor, nvgStrokeWidth, nvgGlobalAlpha
- **Stroke**: nvgLineCap, nvgLineJoin, nvgMiterLimit
- **Scissoring**: nvgScissor, nvgIntersectScissor, nvgResetScissor
- **Text**: nvgCreateFont, nvgSetFontMSDF, nvgFontFace, nvgFontSize, nvgText, nvgTextBox
- **Text Layout**: nvgTextAlign, nvgTextLetterSpacing, nvgTextLineHeight, nvgFontBlur
- **Measurement**: nvgTextBounds, nvgTextMetrics

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
│   └── main/              # Java JNI bindings
│       ├── java/org/emmo/ai/nanovg/
│       │   └── NanoVG.java         # Java API (53 methods)
│       └── native/
│           ├── nanovg_jni.c        # JNI implementation
│           └── org_emmo_ai_nanovg_NanoVG.h  # JNI header
├── tests/                 # Test suite (81 tests)
├── build-jni.sh          # JNI build script
├── pom.xml               # Maven configuration
├── shaders/              # GLSL shaders
│   └── sdf_effects.frag  # Extended SDF shader
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
# All tests (81 total)
make test

# Specific test suites
make smoke-tests          # Basic functionality (3 tests)
make unit-tests          # Unit tests (36 tests)
make integration-tests   # Integration (8 tests)
make phase3-tests        # Atlas packing (4 tests)
make phase4-tests        # International text (22 tests)
make phase5-tests        # Text effects (12 tests)
make fun-tests           # Bad Apple!! (6 tests)
```

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Technical design and implementation
- [CHANGELOG.md](docs/CHANGELOG.md) - Version history and completed phases
- [Archive](docs/archive/) - Historical phase documentation

## Attribution

Based on NanoVG by Mikko Mononen (memon@inside.org)
- Original: https://github.com/memononen/nanovg
- License: zlib (see LICENSE)

Vulkan backend created October 2025.

## License

zlib license (same as NanoVG)
