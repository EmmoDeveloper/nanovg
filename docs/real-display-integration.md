# Real Display Integration with Quaternion Color Space

## Overview

This document describes the integration of physical display detection with the quaternion-based color space transformation system.

## Architecture

```
Physical Display Detection
         ↓
NVGVkRealDisplay
         ↓
NVGVkDisplayColorManager (Quaternion System)
         ↓
Color Space Transforms
```

## Components

### 1. NVGVkRealDisplay (`nvg_vk_display_real.h/c`)

Combines physical display characteristics with Vulkan color space management:

```c
typedef struct NVGVkRealDisplay {
    NVGVkDisplayColorManager* colorManager;  // Quaternion color space system
    DisplayDetectionInfo physicalInfo;        // Physical display detection
    VkSurfaceFormatKHR* surfaceFormats;      // Available Vulkan formats
    float dpi;                                // Computed DPI
    float pixelDensity;                       // Pixels per mm
    float diagonalInches;                     // Screen size
    VkColorSpaceKHR recommendedColorSpace;    // Optimal color space
    int supportsHDR;                          // HDR capable
    int supportsWideGamut;                    // Wide gamut support
    int subpixelRenderingEnabled;             // Subpixel enabled
} NVGVkRealDisplay;
```

### 2. Display Detection Integration

**Physical Display Info:**
- Display name (manufacturer + model)
- Resolution (width × height @ refresh rate)
- Physical dimensions (mm)
- Subpixel layout (RGB, BGR, vertical, none)
- DPI calculation
- Pixel density

**Vulkan Surface Capabilities:**
- Available surface formats
- Supported color spaces
- HDR support detection
- Wide gamut detection

### 3. Color Space Selection

**Automatic Selection:**
```c
VkColorSpaceKHR nvgvk_real_display_choose_color_space(
    const NVGVkRealDisplay* display,
    int preferHDR,
    int preferWideGamut
);
```

**Priority Order:**
1. HDR10 ST.2084 (if preferHDR and supported)
2. HDR10 HLG (if preferHDR and supported)
3. Display P3 (if preferWideGamut and supported)
4. BT.2020 (if preferWideGamut and supported)
5. Adobe RGB (if preferWideGamut and supported)
6. sRGB (fallback)

### 4. Quaternion Transform Integration

The real display automatically creates and manages quaternion-based color space transforms:

```c
// Create real display (auto-detects)
NVGVkRealDisplay* display = nvgvk_real_display_create(physicalDevice, surface);

// Update transform for new source color space
nvgvk_real_display_update_transform(display, sourceColorSpace);

// Get current transform (includes quaternion decomposition)
const NVGVkColorSpaceTransform* transform = nvgvk_real_display_get_transform(display);

// Transform is validated with:
// - Determinant check
// - Orthogonality error
// - Quaternion normalization
```

## Features

### DPI Calculation

```c
float dpi = diagonal_pixels / diagonal_inches
```

Where:
- `diagonal_pixels = sqrt(width² + height²)`
- `diagonal_inches = sqrt((widthMM/25.4)² + (heightMM/25.4)²)`

### Subpixel Rendering

Automatically enables subpixel rendering if display supports it:
- Horizontal RGB
- Horizontal BGR
- Vertical RGB
- Vertical BGR

Maps to NanoVG subpixel modes for LCD text rendering.

### HDR Detection

Checks Vulkan surface formats for HDR color spaces:
- VK_COLOR_SPACE_HDR10_ST2084_EXT (PQ)
- VK_COLOR_SPACE_HDR10_HLG_EXT (HLG)
- VK_COLOR_SPACE_DOLBYVISION_EXT

### Wide Gamut Detection

Checks for extended color gamut support:
- Display P3
- BT.2020
- Adobe RGB

## API Usage

### Basic Usage

```c
// Create real display
NVGVkRealDisplay* display = nvgvk_real_display_create(
    physicalDevice,
    surface
);

// Print detailed information
nvgvk_real_display_print_info(display);

// Choose optimal color space
VkColorSpaceKHR colorSpace = nvgvk_real_display_choose_color_space(
    display,
    0,  // preferHDR
    display->supportsWideGamut  // preferWideGamut if available
);

// Get optimal format
VkSurfaceFormatKHR format = nvgvk_real_display_choose_format(
    display,
    colorSpace
);

// Update transform
nvgvk_real_display_update_transform(
    display,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR  // source
);

// Animate transitions
nvgvk_display_update_transition(display->colorManager, deltaTime);

// Cleanup
nvgvk_real_display_destroy(display);
```

### Display Information

```c
printf("Display: %s\n", display->physicalInfo.name);
printf("Resolution: %dx%d @ %dHz\n",
       display->physicalInfo.width,
       display->physicalInfo.height,
       display->physicalInfo.refreshRate);
printf("DPI: %.1f\n", display->dpi);
printf("Diagonal: %.1f\"\n", display->diagonalInches);
printf("Subpixel: %s\n",
       display_subpixel_name(display->physicalInfo.subpixel));
printf("HDR: %s\n", display->supportsHDR ? "Yes" : "No");
printf("Wide Gamut: %s\n", display->supportsWideGamut ? "Yes" : "No");
```

