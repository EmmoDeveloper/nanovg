package org.emmo.ai.nanovg;

/**
 * NanoVG Java bindings with MSDF text rendering support.
 *
 * Multi-channel Signed Distance Field (MSDF) text rendering provides
 * high-quality scalable text at any size with sharp edges.
 */
public class NanoVG {

	// NanoVG creation flags
	public static final int NVG_ANTIALIAS = 1 << 0;
	public static final int NVG_STENCIL_STROKES = 1 << 1;
	public static final int NVG_DEBUG = 1 << 2;
	public static final int NVG_DYNAMIC_RENDERING = 1 << 4;
	public static final int NVG_SDF_TEXT = 1 << 7;
	public static final int NVG_SUBPIXEL_TEXT = 1 << 8;
	public static final int NVG_MSDF_TEXT = 1 << 13;
	public static final int NVG_COLOR_TEXT = 1 << 14;
	public static final int NVG_VIRTUAL_ATLAS = 1 << 15;
	public static final int NVG_COLOR_EMOJI = 1 << 17;

	// MSDF modes
	public static final int MSDF_MODE_BITMAP = 0;
	public static final int MSDF_MODE_SDF = 1;
	public static final int MSDF_MODE_MSDF = 2;

	// Winding
	public static final int NVG_CCW = 1;
	public static final int NVG_CW = 2;

	// Line caps
	public static final int NVG_BUTT = 0;
	public static final int NVG_ROUND = 1;
	public static final int NVG_SQUARE = 2;

	// Line joins
	public static final int NVG_MITER = 4;
	public static final int NVG_BEVEL = 3;

	// Text alignment
	public static final int NVG_ALIGN_LEFT = 1 << 0;
	public static final int NVG_ALIGN_CENTER = 1 << 1;
	public static final int NVG_ALIGN_RIGHT = 1 << 2;
	public static final int NVG_ALIGN_TOP = 1 << 3;
	public static final int NVG_ALIGN_MIDDLE = 1 << 4;
	public static final int NVG_ALIGN_BOTTOM = 1 << 5;
	public static final int NVG_ALIGN_BASELINE = 1 << 6;

	// Solidity
	public static final int NVG_SOLID = 1;
	public static final int NVG_HOLE = 2;

	// Blend factors
	public static final int NVG_ZERO = 1 << 0;
	public static final int NVG_ONE = 1 << 1;
	public static final int NVG_SRC_COLOR = 1 << 2;
	public static final int NVG_ONE_MINUS_SRC_COLOR = 1 << 3;
	public static final int NVG_DST_COLOR = 1 << 4;
	public static final int NVG_ONE_MINUS_DST_COLOR = 1 << 5;
	public static final int NVG_SRC_ALPHA = 1 << 6;
	public static final int NVG_ONE_MINUS_SRC_ALPHA = 1 << 7;
	public static final int NVG_DST_ALPHA = 1 << 8;
	public static final int NVG_ONE_MINUS_DST_ALPHA = 1 << 9;
	public static final int NVG_SRC_ALPHA_SATURATE = 1 << 10;

	// Composite operations
	public static final int NVG_SOURCE_OVER = 0;
	public static final int NVG_SOURCE_IN = 1;
	public static final int NVG_SOURCE_OUT = 2;
	public static final int NVG_ATOP = 3;
	public static final int NVG_DESTINATION_OVER = 4;
	public static final int NVG_DESTINATION_IN = 5;
	public static final int NVG_DESTINATION_OUT = 6;
	public static final int NVG_DESTINATION_ATOP = 7;
	public static final int NVG_LIGHTER = 8;
	public static final int NVG_COPY = 9;
	public static final int NVG_XOR = 10;

