// NanoVG JNI bindings with MSDF support
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "org_emmo_ai_nanovg_NanoVG.h"

// NanoVG headers
#include "nanovg.h"
#define NANOVG_VK_IMPLEMENTATION
#include "nanovg_vk.h"

// Helper: Convert Java string to C string (caller must free)
static const char* jstring_to_cstring(JNIEnv* env, jstring jstr) {
	if (jstr == NULL) return NULL;
	const char* str = (*env)->GetStringUTFChars(env, jstr, NULL);
	if (str == NULL) return NULL;

	// Make a copy since we need to release the JNI string
	char* copy = strdup(str);
	(*env)->ReleaseStringUTFChars(env, jstr, str);
	return copy;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgCreateVk
 * Signature: (JJJJIJJJII)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateVk
  (JNIEnv* env, jclass cls, jlong instance, jlong physicalDevice, jlong device,
   jlong queue, jint queueFamilyIndex, jlong renderPass, jlong commandPool,
   jlong descriptorPool, jint maxFrames, jint flags) {

	(void)env;  // Unused
	(void)cls;  // Unused

	NVGVkCreateInfo createInfo = {0};
	createInfo.instance = (VkInstance)instance;
	createInfo.physicalDevice = (VkPhysicalDevice)physicalDevice;
	createInfo.device = (VkDevice)device;
	createInfo.queue = (VkQueue)queue;
	createInfo.queueFamilyIndex = (uint32_t)queueFamilyIndex;
	createInfo.renderPass = (VkRenderPass)renderPass;
	createInfo.commandPool = (VkCommandPool)commandPool;
	createInfo.descriptorPool = (VkDescriptorPool)descriptorPool;
	createInfo.maxFrames = (uint32_t)maxFrames;

	NVGcontext* ctx = nvgCreateVk(&createInfo, flags);
	return (jlong)ctx;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgDeleteVk
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgDeleteVk
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		nvgDeleteVk((NVGcontext*)ctx);
	}
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgBeginFrame
 * Signature: (JFFF)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgBeginFrame
  (JNIEnv* env, jclass cls, jlong ctx, jfloat width, jfloat height, jfloat devicePixelRatio) {

	(void)env;
	(void)cls;

	nvgBeginFrame((NVGcontext*)ctx, width, height, devicePixelRatio);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgEndFrame
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgEndFrame
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	nvgEndFrame((NVGcontext*)ctx);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgCreateFont
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateFont
  (JNIEnv* env, jclass cls, jlong ctx, jstring name, jstring filename) {

	(void)cls;

	const char* cName = jstring_to_cstring(env, name);
	const char* cFilename = jstring_to_cstring(env, filename);

	if (cName == NULL || cFilename == NULL) {
		if (cName) free((void*)cName);
		if (cFilename) free((void*)cFilename);
		return -1;
	}

	int font = nvgCreateFont((NVGcontext*)ctx, cName, cFilename);

	free((void*)cName);
	free((void*)cFilename);

	return font;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgFindFont
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFindFont
  (JNIEnv* env, jclass cls, jlong ctx, jstring name) {

	(void)cls;

	const char* cName = jstring_to_cstring(env, name);
	if (cName == NULL) return -1;

	int font = nvgFindFont((NVGcontext*)ctx, cName);
	free((void*)cName);

	return font;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgSetFontMSDF
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgSetFontMSDF
  (JNIEnv* env, jclass cls, jlong ctx, jint font, jint mode) {

	(void)env;
	(void)cls;

	nvgSetFontMSDF((NVGcontext*)ctx, font, mode);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgFontFace
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFontFace
  (JNIEnv* env, jclass cls, jlong ctx, jstring font) {

	(void)cls;

	const char* cFont = jstring_to_cstring(env, font);
	if (cFont == NULL) return;

	nvgFontFace((NVGcontext*)ctx, cFont);
	free((void*)cFont);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgFontSize
 * Signature: (JF)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFontSize
  (JNIEnv* env, jclass cls, jlong ctx, jfloat size) {

	(void)env;
	(void)cls;

	nvgFontSize((NVGcontext*)ctx, size);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgFillColor
 * Signature: (JIIII)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFillColor
  (JNIEnv* env, jclass cls, jlong ctx, jint r, jint g, jint b, jint a) {

	(void)env;
	(void)cls;

	nvgFillColor((NVGcontext*)ctx, nvgRGBA(r, g, b, a));
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgText
 * Signature: (JFFLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgText
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jstring text) {

	(void)cls;

	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return;

	nvgText((NVGcontext*)ctx, x, y, cText, NULL);
	free((void*)cText);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgBeginPath
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgBeginPath
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	nvgBeginPath((NVGcontext*)ctx);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgRect
 * Signature: (JFFFF)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRect
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h) {

	(void)env;
	(void)cls;

	nvgRect((NVGcontext*)ctx, x, y, w, h);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgFill
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFill
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	nvgFill((NVGcontext*)ctx);
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgStroke
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgStroke
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	nvgStroke((NVGcontext*)ctx);
}

// Path operations

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgMoveTo
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgMoveTo((NVGcontext*)ctx, x, y);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgLineTo
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgLineTo((NVGcontext*)ctx, x, y);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgBezierTo
  (JNIEnv* env, jclass cls, jlong ctx, jfloat c1x, jfloat c1y, jfloat c2x, jfloat c2y, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgBezierTo((NVGcontext*)ctx, c1x, c1y, c2x, c2y, x, y);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgQuadTo
  (JNIEnv* env, jclass cls, jlong ctx, jfloat cx, jfloat cy, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgQuadTo((NVGcontext*)ctx, cx, cy, x, y);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgArcTo
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x1, jfloat y1, jfloat x2, jfloat y2, jfloat radius) {

	(void)env; (void)cls;
	nvgArcTo((NVGcontext*)ctx, x1, y1, x2, y2, radius);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgClosePath
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgClosePath((NVGcontext*)ctx);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgPathWinding
  (JNIEnv* env, jclass cls, jlong ctx, jint dir) {

	(void)env; (void)cls;
	nvgPathWinding((NVGcontext*)ctx, dir);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgArc
  (JNIEnv* env, jclass cls, jlong ctx, jfloat cx, jfloat cy, jfloat r, jfloat a0, jfloat a1, jint dir) {

	(void)env; (void)cls;
	nvgArc((NVGcontext*)ctx, cx, cy, r, a0, a1, dir);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRoundedRect
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h, jfloat r) {

	(void)env; (void)cls;
	nvgRoundedRect((NVGcontext*)ctx, x, y, w, h, r);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRoundedRectVarying
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h,
   jfloat radTopLeft, jfloat radTopRight, jfloat radBottomRight, jfloat radBottomLeft) {

	(void)env; (void)cls;
	nvgRoundedRectVarying((NVGcontext*)ctx, x, y, w, h, radTopLeft, radTopRight, radBottomRight, radBottomLeft);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgEllipse
  (JNIEnv* env, jclass cls, jlong ctx, jfloat cx, jfloat cy, jfloat rx, jfloat ry) {

	(void)env; (void)cls;
	nvgEllipse((NVGcontext*)ctx, cx, cy, rx, ry);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCircle
  (JNIEnv* env, jclass cls, jlong ctx, jfloat cx, jfloat cy, jfloat r) {

	(void)env; (void)cls;
	nvgCircle((NVGcontext*)ctx, cx, cy, r);
}

// State management

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgSave
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgSave((NVGcontext*)ctx);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRestore
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgRestore((NVGcontext*)ctx);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgReset
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgReset((NVGcontext*)ctx);
}

// Transforms

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgResetTransform
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgResetTransform((NVGcontext*)ctx);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransform
  (JNIEnv* env, jclass cls, jlong ctx, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f) {

	(void)env; (void)cls;
	nvgTransform((NVGcontext*)ctx, a, b, c, d, e, f);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTranslate
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgTranslate((NVGcontext*)ctx, x, y);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRotate
  (JNIEnv* env, jclass cls, jlong ctx, jfloat angle) {

	(void)env; (void)cls;
	nvgRotate((NVGcontext*)ctx, angle);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgSkewX
  (JNIEnv* env, jclass cls, jlong ctx, jfloat angle) {

	(void)env; (void)cls;
	nvgSkewX((NVGcontext*)ctx, angle);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgSkewY
  (JNIEnv* env, jclass cls, jlong ctx, jfloat angle) {

	(void)env; (void)cls;
	nvgSkewY((NVGcontext*)ctx, angle);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgScale
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y) {

	(void)env; (void)cls;
	nvgScale((NVGcontext*)ctx, x, y);
}

// Stroke properties

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgStrokeColor
  (JNIEnv* env, jclass cls, jlong ctx, jint r, jint g, jint b, jint a) {

	(void)env; (void)cls;
	nvgStrokeColor((NVGcontext*)ctx, nvgRGBA(r, g, b, a));
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgStrokeWidth
  (JNIEnv* env, jclass cls, jlong ctx, jfloat width) {

	(void)env; (void)cls;
	nvgStrokeWidth((NVGcontext*)ctx, width);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgLineCap
  (JNIEnv* env, jclass cls, jlong ctx, jint cap) {

	(void)env; (void)cls;
	nvgLineCap((NVGcontext*)ctx, cap);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgLineJoin
  (JNIEnv* env, jclass cls, jlong ctx, jint join) {

	(void)env; (void)cls;
	nvgLineJoin((NVGcontext*)ctx, join);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgMiterLimit
  (JNIEnv* env, jclass cls, jlong ctx, jfloat limit) {

	(void)env; (void)cls;
	nvgMiterLimit((NVGcontext*)ctx, limit);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgGlobalAlpha
  (JNIEnv* env, jclass cls, jlong ctx, jfloat alpha) {

	(void)env; (void)cls;
	nvgGlobalAlpha((NVGcontext*)ctx, alpha);
}

// Scissoring

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgScissor
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h) {

	(void)env; (void)cls;
	nvgScissor((NVGcontext*)ctx, x, y, w, h);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgIntersectScissor
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h) {

	(void)env; (void)cls;
	nvgIntersectScissor((NVGcontext*)ctx, x, y, w, h);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgResetScissor
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env; (void)cls;
	nvgResetScissor((NVGcontext*)ctx);
}

// Text layout

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextAlign
  (JNIEnv* env, jclass cls, jlong ctx, jint align) {

	(void)env; (void)cls;
	nvgTextAlign((NVGcontext*)ctx, align);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextLetterSpacing
  (JNIEnv* env, jclass cls, jlong ctx, jfloat spacing) {

	(void)env; (void)cls;
	nvgTextLetterSpacing((NVGcontext*)ctx, spacing);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextLineHeight
  (JNIEnv* env, jclass cls, jlong ctx, jfloat lineHeight) {

	(void)env; (void)cls;
	nvgTextLineHeight((NVGcontext*)ctx, lineHeight);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFontBlur
  (JNIEnv* env, jclass cls, jlong ctx, jfloat blur) {

	(void)env; (void)cls;
	nvgFontBlur((NVGcontext*)ctx, blur);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextBox
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat breakRowWidth, jstring text) {

	(void)cls;
	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return;

	nvgTextBox((NVGcontext*)ctx, x, y, breakRowWidth, cText, NULL);
	free((void*)cText);
}

JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextBounds
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jstring text) {

	(void)cls;
	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return NULL;

	float bounds[4];
	nvgTextBounds((NVGcontext*)ctx, x, y, cText, NULL, bounds);
	free((void*)cText);

	jfloatArray result = (*env)->NewFloatArray(env, 4);
	if (result != NULL) {
		(*env)->SetFloatArrayRegion(env, result, 0, 4, bounds);
	}
	return result;
}

JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextMetrics
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)cls;
	float ascender, descender, lineHeight;
	nvgTextMetrics((NVGcontext*)ctx, &ascender, &descender, &lineHeight);

	jfloatArray result = (*env)->NewFloatArray(env, 3);
	if (result != NULL) {
		float metrics[3] = {ascender, descender, lineHeight};
		(*env)->SetFloatArrayRegion(env, result, 0, 3, metrics);
	}
	return result;
}
