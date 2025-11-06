# NanoVG Vulkan Dependencies

This document lists all dependencies, their source locations, and build instructions.

## Dependency Overview

| Library | Location | Build System | Status | Purpose |
|---------|----------|--------------|--------|---------|
| FreeType | `/opt/freetype` | GNU Make | ✅ Built | Font rendering |
| Cairo | `/opt/freetype/cairo` | Meson | ✅ Built | Graphics rendering (FreeType integration) |
| HarfBuzz | `/work/java/ai-ide-jvm/harfbuzz` | Meson | ✅ Built | Text shaping |
| FriBidi | `/opt/fribidi` | Meson/Autotools | ❌ Not built | Bidirectional text |
| GLFW | `/opt/glfw` | CMake | ❌ Not built | Window/input management |
| Vulkan SDK | `/opt/lunarg-vulkan-sdk` | Tarball | ✅ Installed | Graphics API |

---

## 1. FreeType

**Location:** `/opt/freetype`

**Build System:** GNU Make

**Clean:**
```bash
cd /opt/freetype
make clean
```

**Build Instructions:**
```bash
cd /opt/freetype
git pull  # Update to latest
./configure
make
```

**Build Output:**
- Static library: `objs/.libs/libfreetype.a`
- Shared library: `objs/.libs/libfreetype.so`

**Headers:** `include/freetype/*.h`

**Current Status:** ✅ Built (version in git)

---

## 2. Cairo

**Location:** `/opt/freetype/cairo`

**Build System:** Meson

**Clean:**
```bash
cd /opt/freetype/cairo
rm -rf builddir
```

**Build Instructions:**
```bash
cd /opt/freetype/cairo
git pull  # Update to latest
meson setup builddir
meson compile -C builddir
```

**Build Output:**
- Shared library: `builddir/src/libcairo.so`

**Headers:** `src/cairo.h` and related headers

**Current Status:** ✅ Built (version in git)

---

## 3. HarfBuzz

**Location:** `/work/java/ai-ide-jvm/harfbuzz`

**Build System:** Meson

**Clean:**
```bash
cd /work/java/ai-ide-jvm/harfbuzz
rm -rf build
```

**Build Instructions:**
```bash
cd /work/java/ai-ide-jvm/harfbuzz
git pull  # Already done
meson setup build
meson compile -C build
```

**Build Output:**
- Static library: `build/libharfbuzz.a`
- Static library: `build/libharfbuzz-subset.a`

**Headers:** `src/*.h`

**Current Status:** ✅ Built (static libraries available)

**Note:** Currently linked via system pkg-config. Need to update Makefile to use local build.

---

## 4. FriBidi

**Location:** `/opt/fribidi`

**Build System:** Meson (preferred) or Autotools

**Clean:**
```bash
cd /opt/fribidi
rm -rf build
```

**Build Instructions:**
```bash
cd /opt/fribidi
git pull  # Freshly cloned
meson setup build
meson compile -C build
```

**Build Output:**
- Static library: `build/lib/libfribidi.a`
- Shared library: `build/lib/libfribidi.so`

**Headers:** `lib/fribidi.h` and related

**Current Status:** ❌ Not built (freshly cloned)

---

## 5. GLFW

**Location:** `/opt/glfw`

**Build System:** CMake

**Clean:**
```bash
cd /opt/glfw
rm -rf build
```

**Build Instructions:**
```bash
cd /opt/glfw
git pull  # Freshly cloned
mkdir -p build
cd build
cmake ..
make
```

**Expected Build Output:**
- Static library: `build/src/libglfw3.a`
- Shared library: `build/src/libglfw.so`

**Headers:** `include/GLFW/*.h`

**Current Status:** ❌ Not built (freshly cloned)

---

## 6. Vulkan SDK

**Location:** `/opt/lunarg-vulkan-sdk`

**Build System:** Tarball distribution (not git)

**Installation:** Already extracted from tarball

**Headers:** `/opt/lunarg-vulkan-sdk/include/vulkan/*.h`

**Libraries:** System installation via package manager

**Current Status:** ✅ Installed (headers available, using system libraries)

---

## Clean All Dependencies Script

```bash
#!/bin/bash
set -e

echo "Cleaning all NanoVG dependencies..."

# 1. FreeType
echo "=== Cleaning FreeType ==="
cd /opt/freetype
make clean 2>/dev/null || true

# 2. Cairo
echo "=== Cleaning Cairo ==="
cd /opt/freetype/cairo
rm -rf builddir

# 3. HarfBuzz
echo "=== Cleaning HarfBuzz ==="
cd /work/java/ai-ide-jvm/harfbuzz
rm -rf build

# 4. FriBidi
echo "=== Cleaning FriBidi ==="
cd /opt/fribidi
rm -rf build

# 5. GLFW
echo "=== Cleaning GLFW ==="
cd /opt/glfw
rm -rf build

echo "✅ All dependencies cleaned"
```

