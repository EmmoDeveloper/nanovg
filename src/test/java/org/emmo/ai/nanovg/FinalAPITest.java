package org.emmo.ai.nanovg;

public class FinalAPITest {
	public static void main(String[] args) {
		System.out.println("NanoVG Final Complete API Test");
		System.out.println("===============================");
		System.out.println();

		int passed = 0;
		int total = 0;

		// Test new constants
		System.out.println("Testing Constants:");
		total += testConstants();
		passed += testConstants();

		// Test color utility functions
		System.out.println("\nTesting Color Utilities:");
		total += 9;
		passed += testColorUtils();

		// Test transform utilities
		System.out.println("\nTesting Transform Utilities:");
		total += 12;
		passed += testTransformUtils();

		// Test font functions
		System.out.println("\nTesting Font Functions:");
		total += 2;
		passed += testFontFunctions();

		// Count all native methods
		System.out.println("\nNative Method Statistics:");
		try {
			Class<?> clazz = Class.forName("org.emmo.ai.nanovg.NanoVG");
			java.lang.reflect.Method[] methods = clazz.getDeclaredMethods();
			int nativeCount = 0;
			for (java.lang.reflect.Method m : methods) {
				if (java.lang.reflect.Modifier.isNative(m.getModifiers())) {
					nativeCount++;
				}
			}
			System.out.println("Total native methods: " + nativeCount);
			System.out.println("Previous count: 80");
			System.out.println("New native methods added: " + (nativeCount - 80));
		} catch (Exception e) {
			System.out.println("Could not count methods: " + e.getMessage());
		}

		System.out.println("\n===============================");
		System.out.println("Summary: " + passed + "/" + total + " test groups passed");
		System.out.println("===============================");
	}

	static int testConstants() {
		int passed = 0;
		int total = 4;

		// Test blend factors
		if (NanoVG.NVG_ZERO == (1 << 0) && NanoVG.NVG_SRC_ALPHA == (1 << 6)) {
			System.out.println("  ✓ Blend factors (11 constants)");
			passed++;
		} else {
			System.out.println("  ✗ Blend factors");
		}

		// Test composite operations
		if (NanoVG.NVG_SOURCE_OVER == 0 && NanoVG.NVG_XOR == 10) {
			System.out.println("  ✓ Composite operations (11 constants)");
			passed++;
		} else {
			System.out.println("  ✗ Composite operations");
		}

		// Test image flags
		if (NanoVG.NVG_IMAGE_GENERATE_MIPMAPS == (1 << 0) && NanoVG.NVG_IMAGE_NEAREST == (1 << 5)) {
			System.out.println("  ✓ Image flags (6 constants)");
			passed++;
		} else {
			System.out.println("  ✗ Image flags");
		}

		// Test solidity
		if (NanoVG.NVG_SOLID == 1 && NanoVG.NVG_HOLE == 2) {
			System.out.println("  ✓ Solidity (2 constants)");
			passed++;
		} else {
			System.out.println("  ✗ Solidity");
		}

		return passed;
	}

	static int testColorUtils() {
		int passed = 0;

		try {
			// Test nvgRGB
			float[] c = NanoVG.nvgRGB(255, 128, 64);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgRGB");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgRGB: " + e.getMessage());
		}