	// Image flags
	public static final int NVG_IMAGE_GENERATE_MIPMAPS = 1 << 0;
	public static final int NVG_IMAGE_REPEATX = 1 << 1;
	public static final int NVG_IMAGE_REPEATY = 1 << 2;
	public static final int NVG_IMAGE_FLIPY = 1 << 3;
	public static final int NVG_IMAGE_PREMULTIPLIED = 1 << 4;
	public static final int NVG_IMAGE_NEAREST = 1 << 5;

	static {
		System.loadLibrary("nanovg-jni");
	}

	/**
	 * Create NanoVG Vulkan context.
	 *
	 * @param instance Vulkan instance handle
	 * @param physicalDevice Vulkan physical device handle
	 * @param device Vulkan logical device handle
	 * @param queue Vulkan queue handle
	 * @param queueFamilyIndex Queue family index
	 * @param renderPass Vulkan render pass handle
	 * @param commandPool Vulkan command pool handle
	 * @param descriptorPool Vulkan descriptor pool handle
	 * @param maxFrames Maximum frames in flight
	 * @param flags NanoVG creation flags (e.g., NVG_ANTIALIAS | NVG_MSDF_TEXT)
	 * @return NanoVG context handle (native pointer)
	 */
	public static native long nvgCreateVk(
		long instance,
		long physicalDevice,
		long device,
		long queue,
		int queueFamilyIndex,
		long renderPass,
		long commandPool,
		long descriptorPool,
		int maxFrames,
		int flags
	);

	/**
	 * Create NanoVG Vulkan context with emoji font support.
	 *
	 * @param instance Vulkan instance handle
	 * @param physicalDevice Vulkan physical device handle
	 * @param device Vulkan logical device handle
	 * @param queue Vulkan queue handle
	 * @param queueFamilyIndex Queue family index
	 * @param renderPass Vulkan render pass handle
	 * @param commandPool Vulkan command pool handle
	 * @param descriptorPool Vulkan descriptor pool handle
	 * @param maxFrames Maximum frames in flight
	 * @param flags NanoVG creation flags (must include NVG_COLOR_EMOJI for emoji support)
	 * @param emojiFontPath Path to emoji font file (e.g., "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf")
	 * @return NanoVG context handle (native pointer)
	 */
	public static native long nvgCreateVk(
		long instance,
		long physicalDevice,
		long device,
		long queue,
		int queueFamilyIndex,
		long renderPass,
		long commandPool,
		long descriptorPool,
		int maxFrames,
		int flags,
		String emojiFontPath
	);

	/**
	 * Create NanoVG Vulkan context with pre-loaded emoji font data.
	 *
	 * @param instance Vulkan instance handle
	 * @param physicalDevice Vulkan physical device handle
	 * @param device Vulkan logical device handle
	 * @param queue Vulkan queue handle
	 * @param queueFamilyIndex Queue family index
	 * @param renderPass Vulkan render pass handle
	 * @param commandPool Vulkan command pool handle
	 * @param descriptorPool Vulkan descriptor pool handle
	 * @param maxFrames Maximum frames in flight
	 * @param flags NanoVG creation flags (must include NVG_COLOR_EMOJI for emoji support)
	 * @param emojiFontData Pre-loaded emoji font data
	 * @return NanoVG context handle (native pointer)
	 */
	public static native long nvgCreateVk(
		long instance,
		long physicalDevice,
		long device,
		long queue,
		int queueFamilyIndex,
		long renderPass,
		long commandPool,
		long descriptorPool,
		int maxFrames,
		int flags,
		byte[] emojiFontData
	);

	/**
	 * Delete NanoVG context.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgDeleteVk(long ctx);

	/**
	 * Set Vulkan render targets for NanoVG rendering.
	 *
	 * @param ctx NanoVG context handle
	 * @param colorImageView Vulkan color image view handle
	 * @param depthStencilImageView Vulkan depth/stencil image view handle
	 */
	public static native void nvgVkSetRenderTargets(long ctx, long colorImageView, long depthStencilImageView);

