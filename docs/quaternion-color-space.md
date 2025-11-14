# Quaternion-Based Color Space System

## Overview

The NanoVG Vulkan backend implements a quaternion-based matrix decomposition system for color space transformations. This provides numerical stability, smooth interpolation, and efficient composition of color space conversions.

## Architecture

### Core Components

1. **Matrix Decomposition** (`nvg_vk_color_space_math.c`)
   - Polar decomposition: `M = R × S` (rotation × scale)
   - Rotation stored as quaternion (4 floats)
   - Scale stored as vec3 (3 floats)
   - Total: 7 floats vs 9 for full matrix

2. **Color Space Transform** (`nvg_vk_display_color_space.h`)
   - Complete transformation from source to destination color space
   - Includes gamut matrix, transfer functions, HDR scaling
   - Validation: determinant, orthogonality error

3. **Display Manager** (`nvg_vk_display_color_space.c`)
   - Manages display capabilities and color spaces
   - Smooth transitions via quaternion SLERP
   - Transform composition and path finding

## Why Quaternions?

### Problem

Color space conversion matrices have two components:
- **Rotation**: Changes color primaries (RGB basis vectors)
- **Scale**: White point adaptation (luminance adjustments)

Traditional 3x3 matrix multiplication accumulates floating-point errors, leading to:
- Non-orthogonal matrices (distorted colors)
- Gimbal lock in interpolation
- Numerical instability in compositions

### Solution

**Quaternion representation** provides:

1. **Orthogonality Preservation**
   - Quaternions represent pure rotations
   - Automatically maintain orthogonality
   - Eliminates accumulated rounding errors

2. **Smooth Interpolation (SLERP)**
   - Spherical Linear Interpolation
   - Shortest path on rotation manifold
   - No gimbal lock or artifacts

3. **Efficient Composition**
   - Quaternion multiplication: O(16) operations
   - Matrix multiplication: O(27) operations
   - Compact representation

4. **Numerical Stability**
   - Normalized quaternions maintain unit length
   - Better conditioning than Euler angles
   - Robust under repeated operations

## Mathematical Foundation

### Polar Decomposition

Given color space conversion matrix `M`, decompose as:

```
M = R × S
```

Where:
- `R` = rotation matrix (orthogonal)
- `S` = scale matrix (diagonal)

**Algorithm:**
1. Compute `M^T × M = S^2`
2. Extract scale: `S_ii = sqrt((M^T × M)_ii)`
3. Compute rotation: `R = M × S^-1`
4. Convert `R` to quaternion

### Quaternion Conversion

