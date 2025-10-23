#!/bin/bash
# Build script for NanoVG JNI bindings

set -e

echo "Building NanoVG JNI bindings..."

# Configuration
BUILD_DIR="build"
JAVA_HOME="${JAVA_HOME:-$(dirname $(dirname $(readlink -f $(which javac))))}"
JNI_INCLUDES="-I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux"
NANOVG_INCLUDES="-I./src"
VULKAN_INCLUDES="-I/usr/include"

# Additional includes for dependencies
EXTRA_INCLUDES="$(pkg-config --cflags freetype2 fribidi libpng 2>/dev/null || echo '')"

# Compiler flags
CFLAGS="-std=c11 -Wall -Wextra -O2 -g -fPIC -D_POSIX_C_SOURCE=200809L -DFONS_USE_FREETYPE"
ALL_INCLUDES="${NANOVG_INCLUDES} ${VULKAN_INCLUDES} ${JNI_INCLUDES} ${EXTRA_INCLUDES}"

# Libraries
LIBS="-lvulkan -lpthread -lm $(pkg-config --libs freetype2 fribidi libpng 2>/dev/null || echo '-lfreetype -lfribidi -lpng')"

# HarfBuzz library
HARFBUZZ_DIR="${HARFBUZZ_DIR:-/work/java/ai-ide-jvm/harfbuzz}"
if [ -f "${HARFBUZZ_DIR}/build/libharfbuzz.a" ]; then
	HARFBUZZ_LIB="${HARFBUZZ_DIR}/build/libharfbuzz.a"
	HARFBUZZ_INCLUDES="-I${HARFBUZZ_DIR}/src"
	ALL_INCLUDES="${ALL_INCLUDES} ${HARFBUZZ_INCLUDES}"
	LIBS="${LIBS} ${HARFBUZZ_LIB} -lstdc++"
fi

# Create build directory
mkdir -p ${BUILD_DIR}

echo "Step 1: Building required object files with -fPIC..."

# Build nanovg.o
echo "  Compiling nanovg.o..."
gcc ${CFLAGS} ${ALL_INCLUDES} -c src/nanovg.c -o ${BUILD_DIR}/nanovg_pic.o

# Build MSDF support
echo "  Compiling nanovg_vk_msdf.o..."
gcc ${CFLAGS} ${ALL_INCLUDES} -c src/nanovg_vk_msdf.c -o ${BUILD_DIR}/nanovg_vk_msdf_pic.o

# Build virtual atlas
echo "  Compiling nanovg_vk_virtual_atlas.o..."
gcc ${CFLAGS} ${ALL_INCLUDES} -c src/nanovg_vk_virtual_atlas.c -o ${BUILD_DIR}/nanovg_vk_virtual_atlas_pic.o

# Build emoji backend objects
EMOJI_SOURCES="nanovg_vk_emoji_tables nanovg_vk_color_atlas nanovg_vk_bitmap_emoji nanovg_vk_colr_render nanovg_vk_emoji nanovg_vk_text_emoji"
for src in ${EMOJI_SOURCES}; do
	if [ -f src/${src}.c ]; then
		echo "  Compiling ${src}.o..."
		gcc ${CFLAGS} ${ALL_INCLUDES} -c src/${src}.c -o ${BUILD_DIR}/${src}_pic.o
	fi
done

echo "Step 2: Compiling JNI wrapper..."
gcc ${CFLAGS} ${ALL_INCLUDES} -c src/main/native/nanovg_jni.c -o ${BUILD_DIR}/nanovg_jni.o

echo "Step 3: Linking shared library..."

# Collect all object files
OBJECT_FILES="${BUILD_DIR}/nanovg_jni.o ${BUILD_DIR}/nanovg_pic.o ${BUILD_DIR}/nanovg_vk_msdf_pic.o ${BUILD_DIR}/nanovg_vk_virtual_atlas_pic.o"

# Add emoji backend objects if they exist
for src in ${EMOJI_SOURCES}; do
	if [ -f ${BUILD_DIR}/${src}_pic.o ]; then
		OBJECT_FILES="${OBJECT_FILES} ${BUILD_DIR}/${src}_pic.o"
	fi
done

# Link shared library
gcc -shared ${OBJECT_FILES} ${LIBS} -o ${BUILD_DIR}/libnanovg-jni.so

echo "✓ JNI library built: ${BUILD_DIR}/libnanovg-jni.so"