	/**
	 * Get the Vulkan command buffer used by NanoVG.
	 *
	 * @param ctx NanoVG context handle
	 * @return Vulkan command buffer handle
	 */
	public static native long nvgVkGetCommandBuffer(long ctx);

	/**
	 * Get NanoVG's render finished semaphore for frame synchronization.
	 *
	 * @param ctx NanoVG context handle
	 * @return Vulkan semaphore handle
	 */
	public static native long nvgVkGetRenderFinishedSemaphore(long ctx);

	/**
	 * Get NanoVG's frame fence for frame synchronization.
	 *
	 * @param ctx NanoVG context handle
	 * @return Vulkan fence handle
	 */
	public static native long nvgVkGetFrameFence(long ctx);

	/**
	 * Begin frame rendering.
	 *
	 * @param ctx NanoVG context handle
	 * @param width Window width
	 * @param height Window height
	 * @param devicePixelRatio Device pixel ratio (1.0 for standard displays)
	 */
	public static native void nvgBeginFrame(long ctx, float width, float height, float devicePixelRatio);

	/**
	 * End frame rendering.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgEndFrame(long ctx);

	/**
	 * Create font from file.
	 *
	 * @param ctx NanoVG context handle
	 * @param name Font name
	 * @param filename Path to font file
	 * @return Font handle, or -1 on error
	 */
	public static native int nvgCreateFont(long ctx, String name, String filename);

	/**
	 * Find font by name.
	 *
	 * @param ctx NanoVG context handle
	 * @param name Font name
	 * @return Font handle, or -1 if not found
	 */
	public static native int nvgFindFont(long ctx, String name);

	/**
	 * Set MSDF rendering mode for a font.
	 *
	 * IMPORTANT: Requires NVG_MSDF_TEXT flag when creating context.
	 *
	 * @param ctx NanoVG context handle
	 * @param font Font handle
	 * @param mode MSDF mode:
	 *             - MSDF_MODE_BITMAP (0): Standard bitmap rendering
	 *             - MSDF_MODE_SDF (1): Single-channel signed distance field
	 *             - MSDF_MODE_MSDF (2): Multi-channel signed distance field (highest quality)
	 */
	public static native void nvgSetFontMSDF(long ctx, int font, int mode);

	/**
	 * Set current font face.
	 *
	 * @param ctx NanoVG context handle
	 * @param font Font name
	 */
	public static native void nvgFontFace(long ctx, String font);

	/**
	 * Set font size.
	 *
	 * @param ctx NanoVG context handle
	 * @param size Font size in pixels
	 */
	public static native void nvgFontSize(long ctx, float size);

	/**
	 * Set fill color.
	 *
	 * @param ctx NanoVG context handle
	 * @param r Red (0-255)
	 * @param g Green (0-255)
	 * @param b Blue (0-255)
	 * @param a Alpha (0-255)
	 */
	public static native void nvgFillColor(long ctx, int r, int g, int b, int a);

	/**
	 * Render text.
	 *
	 * @param ctx NanoVG context handle
	 * @param x X position
	 * @param y Y position
	 * @param text Text to render
	 */
	public static native void nvgText(long ctx, float x, float y, String text);

	/**
	 * Begin path.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgBeginPath(long ctx);

	/**
	 * Draw rectangle.
	 *
	 * @param ctx NanoVG context handle
	 * @param x X position
	 * @param y Y position
	 * @param w Width
	 * @param h Height
	 */
	public static native void nvgRect(long ctx, float x, float y, float w, float h);

	/**
	 * Fill current path.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgFill(long ctx);

	/**
	 * Stroke current path.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgStroke(long ctx);

	// Path operations

	/**
	 * Start new sub-path at specified point.
	 */
	public static native void nvgMoveTo(long ctx, float x, float y);

	/**
	 * Add line segment to specified point.
	 */
	public static native void nvgLineTo(long ctx, float x, float y);

	/**
	 * Add cubic bezier curve.
	 */
	public static native void nvgBezierTo(long ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);

