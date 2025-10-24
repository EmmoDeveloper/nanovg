# NanoVG Java JNI API Reference

Complete reference for all NanoVG Java JNI bindings.

## Table of Contents
- [Constants](#constants)
- [Context Management](#context-management)
- [State Management](#state-management)
- [Color Utilities](#color-utilities)
- [Transforms](#transforms)
- [Paint & Gradients](#paint--gradients)
- [Paths & Shapes](#paths--shapes)
- [Rendering](#rendering)
- [Scissoring](#scissoring)
- [Images](#images)
- [Fonts](#fonts)
- [Text Rendering](#text-rendering)

## Constants

### Creation Flags
```java
int NVG_ANTIALIAS = 1 << 0;         // Enable antialiasing
int NVG_STENCIL_STROKES = 1 << 1;   // Use stencil buffer for strokes
int NVG_DEBUG = 1 << 2;             // Enable debug mode
int NVG_SDF_TEXT = 1 << 7;          // Enable SDF text rendering
int NVG_MSDF_TEXT = 1 << 13;        // Enable MSDF text rendering
int NVG_COLOR_EMOJI = 1 << 17;      // Enable color emoji support
```

### Blend Factors
```java
int NVG_ZERO = 1 << 0;
int NVG_ONE = 1 << 1;
int NVG_SRC_COLOR = 1 << 2;
int NVG_ONE_MINUS_SRC_COLOR = 1 << 3;
int NVG_DST_COLOR = 1 << 4;
int NVG_ONE_MINUS_DST_COLOR = 1 << 5;
int NVG_SRC_ALPHA = 1 << 6;
int NVG_ONE_MINUS_SRC_ALPHA = 1 << 7;
int NVG_DST_ALPHA = 1 << 8;
int NVG_ONE_MINUS_DST_ALPHA = 1 << 9;
int NVG_SRC_ALPHA_SATURATE = 1 << 10;
```

### Composite Operations
```java
int NVG_SOURCE_OVER = 0;        // Default: source over destination
int NVG_SOURCE_IN = 1;
int NVG_SOURCE_OUT = 2;
int NVG_ATOP = 3;
int NVG_DESTINATION_OVER = 4;
int NVG_DESTINATION_IN = 5;
int NVG_DESTINATION_OUT = 6;
int NVG_DESTINATION_ATOP = 7;
int NVG_LIGHTER = 8;
int NVG_COPY = 9;
int NVG_XOR = 10;
```

### Image Flags
```java
int NVG_IMAGE_GENERATE_MIPMAPS = 1 << 0;
int NVG_IMAGE_REPEATX = 1 << 1;
int NVG_IMAGE_REPEATY = 1 << 2;
int NVG_IMAGE_FLIPY = 1 << 3;
int NVG_IMAGE_PREMULTIPLIED = 1 << 4;
int NVG_IMAGE_NEAREST = 1 << 5;
```

### Alignment
```java
int NVG_ALIGN_LEFT = 1 << 0;
int NVG_ALIGN_CENTER = 1 << 1;
int NVG_ALIGN_RIGHT = 1 << 2;
int NVG_ALIGN_TOP = 1 << 3;
int NVG_ALIGN_MIDDLE = 1 << 4;
int NVG_ALIGN_BOTTOM = 1 << 5;
int NVG_ALIGN_BASELINE = 1 << 6;
```

### Winding & Solidity
```java
int NVG_CCW = 1;    // Counter-clockwise (solid)
int NVG_CW = 2;     // Clockwise (hole)
int NVG_SOLID = 1;  // Solid shape
int NVG_HOLE = 2;   // Hole in shape
```

### Line Caps & Joins
```java
int NVG_BUTT = 0;
int NVG_ROUND = 1;
int NVG_SQUARE = 2;
int NVG_MITER = 4;
int NVG_BEVEL = 3;
```

## Context Management

### nvgCreateVk
```java
long nvgCreateVk(long instance, long physicalDevice, long device, long queue,
                 int queueFamilyIndex, long renderPass, long commandPool,
                 long descriptorPool, int maxFrames, int flags)
```
Create NanoVG Vulkan context.

**Parameters:**
- `instance` - Vulkan instance handle
- `physicalDevice` - Vulkan physical device handle
- `device` - Vulkan logical device handle
- `queue` - Vulkan queue handle
- `queueFamilyIndex` - Queue family index
- `renderPass` - Vulkan render pass handle
- `commandPool` - Vulkan command pool handle
- `descriptorPool` - Vulkan descriptor pool handle
- `maxFrames` - Maximum frames in flight
- `flags` - Creation flags (e.g., `NVG_ANTIALIAS | NVG_MSDF_TEXT`)

**Returns:** Context handle (native pointer)

**Overloads:**
```java
// With emoji font from file
long nvgCreateVk(..., int flags, String emojiFontPath)

// With emoji font from memory
long nvgCreateVk(..., int flags, byte[] emojiFontData)
```

### nvgDeleteVk
```java
void nvgDeleteVk(long ctx)
```
Delete NanoVG Vulkan context and free resources.

### nvgBeginFrame
```java
void nvgBeginFrame(long ctx, float windowWidth, float windowHeight, float devicePixelRatio)
```
Begin drawing a new frame.

### nvgEndFrame
```java
void nvgEndFrame(long ctx)
```
End frame and flush rendering.

### nvgCancelFrame
```java
void nvgCancelFrame(long ctx)
```
Cancel current frame without rendering.

## State Management

### nvgSave
```java
void nvgSave(long ctx)
```
Push current state onto stack.

### nvgRestore
```java
void nvgRestore(long ctx)
```
Pop state from stack.

### nvgReset
```java
void nvgReset(long ctx)
```
Reset all state to defaults.

### nvgShapeAntiAlias
```java
void nvgShapeAntiAlias(long ctx, boolean enabled)
```
Enable or disable shape antialiasing.

## Color Utilities

### nvgRGB
```java
float[] nvgRGB(int r, int g, int b)
```
Create RGB color from byte values (0-255). Alpha set to 1.0.

**Returns:** `float[4]` with `[r, g, b, a]` values (0.0-1.0)

### nvgRGBf
```java
float[] nvgRGBf(float r, float g, float b)
```
Create RGB color from float values (0.0-1.0). Alpha set to 1.0.

### nvgRGBA
```java
float[] nvgRGBA(int r, int g, int b, int a)
```
Create RGBA color from byte values (0-255).

### nvgRGBAf
```java
float[] nvgRGBAf(float r, float g, float b, float a)
```
Create RGBA color from float values (0.0-1.0).

### nvgHSL
```java
float[] nvgHSL(float h, float s, float l)
```
Create color from HSL values. Alpha set to 1.0.

**Parameters:**
- `h` - Hue (0.0-1.0)
- `s` - Saturation (0.0-1.0)
- `l` - Lightness (0.0-1.0)

### nvgHSLA
```java
float[] nvgHSLA(float h, float s, float l, int a)
```
Create color from HSLA values.

### nvgLerpRGBA
```java
float[] nvgLerpRGBA(float[] c0, float[] c1, float u)
```
Linearly interpolate between two colors.

**Parameters:**
- `c0` - Start color
- `c1` - End color
- `u` - Interpolation factor (0.0-1.0)

### nvgTransRGBA
```java
float[] nvgTransRGBA(float[] c0, int a)
```
Set transparency of color (byte alpha 0-255).

### nvgTransRGBAf
```java
float[] nvgTransRGBAf(float[] c0, float a)
```
Set transparency of color (float alpha 0.0-1.0).

## Transforms

### Basic Transforms

#### nvgTranslate
```java
void nvgTranslate(long ctx, float x, float y)
```
Translate coordinate system.

#### nvgRotate
```java
void nvgRotate(long ctx, float angle)
```
Rotate coordinate system (angle in radians).

#### nvgScale
```java
void nvgScale(long ctx, float x, float y)
```
Scale coordinate system.

#### nvgSkewX
```java
void nvgSkewX(long ctx, float angle)
```
Skew coordinate system along X axis.

#### nvgSkewY
```java
void nvgSkewY(long ctx, float angle)
```
Skew coordinate system along Y axis.

#### nvgResetTransform
```java
void nvgResetTransform(long ctx)
```
Reset transform to identity.

#### nvgTransform
```java
void nvgTransform(long ctx, float a, float b, float c, float d, float e, float f)
```
Apply custom transform matrix `[a, b, c, d, e, f]`.

### Transform Utilities

#### nvgCurrentTransform
```java
float[] nvgCurrentTransform(long ctx)
```
Get current transform matrix.

**Returns:** `float[6]` with `[a, b, c, d, e, f]`

#### nvgTransformIdentity
```java
void nvgTransformIdentity(float[] dst)
```
Set transform to identity matrix.

**Parameters:**
- `dst` - Output array `float[6]`

#### nvgTransformTranslate
```java
void nvgTransformTranslate(float[] dst, float tx, float ty)
```
Create translation transform.

#### nvgTransformScale
```java
void nvgTransformScale(float[] dst, float sx, float sy)
```
Create scale transform.

#### nvgTransformRotate
```java
void nvgTransformRotate(float[] dst, float angle)
```
Create rotation transform (angle in radians).

#### nvgTransformSkewX
```java
void nvgTransformSkewX(float[] dst, float angle)
```
Create X skew transform.

#### nvgTransformSkewY
```java
void nvgTransformSkewY(float[] dst, float angle)
```
Create Y skew transform.

#### nvgTransformMultiply
```java
void nvgTransformMultiply(float[] dst, float[] src)
```
Multiply transforms: `dst = dst * src`.

#### nvgTransformPremultiply
```java
void nvgTransformPremultiply(float[] dst, float[] src)
```
Premultiply transforms: `dst = src * dst`.

#### nvgTransformInverse
```java
int nvgTransformInverse(float[] dst, float[] src)
```
Invert transform.

**Returns:** 1 on success, 0 if singular

#### nvgTransformPoint
```java
void nvgTransformPoint(float[] dst, float[] xform, float srcx, float srcy)
```
Transform a point.

**Parameters:**
- `dst` - Output `float[2]` for `[x, y]`
- `xform` - Transform matrix `float[6]`
- `srcx`, `srcy` - Input point

### Angle Conversion

#### nvgDegToRad
```java
float nvgDegToRad(float deg)
```
Convert degrees to radians.

#### nvgRadToDeg
```java
float nvgRadToDeg(float rad)
```
Convert radians to degrees.

## Paint & Gradients

### nvgLinearGradient
```java
long nvgLinearGradient(long ctx, float sx, float sy, float ex, float ey,
                       int icol_r, int icol_g, int icol_b, int icol_a,
                       int ocol_r, int ocol_g, int ocol_b, int ocol_a)
```
Create linear gradient from `(sx, sy)` to `(ex, ey)`.

**Returns:** Paint handle

### nvgBoxGradient
```java
long nvgBoxGradient(long ctx, float x, float y, float w, float h, float r, float f,
                    int icol_r, int icol_g, int icol_b, int icol_a,
                    int ocol_r, int ocol_g, int ocol_b, int ocol_a)
```
Create box gradient with rounded corners.

**Parameters:**
- `r` - Corner radius
- `f` - Feather amount

### nvgRadialGradient
```java
long nvgRadialGradient(long ctx, float cx, float cy, float inr, float outr,
                       int icol_r, int icol_g, int icol_b, int icol_a,
                       int ocol_r, int ocol_g, int ocol_b, int ocol_a)
```
Create radial gradient.

### nvgImagePattern
```java
long nvgImagePattern(long ctx, float ox, float oy, float ex, float ey,
                     float angle, int image, float alpha)
```
Create image pattern.

### nvgFillPaint
```java
void nvgFillPaint(long ctx, long paint)
```
Set fill paint.

### nvgStrokePaint
```java
void nvgStrokePaint(long ctx, long paint)
```
Set stroke paint.

### nvgFillColor
```java
void nvgFillColor(long ctx, int r, int g, int b, int a)
```
Set fill color.

### nvgStrokeColor
```java
void nvgStrokeColor(long ctx, int r, int g, int b, int a)
```
Set stroke color.

### nvgGlobalAlpha
```java
void nvgGlobalAlpha(long ctx, float alpha)
```
Set global alpha (0.0-1.0).

### nvgGlobalCompositeOperation
```java
void nvgGlobalCompositeOperation(long ctx, int op)
```
Set composite operation (use `NVG_SOURCE_OVER`, etc.).

### nvgGlobalCompositeBlendFunc
```java
void nvgGlobalCompositeBlendFunc(long ctx, int sfactor, int dfactor)
```
Set custom blend factors.

### nvgGlobalCompositeBlendFuncSeparate
```java
void nvgGlobalCompositeBlendFuncSeparate(long ctx, int srcRGB, int dstRGB,
                                          int srcAlpha, int dstAlpha)
```
Set separate RGB and alpha blend factors.

## Paths & Shapes

### nvgBeginPath
```java
void nvgBeginPath(long ctx)
```
Start new path.

### nvgMoveTo
```java
void nvgMoveTo(long ctx, float x, float y)
```
Move to point.

### nvgLineTo
```java
void nvgLineTo(long ctx, float x, float y)
```
Add line to path.

### nvgBezierTo
```java
void nvgBezierTo(long ctx, float c1x, float c1y, float c2x, float c2y, float x, float y)
```
Add cubic bezier curve.

### nvgQuadTo
```java
void nvgQuadTo(long ctx, float cx, float cy, float x, float y)
```
Add quadratic bezier curve.

### nvgArcTo
```java
void nvgArcTo(long ctx, float x1, float y1, float x2, float y2, float radius)
```
Add arc.

### nvgClosePath
```java
void nvgClosePath(long ctx)
```
Close path.

### nvgPathWinding
```java
void nvgPathWinding(long ctx, int dir)
```
Set path winding (`NVG_CCW` or `NVG_CW`).

### nvgRect
```java
void nvgRect(long ctx, float x, float y, float w, float h)
```
Add rectangle.

### nvgRoundedRect
```java
void nvgRoundedRect(long ctx, float x, float y, float w, float h, float r)
```
Add rounded rectangle.

### nvgRoundedRectVarying
```java
void nvgRoundedRectVarying(long ctx, float x, float y, float w, float h,
                           float radTopLeft, float radTopRight,
                           float radBottomRight, float radBottomLeft)
```
Add rounded rectangle with varying corner radii.

### nvgEllipse
```java
void nvgEllipse(long ctx, float cx, float cy, float rx, float ry)
```
Add ellipse.

### nvgCircle
```java
void nvgCircle(long ctx, float cx, float cy, float r)
```
Add circle.

### nvgArc
```java
void nvgArc(long ctx, float cx, float cy, float r, float a0, float a1, int dir)
```
Add arc (angles in radians).

## Rendering

### nvgFill
```java
void nvgFill(long ctx)
```
Fill current path.

### nvgStroke
```java
void nvgStroke(long ctx)
```
Stroke current path.

### nvgStrokeWidth
```java
void nvgStrokeWidth(long ctx, float width)
```
Set stroke width.

### nvgLineCap
```java
void nvgLineCap(long ctx, int cap)
```
Set line cap style (`NVG_BUTT`, `NVG_ROUND`, `NVG_SQUARE`).

### nvgLineJoin
```java
void nvgLineJoin(long ctx, int join)
```
Set line join style (`NVG_MITER`, `NVG_ROUND`, `NVG_BEVEL`).

### nvgMiterLimit
```java
void nvgMiterLimit(long ctx, float limit)
```
Set miter limit.

## Scissoring

### nvgScissor
```java
void nvgScissor(long ctx, float x, float y, float w, float h)
```
Set scissor rectangle.

### nvgIntersectScissor
```java
void nvgIntersectScissor(long ctx, float x, float y, float w, float h)
```
Intersect with current scissor.

### nvgResetScissor
```java
void nvgResetScissor(long ctx)
```
Reset scissor to no clipping.

## Images

### nvgCreateImage
```java
int nvgCreateImage(long ctx, String filename, int imageFlags)
```
Load image from file.

**Returns:** Image handle or 0 on error

### nvgCreateImageMem
```java
int nvgCreateImageMem(long ctx, int imageFlags, byte[] data)
```
Load image from memory.

### nvgCreateImageRGBA
```java
int nvgCreateImageRGBA(long ctx, int w, int h, int imageFlags, byte[] data)
```
Create image from RGBA pixel data.

### nvgUpdateImage
```java
void nvgUpdateImage(long ctx, int image, byte[] data)
```
Update image pixels.

### nvgImageSize
```java
int[] nvgImageSize(long ctx, int image)
```
Get image dimensions.

**Returns:** `int[2]` with `[width, height]`

### nvgDeleteImage
```java
void nvgDeleteImage(long ctx, int image)
```
Delete image.

## Fonts

### nvgCreateFont
```java
int nvgCreateFont(long ctx, String name, String filename)
```
Load font from file.

**Returns:** Font handle or -1 on error

### nvgCreateFontAtIndex
```java
int nvgCreateFontAtIndex(long ctx, String name, String filename, int fontIndex)
```
Load font at specific index from TTC/OTC file.

### nvgCreateFontMem
```java
int nvgCreateFontMem(long ctx, String name, byte[] data, boolean freeData)
```
Load font from memory.

### nvgCreateFontMemAtIndex
```java
int nvgCreateFontMemAtIndex(long ctx, String name, byte[] data, boolean freeData, int fontIndex)
```
Load font at specific index from memory.

### nvgFindFont
```java
int nvgFindFont(long ctx, String name)
```
Find font by name.

**Returns:** Font handle or -1 if not found

### nvgFontFace
```java
void nvgFontFace(long ctx, String font)
```
Set current font by name.

### nvgFontFaceId
```java
void nvgFontFaceId(long ctx, int font)
```
Set current font by ID.

### nvgAddFallbackFont
```java
int nvgAddFallbackFont(long ctx, String baseFont, String fallbackFont)
```
Add fallback font by name.

### nvgAddFallbackFontId
```java
int nvgAddFallbackFontId(long ctx, int baseFont, int fallbackFont)
```
Add fallback font by ID.

### nvgResetFallbackFonts
```java
void nvgResetFallbackFonts(long ctx, String baseFont)
```
Reset fallback fonts by name.

### nvgResetFallbackFontsId
```java
void nvgResetFallbackFontsId(long ctx, int baseFont)
```
Reset fallback fonts by ID.

## Text Rendering

### nvgFontSize
```java
void nvgFontSize(long ctx, float size)
```
Set font size.

### nvgFontBlur
```java
void nvgFontBlur(long ctx, float blur)
```
Set font blur amount.

### nvgTextAlign
```java
void nvgTextAlign(long ctx, int align)
```
Set text alignment (combine flags: `NVG_ALIGN_LEFT | NVG_ALIGN_TOP`).

### nvgTextLetterSpacing
```java
void nvgTextLetterSpacing(long ctx, float spacing)
```
Set letter spacing.

### nvgTextLineHeight
```java
void nvgTextLineHeight(long ctx, float lineHeight)
```
Set line height.

### nvgText
```java
void nvgText(long ctx, float x, float y, String text)
```
Render text at position.

### nvgTextBox
```java
void nvgTextBox(long ctx, float x, float y, float breakRowWidth, String text)
```
Render wrapped text.

### nvgTextBounds
```java
float[] nvgTextBounds(long ctx, float x, float y, String text)
```
Get text bounds.

**Returns:** `float[4]` with `[xmin, ymin, xmax, ymax]`

### nvgTextBoxBounds
```java
float[] nvgTextBoxBounds(long ctx, float x, float y, float breakRowWidth, String text)
```
Get wrapped text bounds.

**Returns:** `float[4]` with `[xmin, ymin, xmax, ymax]`

### nvgTextMetrics
```java
float[] nvgTextMetrics(long ctx)
```
Get font metrics.

**Returns:** `float[3]` with `[ascender, descender, lineh]`

### nvgTextGlyphPositions
```java
int nvgTextGlyphPositions(long ctx, float x, float y, String text,
                          float[] positions, int maxPositions)
```
Get glyph positions for custom layout.

**Parameters:**
- `positions` - Output array `float[maxPositions * 4]`
  - For each glyph: `[x, minx, maxx, reserved]`

**Returns:** Number of glyphs

### nvgTextBreakLines
```java
int nvgTextBreakLines(long ctx, String text, float breakRowWidth,
                      float[] rows, int maxRows)
```
Break text into lines.

**Parameters:**
- `rows` - Output array `float[maxRows * 4]`
  - For each row: `[startIdx, endIdx, width, minx]`

**Returns:** Number of rows

### nvgSetFontMSDF
```java
void nvgSetFontMSDF(long ctx, int font, int mode)
```
Enable MSDF rendering for font.

**Parameters:**
- `mode` - `MSDF_MODE_BITMAP` (0), `MSDF_MODE_SDF` (1), or `MSDF_MODE_MSDF` (2)

## License

zlib license (same as NanoVG)
