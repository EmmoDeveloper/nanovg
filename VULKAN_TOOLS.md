# Vulkan SDK Tools Integration

This project integrates several Vulkan SDK tools to improve shader development, debugging, and performance analysis.

## Shader Build System

### Quick Start

```bash
# Compile all shaders
make shaders

# Compile with optimizations (release mode)
make shaders RELEASE=1

# Validate all compiled shaders
make validate-shaders

# Clean shader artifacts
make clean-shaders
```

### How It Works

The Makefile automatically:
1. **Compiles** GLSL source files (.vert, .frag, .comp) to SPIR-V (.spv)
2. **Validates** each shader with `spirv-val`
3. **Optimizes** shaders in release mode with `spirv-opt`
4. **Tracks** dependencies - only recompiles changed shaders

### Shader Compilation Details

- **Source files**: `shaders/*.vert`, `shaders/*.frag`, `shaders/*.comp`
- **Output files**: `shaders/*.spv`
- **Tools used**:
  - `glslangValidator` - GLSL to SPIR-V compiler
  - `spirv-val` - SPIR-V validator (catches errors early)
  - `spirv-opt` - SPIR-V optimizer (release builds only)

### Release Mode Optimizations

When building with `RELEASE=1`:
- Dead code elimination
- Constant propagation
- Instruction combining
- Debug info stripping
- Smaller binary size
- Faster execution

**Example:**
```bash
# Normal build
make shaders

# Optimized build (for production)
make shaders RELEASE=1
```

## Vulkan Call Capture & Replay

### GFXReconstruct Helper Script

The `gfxr-capture.sh` script simplifies capturing and replaying Vulkan API calls.

### Capturing Vulkan Calls

```bash
# Capture all Vulkan calls from a test
./gfxr-capture.sh capture ./build/test_unit_texture

# Capture with specific frames (e.g., frames 10-20)
GFXR_CAPTURE_FRAMES=10-20 ./gfxr-capture.sh capture ./build/test_init

# Capture with memory tracking
GFXR_MEMORY_TRACKING=true ./gfxr-capture.sh capture ./build/test_init
```

### Replaying Captures

```bash
# List all captures
./gfxr-capture.sh list

# Replay a capture
./gfxr-capture.sh replay captures/test_unit_texture_20251025_112345.gfxr

# Get detailed info about a capture
./gfxr-capture.sh info captures/test_unit_texture_20251025_112345.gfxr
```

### Use Cases

**1. Debug Rendering Issues**
```bash
# Capture the problematic frame
GFXR_CAPTURE_FRAMES=42 ./gfxr-capture.sh capture ./build/test_visual_emoji

# Replay frame-by-frame
./gfxr-capture.sh replay captures/test_visual_emoji_*.gfxr
```

**2. Performance Analysis**
```bash
# Capture with full tracking
GFXR_MEMORY_TRACKING=true ./gfxr-capture.sh capture ./build/test_benchmark

# Analyze the capture
./gfxr-capture.sh info captures/test_benchmark_*.gfxr
```

**3. Cross-GPU Testing**
```bash
# Capture on GPU A
./gfxr-capture.sh capture ./build/test_init

# Copy .gfxr file to different machine with GPU B
# Replay to test compatibility
./gfxr-capture.sh replay captures/test_init_*.gfxr
```

## Validation Layers

Validation layers are now **automatically enabled** in all tests via `test_utils.c`.

### What They Catch

- ✅ Invalid API usage
- ✅ Resource leaks
- ✅ Synchronization errors
- ✅ Memory mapping bugs
- ✅ Queue family mismatches
- ✅ Invalid image layouts

### Validation Output

Errors are printed to stderr with format:
```
[VULKAN ERROR VALIDATION] vkMapMemory(): memory has already be mapped.
The Vulkan spec states: memory must not be currently host mapped
```

### Disabling Validation (if needed)

Edit `tests/test_utils.c` and modify the validation layer check.

## Manual Tool Usage

If you need to use SDK tools directly:

### spirv-val - Validate SPIR-V

```bash
spirv-val shaders/fill.frag.spv
```

### spirv-opt - Optimize SPIR-V

```bash
# Optimize for performance
spirv-opt -O shaders/fill.frag.spv -o shaders/fill.frag.opt.spv

# Optimize for size
spirv-opt -Os shaders/fill.frag.spv -o shaders/fill.frag.small.spv
```

### spirv-dis - Disassemble SPIR-V

```bash
spirv-dis shaders/fill.frag.spv > fill.frag.asm
```

Useful for:
- Understanding shader instructions
- Debugging optimization issues
- Learning SPIR-V assembly

### spirv-reflect - Shader Reflection

```bash
spirv-reflect shaders/fill.frag.spv
```

Shows:
- Descriptor set bindings
- Uniform buffer layouts
- Push constant usage
- Input/output variables

### vulkaninfo - GPU Capabilities

```bash
# Full device info
vulkaninfo

# Quick summary
vulkaninfo --summary

# Export to JSON
vulkaninfo --json > gpu_info.json
```

## Troubleshooting

### Shader Compilation Fails

```bash
# Check glslangValidator is available
which glslangValidator

# Manually compile to see full error
glslangValidator -V shaders/fill.frag
```

### Validation Errors

Validation errors indicate real bugs. Fix them:
1. Read the error message carefully
2. Check the Vulkan spec link provided
3. Fix the code
4. Rerun tests

### GFXReconstruct Capture Empty

Ensure the layer is loaded:
```bash
# Verify layer exists
ls /opt/lunarg-vulkan-sdk/x86_64/share/vulkan/explicit_layer.d/
```

## Performance Tips

1. **Use RELEASE=1 for production shaders** - Significantly faster
2. **Validate shaders regularly** - Catches bugs early
3. **Capture minimal frames** - Use `GFXR_CAPTURE_FRAMES` to reduce file size
4. **Keep validation layers on during development** - Prevents subtle bugs

## Tool Locations

All tools are in: `/opt/lunarg-vulkan-sdk/x86_64/bin/`

- `glslangValidator` - GLSL/SPIR-V compiler
- `spirv-val` - SPIR-V validator
- `spirv-opt` - SPIR-V optimizer
- `spirv-dis` - SPIR-V disassembler
- `spirv-reflect` - Shader reflection
- `gfxrecon-replay` - Capture replay
- `gfxrecon-info` - Capture info
- `vulkaninfo` - Device capabilities

## Further Reading

- [Vulkan Validation Layers Guide](https://vulkan.lunarg.com/doc/view/latest/linux/validation_layers.html)
- [GFXReconstruct Documentation](https://vulkan.lunarg.com/doc/view/latest/linux/capture_tools.html)
- [SPIR-V Tools](https://github.com/KhronosGroup/SPIRV-Tools)
