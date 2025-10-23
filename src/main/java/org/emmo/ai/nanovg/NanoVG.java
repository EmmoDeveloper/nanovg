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
	public static final int NVG_SDF_TEXT = 1 << 7;
	public static final int NVG_SUBPIXEL_TEXT = 1 << 8;
	public static final int NVG_MSDF_TEXT = 1 << 13;
	public static final int NVG_COLOR_TEXT = 1 << 14;
	public static final int NVG_VIRTUAL_ATLAS = 1 << 15;

	// MSDF modes
	public static final int MSDF_MODE_BITMAP = 0;
	public static final int MSDF_MODE_SDF = 1;
	public static final int MSDF_MODE_MSDF = 2;

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
	 * Delete NanoVG context.
	 *
	 * @param ctx NanoVG context handle
	 */
	public static native void nvgDeleteVk(long ctx);

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