## Build All Dependencies Script

```bash
#!/bin/bash
set -e

echo "Building all NanoVG dependencies..."

# 1. FreeType
echo "=== Building FreeType ==="
cd /opt/freetype
git pull
./configure
make -j$(nproc)

# 2. Cairo
echo "=== Building Cairo ==="
cd /opt/freetype/cairo
git pull
meson setup builddir 2>/dev/null || true  # Skip if exists
meson compile -C builddir

# 3. HarfBuzz
echo "=== Building HarfBuzz ==="
cd /work/java/ai-ide-jvm/harfbuzz
git pull
meson setup build 2>/dev/null || true  # Skip if exists
meson compile -C build

# 4. FriBidi
echo "=== Building FriBidi ==="
cd /opt/fribidi
git pull
meson setup build 2>/dev/null || true  # Skip if exists
meson compile -C build

# 5. GLFW
echo "=== Building GLFW ==="
cd /opt/glfw
git pull
mkdir -p build
cd build
cmake .. 2>/dev/null || true  # Skip if already configured
make -j$(nproc)

echo "✅ All dependencies built successfully"
```

---

## Updating NanoVG Makefile

After building all dependencies, update the Makefile to use local builds:

**Current Makefile (lines 5-13):**
```makefile
INCLUDES := -I./src -I./tests \
	-I/opt/freetype/include \
	-I/opt/freetype/cairo/src \
	$(shell pkg-config --cflags vulkan glfw3 harfbuzz fribidi)
LIBS := $(shell pkg-config --libs vulkan glfw3 harfbuzz fribidi) \
	-L/opt/freetype/objs/.libs -lfreetype \
	-L/opt/freetype/cairo/builddir/src -lcairo \
	-lm -lpthread \
	-Wl,-rpath,/opt/freetype/objs/.libs -Wl,-rpath,/opt/freetype/cairo/builddir/src
```

**Updated Makefile (use local builds):**
```makefile
INCLUDES := -I./src -I./tests \
	-I/opt/freetype/include \
	-I/opt/freetype/cairo/src \
	-I/work/java/ai-ide-jvm/harfbuzz/src \
	-I/opt/fribidi/lib \
	-I/opt/fribidi/build/lib \
	-I/opt/fribidi/build/gen.tab \
	-I/opt/glfw/include \
	$(shell pkg-config --cflags vulkan)

LIBS := $(shell pkg-config --libs vulkan) \
	-L/opt/freetype/objs/.libs -lfreetype \
	-L/opt/freetype/cairo/builddir/src -lcairo \
	-L/work/java/ai-ide-jvm/harfbuzz/build/src -lharfbuzz \
	-L/opt/fribidi/build/lib -lfribidi \
	-L/opt/glfw/build/src -lglfw \
	-lm -lpthread \
	-Wl,-rpath,/opt/freetype/objs/.libs \
	-Wl,-rpath,/opt/freetype/cairo/builddir/src \
	-Wl,-rpath,/work/java/ai-ide-jvm/harfbuzz/build/src \
	-Wl,-rpath,/opt/fribidi/build/lib \
	-Wl,-rpath,/opt/glfw/build/src
```

---

## Verification

After building and updating Makefile:

```bash
# Check all libraries exist
ls -l /opt/freetype/objs/.libs/libfreetype.a
ls -l /opt/freetype/cairo/builddir/src/libcairo.so
ls -l /work/java/ai-ide-jvm/harfbuzz/build/libharfbuzz.a
ls -l /opt/fribidi/build/lib/libfribidi.a
ls -l /opt/glfw/build/src/libglfw3.a

# Verify NanoVG links against local libraries
cd /work/java/ai-ide-jvm/nanovg
make clean
make
ldd build/test_window | grep -E "freetype|cairo|harfbuzz|fribidi|glfw"
```

Expected output should show local paths, not `/usr/lib`.

---

## Notes

1. **Vulkan SDK** from tarball is acceptable - system libraries work fine
2. **HarfBuzz, FriBidi, GLFW** currently use system libraries via pkg-config
3. **FreeType and Cairo** already use local builds
4. After building FriBidi and GLFW, update Makefile to use local builds
5. All dependencies use static or shared libraries - prefer static for portability