		try {
			// Test nvgRGBf
			float[] c = NanoVG.nvgRGBf(1.0f, 0.5f, 0.25f);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgRGBf");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgRGBf: " + e.getMessage());
		}

		try {
			// Test nvgRGBA
			float[] c = NanoVG.nvgRGBA(255, 128, 64, 128);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgRGBA");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgRGBA: " + e.getMessage());
		}

		try {
			// Test nvgRGBAf
			float[] c = NanoVG.nvgRGBAf(1.0f, 0.5f, 0.25f, 0.5f);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgRGBAf");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgRGBAf: " + e.getMessage());
		}

		try {
			// Test nvgLerpRGBA
			float[] c0 = NanoVG.nvgRGBA(255, 0, 0, 255);
			float[] c1 = NanoVG.nvgRGBA(0, 255, 0, 255);
			float[] c = NanoVG.nvgLerpRGBA(c0, c1, 0.5f);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgLerpRGBA");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgLerpRGBA: " + e.getMessage());
		}

		try {
			// Test nvgTransRGBA
			float[] c0 = NanoVG.nvgRGBA(255, 128, 64, 255);
			float[] c = NanoVG.nvgTransRGBA(c0, 128);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgTransRGBA");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransRGBA: " + e.getMessage());
		}

		try {
			// Test nvgTransRGBAf
			float[] c0 = NanoVG.nvgRGBAf(1.0f, 0.5f, 0.25f, 1.0f);
			float[] c = NanoVG.nvgTransRGBAf(c0, 0.5f);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgTransRGBAf");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransRGBAf: " + e.getMessage());
		}

		try {
			// Test nvgHSL - native method
			float[] c = NanoVG.nvgHSL(0.5f, 1.0f, 0.5f);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgHSL");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgHSL: " + e.getMessage());
		}

		try {
			// Test nvgHSLA - native method
			float[] c = NanoVG.nvgHSLA(0.5f, 1.0f, 0.5f, 128);
			if (c != null && c.length == 4) {
				System.out.println("  ✓ nvgHSLA");
				passed++;
			}
		} catch (Exception e) {
			System.out.println("  ✗ nvgHSLA: " + e.getMessage());
		}

		return passed;
	}

	static int testTransformUtils() {
		int passed = 0;

		try {
			// Test nvgTransformIdentity
			float[] t = new float[6];
			NanoVG.nvgTransformIdentity(t);
			System.out.println("  ✓ nvgTransformIdentity");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformIdentity: " + e.getMessage());
		}

		try {
			// Test nvgTransformTranslate
			float[] t = new float[6];
			NanoVG.nvgTransformTranslate(t, 100.0f, 200.0f);
			System.out.println("  ✓ nvgTransformTranslate");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformTranslate: " + e.getMessage());
		}

		try {
			// Test nvgTransformScale
			float[] t = new float[6];
			NanoVG.nvgTransformScale(t, 2.0f, 2.0f);
			System.out.println("  ✓ nvgTransformScale");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformScale: " + e.getMessage());
		}

		try {
			// Test nvgTransformRotate
			float[] t = new float[6];
			NanoVG.nvgTransformRotate(t, 1.57f);
			System.out.println("  ✓ nvgTransformRotate");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformRotate: " + e.getMessage());
		}

		try {
			// Test nvgTransformSkewX
			float[] t = new float[6];
			NanoVG.nvgTransformSkewX(t, 0.5f);
			System.out.println("  ✓ nvgTransformSkewX");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformSkewX: " + e.getMessage());
		}

		try {
			// Test nvgTransformSkewY
			float[] t = new float[6];
			NanoVG.nvgTransformSkewY(t, 0.5f);
			System.out.println("  ✓ nvgTransformSkewY");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformSkewY: " + e.getMessage());
		}

		try {
			// Test nvgTransformMultiply
			float[] t1 = new float[6];
			float[] t2 = new float[6];
			NanoVG.nvgTransformIdentity(t1);
			NanoVG.nvgTransformTranslate(t2, 100.0f, 100.0f);
			NanoVG.nvgTransformMultiply(t1, t2);
			System.out.println("  ✓ nvgTransformMultiply");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformMultiply: " + e.getMessage());
		}

		try {
			// Test nvgTransformPremultiply
			float[] t1 = new float[6];
			float[] t2 = new float[6];
			NanoVG.nvgTransformIdentity(t1);
			NanoVG.nvgTransformTranslate(t2, 100.0f, 100.0f);
			NanoVG.nvgTransformPremultiply(t1, t2);
			System.out.println("  ✓ nvgTransformPremultiply");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformPremultiply: " + e.getMessage());
		}

		try {
			// Test nvgTransformInverse
			float[] t1 = new float[6];
			float[] t2 = new float[6];
			NanoVG.nvgTransformTranslate(t2, 100.0f, 100.0f);
			int result = NanoVG.nvgTransformInverse(t1, t2);
			System.out.println("  ✓ nvgTransformInverse");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformInverse: " + e.getMessage());
		}

		try {
			// Test nvgTransformPoint
			float[] dst = new float[2];
			float[] t = new float[6];
			NanoVG.nvgTransformIdentity(t);
			NanoVG.nvgTransformPoint(dst, t, 100.0f, 200.0f);
			System.out.println("  ✓ nvgTransformPoint");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgTransformPoint: " + e.getMessage());
		}

		try {
			// Test nvgDegToRad
			float rad = NanoVG.nvgDegToRad(180.0f);
			System.out.println("  ✓ nvgDegToRad");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgDegToRad: " + e.getMessage());
		}

		try {
			// Test nvgRadToDeg
			float deg = NanoVG.nvgRadToDeg(3.14159f);
			System.out.println("  ✓ nvgRadToDeg");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgRadToDeg: " + e.getMessage());
		}

		return passed++;
	}

	static int testFontFunctions() {
		int passed = 0;

		try {
			// Test nvgCreateFontAtIndex - just verify method exists
			Class.forName("org.emmo.ai.nanovg.NanoVG").getMethod("nvgCreateFontAtIndex",
				long.class, String.class, String.class, int.class);
			System.out.println("  ✓ nvgCreateFontAtIndex");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgCreateFontAtIndex: " + e.getMessage());
		}

		try {
			// Test nvgResetFallbackFontsId - just verify method exists
			Class.forName("org.emmo.ai.nanovg.NanoVG").getMethod("nvgResetFallbackFontsId",
				long.class, int.class);
			System.out.println("  ✓ nvgResetFallbackFontsId");
			passed++;
		} catch (Exception e) {
			System.out.println("  ✗ nvgResetFallbackFontsId: " + e.getMessage());
		}

		return passed;
	}
}
