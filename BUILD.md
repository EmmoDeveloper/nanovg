# Building NanoVG with CMake

This project uses CMake for building. CMake automatically finds dependencies and generates clean, maintainable build files.

## Quick Start

```bash
# Configure
cmake -S . -B build

# Build
cmake --build build --parallel 4

# Or use make directly
cd build && make -j4
```

## Building Individual Tests

```bash
# Build a specific test
cmake --build build --target test_kerning_disabled

# Or with make
cd build && make test_kerning_disabled
```

## Running Tests

```bash
# Run a specific test
make -C build run_test_kerning_disabled

# Or run directly from project root
./build/test_kerning_disabled
```

## Screenshots

All test screenshots are saved to `screendumps/` in the project root.

```bash
ls screendumps/*.ppm
ls screendumps/*.png
```

## Dependencies

CMake automatically finds these dependencies using `find_package` and `pkg-config`:

- **Vulkan** - Graphics API
- **FreeType** - Font rendering
- **HarfBuzz** - Text shaping
- **Cairo** - 2D graphics
- **FriBidi** - BiDi text support
- **GLFW** - Window/input handling

## Build Configuration

```bash
# Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Release build (default)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

## Clean Build

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

## Comparison with Old Makefile

### Old Way (Makefile)
```bash
gcc -std=c11 -Wall -Wextra -O2 -g -I./src -I./src/font -I./tests -I/opt/freetype/include -I/opt/freetype/cairo/src -I/work/java/ai-ide-jvm/harfbuzz/src -I/opt/fribidi/lib -I/opt/fribidi/build/lib -I/opt/fribidi/build/gen.tab -I/opt/glfw/include -I/opt/lunarg-vulkan-sdk/x86_64/include -c tests/test_kerning_disabled.c -o build/test_kerning_disabled.o && gcc build/test_kerning_disabled.o build/nanovg.o build/nvg_vk.o ... [200+ characters]
```

### New Way (CMake)
```bash
cmake --build build --target test_kerning_disabled
```

CMake handles all include paths, library paths, and dependencies automatically.
