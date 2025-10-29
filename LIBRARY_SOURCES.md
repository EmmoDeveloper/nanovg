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

### Key Points

1. **No `.local` dependencies** - All library paths point directly to `/opt/` source/build directories
2. **Direct linking** - Libraries are linked from their build directories, not system install locations
3. **Runtime paths** - `rpath` ensures binaries find the correct library versions at runtime
4. **Source availability** - All library source code is accessible for debugging and modification

## Color Emoji Implementation

The color emoji rendering uses:
- FreeType for font loading and glyph detection
- Cairo for COLR v1 vector emoji rendering
- Custom integration in `src/nvg_freetype.c`:
  - Function: `nvgft__render_color_emoji()` (line 368)
  - Creates fresh `FT_Face` for Cairo compatibility
  - Uses `CAIRO_COLOR_MODE_COLOR` for color glyph rendering

## Testing

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

## Notes

- System libraries (vulkan, glfw3, harfbuzz, fribidi) still use pkg-config
- All font rendering libraries (FreeType, Cairo) use `/opt/` sources
- Test outputs (PNG/PPM files) are git-ignored and can be regenerated