**Matrix to Quaternion** (Shepperd's method):
```c
if (trace > 0) {
    s = sqrt(trace + 1) * 2
    w = 0.25 * s
    x = (m[7] - m[5]) / s
    y = (m[2] - m[6]) / s
    z = (m[3] - m[1]) / s
} else {
    // Use largest diagonal element
    // ... (see implementation)
}
```

**Quaternion to Matrix:**
```c
m[0] = 1 - 2*(yy + zz)
m[1] = 2*(xy - wz)
m[2] = 2*(xz + wy)
// ... etc
```

### SLERP Interpolation

Spherical Linear Interpolation between quaternions `a` and `b`:

```
slerp(a, b, t) = a * sin((1-t)θ) / sin(θ) + b * sin(tθ) / sin(θ)
```

Where `θ = acos(a · b)`

## API Usage

### Building a Transform

```c
NVGVkColorSpaceTransform transform;
nvgvk_build_color_space_transform(
    &NVGVK_PRIMARIES_BT709,      // sRGB/BT.709
    &NVGVK_PRIMARIES_DISPLAYP3,  // Display P3
    NVGVK_TRANSFER_SRGB,         // Source transfer
    NVGVK_TRANSFER_SRGB,         // Dest transfer
    1.0f,                        // HDR scale
    &transform
);
```

### Interpolating Between Color Spaces

```c
NVGVkColorSpaceTransform result;
nvgvk_interpolate_transforms(&transformA, &transformB, 0.5f, &result);
```

### Using Display Manager

```c
// Create manager
NVGVkDisplayColorManager* manager =
    nvgvk_display_color_manager_create(physicalDevice, surface);

// Set target color space (starts smooth transition)
nvgvk_display_set_target_color_space(
    manager,
    VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,  // target
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR         // source
);

// Update animation (call every frame)
nvgvk_display_update_transition(manager, deltaTime);

// Get current transform
const NVGVkColorSpaceTransform* current =
    nvgvk_display_get_current_transform(manager);
```

## Validation

Transforms are automatically validated:

1. **Determinant Check**
   - Should be positive (no reflection)
   - Close to 1.0 for volume-preserving transforms

2. **Orthogonality Check**
   - `R^T × R` should equal identity
   - Error < 0.01 for valid transforms

3. **Quaternion Normalization**
   - `w^2 + x^2 + y^2 + z^2 = 1`

## Performance

### Memory

| Representation | Floats | Bytes |
|---------------|--------|-------|
| Full matrix   | 9      | 36    |
| Quaternion + scale | 7  | 28    |
| **Savings**   | 22%    | 22%   |

### Operations

| Operation | Matrix | Quaternion |
|-----------|--------|------------|
| Multiply  | 27 ops | 16 ops     |
| Interpolate | N/A  | 1 SLERP    |
| Normalize | 9 ops  | 4 ops      |

## Color Space Graph

The display manager maintains a graph of registered color spaces:

```
         XYZ (canonical)
          ^
         / \
        /   \
       /     \
    sRGB   Display P3
      \       /
       \     /
        \   /
       BT.2020
```

Transforms can be composed along paths through the graph.

## Shader Integration

The final matrix is uploaded to shaders for GPU-side conversion:

```glsl
vec3 convertGamut(vec3 rgb, mat3 conversionMatrix) {
    return conversionMatrix * rgb;
}
```

The quaternion representation ensures the matrix remains orthogonal and numerically stable.

## Examples

### Example 1: Custom Color Space

```c
// Define custom primaries
NVGVkColorPrimaries customPrimaries = {
    {0.64f, 0.33f},  // Red
    {0.30f, 0.60f},  // Green
    {0.15f, 0.06f},  // Blue
    {0.3127f, 0.3290f}  // D65 white
};

// Build transform
nvgvk_build_color_space_transform(
    &NVGVK_PRIMARIES_BT709,
    &customPrimaries,
    NVGVK_TRANSFER_SRGB,
    NVGVK_TRANSFER_LINEAR,
    1.0f,
    &transform
);
```

### Example 2: HDR Color Space

```c
// sRGB SDR -> BT.2020 HDR
nvgvk_build_color_space_transform(
    &NVGVK_PRIMARIES_BT709,
    &NVGVK_PRIMARIES_BT2020,
    NVGVK_TRANSFER_SRGB,
    NVGVK_TRANSFER_PQ,          // PQ for HDR
    10000.0f / 80.0f,           // 80 nits -> 10000 nits
    &transform
);
```

### Example 3: Transform Composition

```c
// Compose: sRGB -> P3 -> BT.2020
NVGVkColorSpaceTransform srgbToP3, p3ToBt2020, composed;

nvgvk_build_color_space_transform(&NVGVK_PRIMARIES_BT709,
    &NVGVK_PRIMARIES_DISPLAYP3, ..., &srgbToP3);
nvgvk_build_color_space_transform(&NVGVK_PRIMARIES_DISPLAYP3,
    &NVGVK_PRIMARIES_BT2020, ..., &p3ToBt2020);

nvgvk_compose_transforms(&srgbToP3, &p3ToBt2020, &composed);
// composed = p3ToBt2020(srgbToP3(x))
```

## Testing

Run the quaternion color space tests:

```bash
./build/test_color_space_interpolation
./build/test_display_color_space
```

Tests verify:
- Matrix decomposition accuracy
- Quaternion SLERP interpolation
- Transform composition
- Numerical stability

## References

1. **Polar Decomposition**: "Matrix Computations" by Golub & Van Loan
2. **Quaternions**: "3D Math Primer for Graphics and Game Development" by Dunn & Parberry
3. **Color Spaces**: ITU-R BT.709-6, ITU-R BT.2020-2, SMPTE EG 432-1
4. **SLERP**: Ken Shoemake, "Animating Rotation with Quaternion Curves" (1985)

## Implementation Details

### Files

- `nvg_vk_color_space_math.h/c`: Quaternion math and matrix operations
- `nvg_vk_display_color_space.h/c`: Display manager and transforms
- `nvg_vk_color_space.c`: Integration with Vulkan color spaces

### Key Functions

- `nvgvk_mat3_decompose()`: Polar decomposition
- `nvgvk_mat3_compose()`: Recompose from quaternion + scale
- `nvgvk_quat_slerp()`: Spherical linear interpolation
- `nvgvk_interpolate_transforms()`: Smooth color space transitions
- `nvgvk_compose_transforms()`: Chain transformations
- `nvgvk_validate_transform()`: Check orthogonality and determinant

## Future Enhancements

1. **Gamut Mapping**: Perceptual gamut boundary mapping
2. **CAM Integration**: Use CIECAM02 for perceptually uniform transitions
3. **HDR Tone Mapping**: Advanced tone mapping operators (ACES, Reinhard)
4. **Color Appearance**: Adapt for viewing conditions
5. **Path Finding**: Dijkstra's algorithm for optimal color space routes
