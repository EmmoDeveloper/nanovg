#!/bin/bash
#
# Compile GLSL shaders to SPIR-V
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if glslc is available (from Vulkan SDK)
if command -v glslc &> /dev/null; then
	COMPILER="glslc"
	echo "Using glslc compiler"
elif command -v glslangValidator &> /dev/null; then
	COMPILER="glslangValidator"
	echo "Using glslangValidator compiler"
else
	echo "Error: No GLSL compiler found. Please install Vulkan SDK."
	exit 1
fi

# Compile vertex shader
echo "Compiling fill.vert..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=vertex fill.vert -o fill.vert.spv
else
	glslangValidator -V fill.vert -o fill.vert.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile fill.vert"
	exit 1
fi

# Compile instanced text vertex shader
echo "Compiling text_instanced.vert..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=vertex text_instanced.vert -o text_instanced.vert.spv
else
	glslangValidator -V text_instanced.vert -o text_instanced.vert.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile text_instanced.vert"
	exit 1
fi

# Compile fragment shader
echo "Compiling fill.frag..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=fragment fill.frag -o fill.frag.spv
else
	glslangValidator -V fill.frag -o fill.frag.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile fill.frag"
	exit 1
fi

# Compile SDF text fragment shader
echo "Compiling text_sdf.frag..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=fragment text_sdf.frag -o text_sdf.frag.spv
else
	glslangValidator -V text_sdf.frag -o text_sdf.frag.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile text_sdf.frag"
	exit 1
fi

# Compile subpixel text fragment shader
echo "Compiling text_subpixel.frag..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=fragment text_subpixel.frag -o text_subpixel.frag.spv
else
	glslangValidator -V text_subpixel.frag -o text_subpixel.frag.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile text_subpixel.frag"
	exit 1
fi

# Compile MSDF text fragment shader
echo "Compiling text_msdf.frag..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=fragment text_msdf.frag -o text_msdf.frag.spv
else
	glslangValidator -V text_msdf.frag -o text_msdf.frag.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile text_msdf.frag"
	exit 1
fi

# Compile color text fragment shader
echo "Compiling text_color.frag..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=fragment text_color.frag -o text_color.frag.spv
else
	glslangValidator -V text_color.frag -o text_color.frag.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile text_color.frag"
	exit 1
fi

# Compile compute shader for path tessellation
echo "Compiling tessellate.comp..."
if [ "$COMPILER" = "glslc" ]; then
	glslc -fshader-stage=compute tessellate.comp -o tessellate.comp.spv
else
	glslangValidator -V tessellate.comp -o tessellate.comp.spv
fi

if [ $? -ne 0 ]; then
	echo "Error: Failed to compile tessellate.comp"
	exit 1
fi

echo "Shader compilation successful!"
echo "Generated: fill.vert.spv, text_instanced.vert.spv, fill.frag.spv, text_sdf.frag.spv, text_subpixel.frag.spv, text_msdf.frag.spv, text_color.frag.spv, tessellate.comp.spv"