### Transform Information

```c
const NVGVkColorSpaceTransform* transform =
    nvgvk_real_display_get_transform(display);

if (transform->isValid) {
    printf("Quaternion: [%.3f, %.3f, %.3f, %.3f]\n",
           transform->decomposed.rotation.w,
           transform->decomposed.rotation.x,
           transform->decomposed.rotation.y,
           transform->decomposed.rotation.z);
    printf("Scale: [%.3f, %.3f, %.3f]\n",
           transform->decomposed.scale[0],
           transform->decomposed.scale[1],
           transform->decomposed.scale[2]);
    printf("Determinant: %.6f\n", transform->determinant);
    printf("Ortho Error: %.8f\n", transform->orthogonalityError);
}
```

## Example Output

```
=== Real Display Information ===
Name: Dell P2415Q
Resolution: 3840x2160 @ 60Hz
Physical Size: 527 x 296 mm (24.0")
DPI: 183.6
Pixel Density: 7.29 px/mm
Subpixel Layout: RGB (Horizontal)
Subpixel Rendering: Enabled

Capabilities:
  HDR Support: No
  Wide Gamut: Yes

Surface Formats: 7 available
   1. Format=44, ColorSpace=sRGB Non-linear
   2. Format=50, ColorSpace=sRGB Non-linear
   3. Format=43, ColorSpace=Display P3 Non-linear

Recommended Color Space: Display P3 Non-linear

Color Space Manager:
Display: Dell P2415Q
  HDR Capable: No
  Peak Luminance: 350.0 cd/m²
  Black Level: 0.200 cd/m²
  Supported Color Spaces: 3
    1. sRGB Non-linear
    2. Display P3 Non-linear
    3. Extended sRGB Linear

Active Transform:
Color Space Transform:
  Valid: Yes
  Determinant: 0.999998
  Orthogonality Error: 0.00000012
  HDR Scale: 1.00
  Quaternion: [0.9998, 0.0054, 0.0012, -0.0021]
  Scale: [1.0435, 0.9892, 1.0123]
  Matrix:
    [1.0434 -0.0109 0.0043]
    [0.0109  0.9892 -0.0025]
    [-0.0043 0.0025 1.0123]
================================
```

## Test Application

`test_real_display.c` demonstrates the complete system:

1. **Display Detection**: Automatically detects physical display
2. **Capability Query**: Checks HDR and wide gamut support
3. **Color Space Selection**: Chooses optimal color space
4. **Transform Validation**: Verifies quaternion transforms
5. **Visualization**: Renders display information with:
   - Display name and specs
   - Capabilities (HDR, wide gamut)
   - Active transform details
   - Subpixel rendering visualization

## Integration Points

### With NanoVG Context

```c
// Create NanoVG with real display detection
NVGVkRealDisplay* display = nvgvk_real_display_create(physDevice, surface);
NVGcontext* vg = nvgCreateVk(...);

// Use display info for rendering decisions
if (display->subpixelRenderingEnabled) {
    nvgTextSubpixelMode(vg, display_subpixel_to_nvg(display->physicalInfo.subpixel));
}

// Use display color space for accurate colors
nvgvk_real_display_update_transform(display, contentColorSpace);
```

### With Font Rendering

The subpixel layout is automatically used for LCD subpixel rendering:
- Horizontal RGB: Standard LCD monitors
- Horizontal BGR: Some laptops
- Vertical RGB/BGR: Rotated displays

### With HDR Content

```c
if (display->supportsHDR) {
    VkColorSpaceKHR hdrSpace = nvgvk_real_display_choose_color_space(
        display, 1, 0);  // preferHDR=1

    // Recreate swapchain with HDR color space
    // Transform will automatically handle HDR scaling
}
```

## Benefits

1. **Automatic Configuration**: No manual display setup required
2. **Color Accuracy**: Quaternion transforms ensure accurate color reproduction
3. **Subpixel Rendering**: Automatically enabled for sharper text
4. **HDR Support**: Detects and enables HDR when available
5. **Wide Gamut**: Utilizes extended color spaces when supported
6. **Numerical Stability**: Quaternion decomposition prevents color drift
7. **Smooth Transitions**: SLERP provides artifact-free color space changes

## Platform Support

- **Linux**: Full support via display_detection.c (Wayland + X11)
- **Windows**: Requires display detection implementation
- **macOS**: Requires display detection implementation

Fallback behavior when detection fails:
- 1920×1080 @ 60Hz
- 24" 16:9 display (96 DPI)
- Horizontal RGB subpixel
- sRGB color space

## Future Enhancements

1. **Display Profiles**: Save per-display color calibration
2. **Multi-Monitor**: Manage multiple displays simultaneously
3. **Gamut Mapping**: Advanced perceptual gamut mapping
4. **Tone Mapping**: HDR→SDR tone mapping operators
5. **Color Calibration**: ICC profile integration
6. **Dynamic Range**: Automatic content-based HDR switching
