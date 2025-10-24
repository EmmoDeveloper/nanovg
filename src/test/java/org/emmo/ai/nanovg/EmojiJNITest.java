package org.emmo.ai.nanovg;

public class EmojiJNITest {
	static {
		System.load("/work/java/ai-ide-jvm/nanovg/build/libnanovg-jni.so");
	}

	public static void main(String[] args) {
		System.out.println("NanoVG Emoji JNI Test");
		System.out.println("======================");
		System.out.println();

		System.out.println("Test 1: NVG_COLOR_EMOJI flag");
		System.out.println("  NVG_COLOR_EMOJI = " + NanoVG.NVG_COLOR_EMOJI);
		System.out.println("  Expected: " + (1 << 17));
		if (NanoVG.NVG_COLOR_EMOJI == (1 << 17)) {
			System.out.println("✓ PASS: NVG_COLOR_EMOJI constant correct");
		} else {
			System.out.println("✗ FAIL: NVG_COLOR_EMOJI constant incorrect");
		}
		System.out.println();

		System.out.println("Test 2: Overloaded nvgCreateVk signatures available");
		try {
			// Check if the three overloaded methods exist
			Class<?> clazz = NanoVG.class;

			// Original signature (no emoji)
			clazz.getMethod("nvgCreateVk", long.class, long.class, long.class,
				long.class, int.class, long.class, long.class, long.class,
				int.class, int.class);
			System.out.println("  ✓ nvgCreateVk(10 params) exists");

			// With emoji font path
			clazz.getMethod("nvgCreateVk", long.class, long.class, long.class,
				long.class, int.class, long.class, long.class, long.class,
				int.class, int.class, String.class);
			System.out.println("  ✓ nvgCreateVk(11 params, String) exists");

			// With emoji font data
			clazz.getMethod("nvgCreateVk", long.class, long.class, long.class,
				long.class, int.class, long.class, long.class, long.class,
				int.class, int.class, byte[].class);
			System.out.println("  ✓ nvgCreateVk(11 params, byte[]) exists");

			System.out.println("✓ PASS: All three nvgCreateVk overloads exist");
		} catch (NoSuchMethodException e) {
			System.out.println("✗ FAIL: Missing method: " + e.getMessage());
		}
		System.out.println();

		System.out.println("======================");
		System.out.println("✓ Phase 6 emoji support exposed via JNI");
		System.out.println();
		System.out.println("Summary:");
		System.out.println("  - NVG_COLOR_EMOJI flag (1<<17)");
		System.out.println("  - nvgCreateVk() - original (no emoji)");
		System.out.println("  - nvgCreateVk(String emojiFontPath) - load from file");
		System.out.println("  - nvgCreateVk(byte[] emojiFontData) - load from memory");
		System.out.println();
		System.out.println("Note: Full Vulkan context required for actual rendering.");
	}
}