	/**
	 * Add quadratic bezier curve.
	 */
	public static native void nvgQuadTo(long ctx, float cx, float cy, float x, float y);

	/**
	 * Add arc segment.
	 */
	public static native void nvgArcTo(long ctx, float x1, float y1, float x2, float y2, float radius);

	/**
	 * Close current sub-path with line segment.
	 */
	public static native void nvgClosePath(long ctx);

	/**
	 * Set winding direction (NVG_CCW=1 or NVG_CW=2).
	 */
	public static native void nvgPathWinding(long ctx, int dir);

	/**
	 * Create arc path.
	 */
	public static native void nvgArc(long ctx, float cx, float cy, float r, float a0, float a1, int dir);

	/**
	 * Create rounded rectangle path.
	 */
	public static native void nvgRoundedRect(long ctx, float x, float y, float w, float h, float r);

	/**
	 * Create rounded rectangle with varying corner radii.
	 */
	public static native void nvgRoundedRectVarying(long ctx, float x, float y, float w, float h,
		float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

	/**
	 * Create ellipse path.
	 */
	public static native void nvgEllipse(long ctx, float cx, float cy, float rx, float ry);

	/**
	 * Create circle path.
	 */
	public static native void nvgCircle(long ctx, float cx, float cy, float r);

	// State management

	/**
	 * Push current state to stack.
	 */
	public static native void nvgSave(long ctx);

	/**
	 * Restore state from stack.
	 */
	public static native void nvgRestore(long ctx);

	/**
	 * Reset state to defaults.
	 */
	public static native void nvgReset(long ctx);

	// Transforms

	/**
	 * Reset transform to identity.
	 */
	public static native void nvgResetTransform(long ctx);

	/**
	 * Apply custom transform matrix.
	 */
	public static native void nvgTransform(long ctx, float a, float b, float c, float d, float e, float f);

	/**
	 * Translate coordinate system.
	 */
	public static native void nvgTranslate(long ctx, float x, float y);

	/**
	 * Rotate coordinate system (angle in radians).
	 */
	public static native void nvgRotate(long ctx, float angle);

	/**
	 * Skew coordinate system along X axis (angle in radians).
	 */
	public static native void nvgSkewX(long ctx, float angle);

	/**
	 * Skew coordinate system along Y axis (angle in radians).
	 */
	public static native void nvgSkewY(long ctx, float angle);

	/**
	 * Scale coordinate system.
	 */
	public static native void nvgScale(long ctx, float x, float y);

	// Stroke properties

	/**
	 * Set stroke color.
	 */
	public static native void nvgStrokeColor(long ctx, int r, int g, int b, int a);

	/**
	 * Set stroke width.
	 */
	public static native void nvgStrokeWidth(long ctx, float width);

	/**
	 * Set line cap style (NVG_BUTT=0, NVG_ROUND=1, NVG_SQUARE=2).
	 */
	public static native void nvgLineCap(long ctx, int cap);

	/**
	 * Set line join style (NVG_MITER=4, NVG_ROUND=1, NVG_BEVEL=3).
	 */
	public static native void nvgLineJoin(long ctx, int join);

	/**
	 * Set miter limit.
	 */
	public static native void nvgMiterLimit(long ctx, float limit);

	/**
	 * Set global alpha.
	 */
	public static native void nvgGlobalAlpha(long ctx, float alpha);

	// Scissoring

	/**
	 * Set scissor rectangle.
	 */
	public static native void nvgScissor(long ctx, float x, float y, float w, float h);

	/**
	 * Intersect with scissor rectangle.
	 */
	public static native void nvgIntersectScissor(long ctx, float x, float y, float w, float h);

	/**
	 * Reset scissoring.
	 */
	public static native void nvgResetScissor(long ctx);

	// Text layout

	/**
	 * Set text alignment (combination of NVG_ALIGN_* flags).
	 */
	public static native void nvgTextAlign(long ctx, int align);

	/**
	 * Set text letter spacing.
	 */
	public static native void nvgTextLetterSpacing(long ctx, float spacing);

	/**
	 * Set text line height.
	 */
	public static native void nvgTextLineHeight(long ctx, float lineHeight);

	/**
	 * Set font blur.
	 */
	public static native void nvgFontBlur(long ctx, float blur);

	/**
	 * Draw text box with line wrapping.
	 */
	public static native void nvgTextBox(long ctx, float x, float y, float breakRowWidth, String text);

	/**
	 * Measure text bounds. Returns bounds as [xmin, ymin, xmax, ymax].
	 */
	public static native float[] nvgTextBounds(long ctx, float x, float y, String text);

	/**
	 * Get text metrics. Returns [ascender, descender, lineHeight].
	 */
	public static native float[] nvgTextMetrics(long ctx);

	// ====================
	// Frame Control
	// ====================

	/** Cancel drawing the current frame. */
	public static native void nvgCancelFrame(long ctx);

	// ====================
	// Composite Operations
	// ====================

	/** Set composite operation (one of NVGcompositeOperation). */
	public static native void nvgGlobalCompositeOperation(long ctx, int op);

	/** Set composite operation with custom blend factors (one of NVGblendFactor). */
	public static native void nvgGlobalCompositeBlendFunc(long ctx, int sfactor, int dfactor);

	/** Set composite operation with separate RGB and alpha blend factors. */
	public static native void nvgGlobalCompositeBlendFuncSeparate(long ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

	// ====================
	// Paint/Gradient System
	// ====================

	/**
	 * Create linear gradient paint.
	 * Returns paint handle (long pointer to NVGpaint).
	 */
	public static native long nvgLinearGradient(long ctx, float sx, float sy, float ex, float ey,
		int icol_r, int icol_g, int icol_b, int icol_a,
		int ocol_r, int ocol_g, int ocol_b, int ocol_a);

	/**
	 * Create box gradient paint.
	 * Returns paint handle (long pointer to NVGpaint).
	 */
	public static native long nvgBoxGradient(long ctx, float x, float y, float w, float h, float r, float f,
		int icol_r, int icol_g, int icol_b, int icol_a,
		int ocol_r, int ocol_g, int ocol_b, int ocol_a);

	/**
	 * Create radial gradient paint.
	 * Returns paint handle (long pointer to NVGpaint).
	 */
	public static native long nvgRadialGradient(long ctx, float cx, float cy, float inr, float outr,
		int icol_r, int icol_g, int icol_b, int icol_a,
		int ocol_r, int ocol_g, int ocol_b, int ocol_a);

	/**
	 * Create image pattern paint.
	 * Returns paint handle (long pointer to NVGpaint).
	 */
	public static native long nvgImagePattern(long ctx, float ox, float oy, float ex, float ey,
		float angle, int image, float alpha);

	/** Set current fill style to a paint. */
	public static native void nvgFillPaint(long ctx, long paint);

	/** Set current stroke style to a paint. */
	public static native void nvgStrokePaint(long ctx, long paint);

	// ====================
	// Image System
	// ====================

	/** Create image from file. Returns image handle or 0 on error. */
	public static native int nvgCreateImage(long ctx, String filename, int imageFlags);

	/** Create image from memory. Returns image handle or 0 on error. */
	public static native int nvgCreateImageMem(long ctx, int imageFlags, byte[] data);

	/** Create image from RGBA pixel data. Returns image handle or 0 on error. */
	public static native int nvgCreateImageRGBA(long ctx, int w, int h, int imageFlags, byte[] data);

	/** Update image with new pixel data. */
	public static native void nvgUpdateImage(long ctx, int image, byte[] data);

	/** Get image dimensions. Returns [width, height]. */
	public static native int[] nvgImageSize(long ctx, int image);

	/** Delete image. */
	public static native void nvgDeleteImage(long ctx, int image);

	// ====================
	// Font Memory Loading
	// ====================

	/** Create font from memory buffer. Returns font handle or -1 on error. */
	public static native int nvgCreateFontMem(long ctx, String name, byte[] data, boolean freeData);

	/** Create font from memory buffer at specific index. Returns font handle or -1 on error. */
	public static native int nvgCreateFontMemAtIndex(long ctx, String name, byte[] data, boolean freeData, int fontIndex);

	/** Add fallback font. Returns 1 on success, 0 on error. */
	public static native int nvgAddFallbackFont(long ctx, String baseFont, String fallbackFont);

	/** Add fallback font by ID. Returns 1 on success, 0 on error. */
	public static native int nvgAddFallbackFontId(long ctx, int baseFont, int fallbackFont);

	/** Reset fallback fonts for a base font. */
	public static native void nvgResetFallbackFonts(long ctx, String baseFont);

	/** Set current font face by ID. */
	public static native void nvgFontFaceId(long ctx, int font);

	// ====================
	// Advanced Text Layout
	// ====================

	/**
	 * Get bounds of wrapped text box. Returns [xmin, ymin, xmax, ymax].
	 */
	public static native float[] nvgTextBoxBounds(long ctx, float x, float y, float breakRowWidth, String text);

	// ====================
	// Miscellaneous
	// ====================

	/** Set shape antialiasing on/off. */
	public static native void nvgShapeAntiAlias(long ctx, boolean enabled);

	/** Get current transform matrix. Returns [a, b, c, d, e, f]. */
	public static native float[] nvgCurrentTransform(long ctx);

	// ====================
	// Color Utilities
	// ====================

	/** Create RGB color. Returns [r, g, b, a] where a=1.0. */
	public static float[] nvgRGB(int r, int g, int b) {
		return new float[] {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
	}

	/** Create RGB color from floats. Returns [r, g, b, a] where a=1.0. */
	public static float[] nvgRGBf(float r, float g, float b) {
		return new float[] {r, g, b, 1.0f};
	}

	/** Create RGBA color. Returns [r, g, b, a]. */
	public static float[] nvgRGBA(int r, int g, int b, int a) {
		return new float[] {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
	}

	/** Create RGBA color from floats. Returns [r, g, b, a]. */
	public static float[] nvgRGBAf(float r, float g, float b, float a) {
		return new float[] {r, g, b, a};
	}

	/** Linearly interpolate between two colors. Returns [r, g, b, a]. */
	public static float[] nvgLerpRGBA(float[] c0, float[] c1, float u) {
		float u2 = 1 - u;
		return new float[] {
			c0[0] * u2 + c1[0] * u,
			c0[1] * u2 + c1[1] * u,
			c0[2] * u2 + c1[2] * u,
			c0[3] * u2 + c1[3] * u
		};
	}

	/** Set transparency of a color. Returns [r, g, b, a]. */
	public static float[] nvgTransRGBA(float[] c0, int a) {
		return new float[] {c0[0], c0[1], c0[2], a / 255.0f};
	}

	/** Set transparency of a color from float. Returns [r, g, b, a]. */
	public static float[] nvgTransRGBAf(float[] c0, float a) {
		return new float[] {c0[0], c0[1], c0[2], a};
	}

	/** Create HSL color. Returns [r, g, b, a] where a=1.0. */
	public static native float[] nvgHSL(float h, float s, float l);

	/** Create HSLA color. Returns [r, g, b, a]. */
	public static native float[] nvgHSLA(float h, float s, float l, int a);

	// ====================
	// Transform Utilities
	// ====================

	/** Set transform to identity. dst must be float[6]. */
	public static native void nvgTransformIdentity(float[] dst);

	/** Set transform to translation. dst must be float[6]. */
	public static native void nvgTransformTranslate(float[] dst, float tx, float ty);

	/** Set transform to scale. dst must be float[6]. */
	public static native void nvgTransformScale(float[] dst, float sx, float sy);

	/** Set transform to rotation. dst must be float[6]. */
	public static native void nvgTransformRotate(float[] dst, float angle);

	/** Set transform to skew along X axis. dst must be float[6]. */
	public static native void nvgTransformSkewX(float[] dst, float angle);

	/** Set transform to skew along Y axis. dst must be float[6]. */
	public static native void nvgTransformSkewY(float[] dst, float angle);

	/** Multiply two transforms. dst = dst * src. dst and src must be float[6]. */
	public static native void nvgTransformMultiply(float[] dst, float[] src);

	/** Premultiply two transforms. dst = src * dst. dst and src must be float[6]. */
	public static native void nvgTransformPremultiply(float[] dst, float[] src);

	/** Invert transform. Returns 1 on success, 0 if transform is singular. dst and src must be float[6]. */
	public static native int nvgTransformInverse(float[] dst, float[] src);

	/** Transform a point. Returns transformed coordinates in dst. */
	public static native void nvgTransformPoint(float[] dst, float[] xform, float srcx, float srcy);

	/** Convert degrees to radians. */
	public static float nvgDegToRad(float deg) {
		return deg / 180.0f * (float)Math.PI;
	}

	/** Convert radians to degrees. */
	public static float nvgRadToDeg(float rad) {
		return rad / (float)Math.PI * 180.0f;
	}

	// ====================
	// Additional Font Functions
	// ====================

	/** Create font from file at specific index. Returns font handle or -1 on error. */
	public static native int nvgCreateFontAtIndex(long ctx, String name, String filename, int fontIndex);

	/** Reset fallback fonts by ID. */
	public static native void nvgResetFallbackFontsId(long ctx, int baseFont);

	// ====================
	// Complex Text Layout
	// ====================

	/** Get glyph positions. Returns number of positions filled (max maxPositions). */
	public static native int nvgTextGlyphPositions(long ctx, float x, float y, String text, float[] positions, int maxPositions);

	/** Break text into lines. Returns number of rows filled (max maxRows). */
	public static native int nvgTextBreakLines(long ctx, String text, float breakRowWidth, float[] rows, int maxRows);

	/**
	 * Example usage with MSDF text rendering.
	 */
	public static void main(String[] args) {
		System.out.println("NanoVG Java Bindings with MSDF Support");
		System.out.println("======================================");
		System.out.println();
		System.out.println("Example usage:");
		System.out.println();
		System.out.println("  // Create context with MSDF support");
		System.out.println("  long ctx = NanoVG.nvgCreateVk(instance, physicalDevice, device,");
		System.out.println("      queue, queueFamilyIndex, renderPass, commandPool,");
		System.out.println("      descriptorPool, 3, NVG_ANTIALIAS | NVG_MSDF_TEXT);");
		System.out.println();
		System.out.println("  // Load font and enable MSDF");
		System.out.println("  int font = NanoVG.nvgCreateFont(ctx, \"sans\", \"/path/to/font.ttf\");");
		System.out.println("  NanoVG.nvgSetFontMSDF(ctx, font, MSDF_MODE_MSDF);");
		System.out.println();
		System.out.println("  // Render high-quality scalable text");
		System.out.println("  NanoVG.nvgBeginFrame(ctx, 800, 600, 1.0f);");
		System.out.println("  NanoVG.nvgFontFace(ctx, \"sans\");");
		System.out.println("  NanoVG.nvgFontSize(ctx, 48.0f);");
		System.out.println("  NanoVG.nvgFillColor(ctx, 255, 255, 255, 255);");
		System.out.println("  NanoVG.nvgText(ctx, 100, 100, \"High Quality MSDF Text!\");");
		System.out.println("  NanoVG.nvgEndFrame(ctx);");
		System.out.println();
		System.out.println("  // Cleanup");
		System.out.println("  NanoVG.nvgDeleteVk(ctx);");
	}
}
