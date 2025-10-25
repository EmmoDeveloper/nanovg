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
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateVk__JJJJIJJJII
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
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_UNDEFINED;
	createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	NVGcontext* ctx = nvgCreateVk(&createInfo, flags);
	return (jlong)ctx;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgCreateVk (with emoji font path)
 * Signature: (JJJJIJJJIILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateVk__JJJJIJJJIILjava_lang_String_2
  (JNIEnv* env, jclass cls, jlong instance, jlong physicalDevice, jlong device,
   jlong queue, jint queueFamilyIndex, jlong renderPass, jlong commandPool,
   jlong descriptorPool, jint maxFrames, jint flags, jstring emojiFontPath) {

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
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_UNDEFINED;
	createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	// Convert emoji font path from Java string to C string
	const char* cEmojiFontPath = jstring_to_cstring(env, emojiFontPath);
	createInfo.emojiFontPath = cEmojiFontPath;
	createInfo.emojiFontData = NULL;
	createInfo.emojiFontDataSize = 0;

	NVGcontext* ctx = nvgCreateVk(&createInfo, flags);

	// Free the converted string
	if (cEmojiFontPath != NULL) {
		free((void*)cEmojiFontPath);
	}

	return (jlong)ctx;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgCreateVk (with emoji font data)
 * Signature: (JJJJIJJJII[B)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateVk__JJJJIJJJII_3B
  (JNIEnv* env, jclass cls, jlong instance, jlong physicalDevice, jlong device,
   jlong queue, jint queueFamilyIndex, jlong renderPass, jlong commandPool,
   jlong descriptorPool, jint maxFrames, jint flags, jbyteArray emojiFontData) {

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
	createInfo.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.depthStencilFormat = VK_FORMAT_UNDEFINED;
	createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	// Get emoji font data from Java byte array
	jsize dataSize = (*env)->GetArrayLength(env, emojiFontData);
	uint8_t* fontData = (uint8_t*)malloc(dataSize);
	if (fontData == NULL) {
		return 0;  // Out of memory
	}

	// Copy data from Java array to C buffer
	(*env)->GetByteArrayRegion(env, emojiFontData, 0, dataSize, (jbyte*)fontData);

	createInfo.emojiFontPath = NULL;
	createInfo.emojiFontData = fontData;
	createInfo.emojiFontDataSize = (uint32_t)dataSize;

	NVGcontext* ctx = nvgCreateVk(&createInfo, flags);

	// Note: fontData is NOT freed here because nvgCreateVk may keep a reference to it
	// The NanoVG context will be responsible for freeing it when nvgDeleteVk is called

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
 * Method:    nvgVkSetRenderTargets
 * Signature: (JJJ)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkSetRenderTargets
  (JNIEnv* env, jclass cls, jlong ctx, jlong colorImageView, jlong depthStencilImageView) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		nvgVkSetRenderTargets((NVGcontext*)ctx, (VkImageView)colorImageView, (VkImageView)depthStencilImageView);
	}
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkGetCommandBuffer
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkGetCommandBuffer
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		return (jlong)nvgVkGetCommandBuffer((NVGcontext*)ctx);
	}
	return 0;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkGetRenderFinishedSemaphore
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkGetRenderFinishedSemaphore
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		return (jlong)nvgVkGetRenderFinishedSemaphore((NVGcontext*)ctx);
	}
	return 0;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkGetFrameFence
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkGetFrameFence
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		return (jlong)nvgVkGetFrameFence((NVGcontext*)ctx);
	}
	return 0;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkSetCurrentFrame
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkSetCurrentFrame
  (JNIEnv* env, jclass cls, jlong ctx, jint frameIndex) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		nvgVkSetCurrentFrame((NVGcontext*)ctx, (uint32_t)frameIndex);
	}
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkGetImageAvailableSemaphore
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkGetImageAvailableSemaphore
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		return (jlong)nvgVkGetImageAvailableSemaphore((NVGcontext*)ctx);
	}
	return 0;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkCreateImageFromHandle
 * Signature: (JJJIII)I
 */
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkCreateImageFromHandle
  (JNIEnv* env, jclass cls, jlong ctx, jlong image, jlong imageView, jint width, jint height, jint imageFlags) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		return nvgVkCreateImageFromHandle((NVGcontext*)ctx, (VkImage)image, (VkImageView)imageView,
		                                   width, height, imageFlags);
	}
	return -1;
}

/*
 * Class:     org_emmo_ai_nanovg_NanoVG
 * Method:    nvgVkResetProfiling
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgVkResetProfiling
  (JNIEnv* env, jclass cls, jlong ctx) {

	(void)env;
	(void)cls;

	if (ctx != 0) {
		nvgVkResetProfiling((NVGcontext*)ctx);
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

// ====================
// NEW JNI IMPLEMENTATIONS
// ====================

// Frame Control
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCancelFrame
  (JNIEnv* env, jclass cls, jlong ctx) {
	(void)env; (void)cls;
	nvgCancelFrame((NVGcontext*)ctx);
}

// Composite Operations
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgGlobalCompositeOperation
  (JNIEnv* env, jclass cls, jlong ctx, jint op) {
	(void)env; (void)cls;
	nvgGlobalCompositeOperation((NVGcontext*)ctx, op);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgGlobalCompositeBlendFunc
  (JNIEnv* env, jclass cls, jlong ctx, jint sfactor, jint dfactor) {
	(void)env; (void)cls;
	nvgGlobalCompositeBlendFunc((NVGcontext*)ctx, sfactor, dfactor);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgGlobalCompositeBlendFuncSeparate
  (JNIEnv* env, jclass cls, jlong ctx, jint srcRGB, jint dstRGB, jint srcAlpha, jint dstAlpha) {
	(void)env; (void)cls;
	nvgGlobalCompositeBlendFuncSeparate((NVGcontext*)ctx, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

// Paint/Gradient System
JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgLinearGradient
  (JNIEnv* env, jclass cls, jlong ctx, jfloat sx, jfloat sy, jfloat ex, jfloat ey,
   jint icol_r, jint icol_g, jint icol_b, jint icol_a,
   jint ocol_r, jint ocol_g, jint ocol_b, jint ocol_a) {
	(void)env; (void)cls;
	NVGcolor icol = nvgRGBA(icol_r, icol_g, icol_b, icol_a);
	NVGcolor ocol = nvgRGBA(ocol_r, ocol_g, ocol_b, ocol_a);
	NVGpaint* paint = (NVGpaint*)malloc(sizeof(NVGpaint));
	if (paint) {
		*paint = nvgLinearGradient((NVGcontext*)ctx, sx, sy, ex, ey, icol, ocol);
	}
	return (jlong)paint;
}

JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgBoxGradient
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat w, jfloat h, jfloat r, jfloat f,
   jint icol_r, jint icol_g, jint icol_b, jint icol_a,
   jint ocol_r, jint ocol_g, jint ocol_b, jint ocol_a) {
	(void)env; (void)cls;
	NVGcolor icol = nvgRGBA(icol_r, icol_g, icol_b, icol_a);
	NVGcolor ocol = nvgRGBA(ocol_r, ocol_g, ocol_b, ocol_a);
	NVGpaint* paint = (NVGpaint*)malloc(sizeof(NVGpaint));
	if (paint) {
		*paint = nvgBoxGradient((NVGcontext*)ctx, x, y, w, h, r, f, icol, ocol);
	}
	return (jlong)paint;
}

JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgRadialGradient
  (JNIEnv* env, jclass cls, jlong ctx, jfloat cx, jfloat cy, jfloat inr, jfloat outr,
   jint icol_r, jint icol_g, jint icol_b, jint icol_a,
   jint ocol_r, jint ocol_g, jint ocol_b, jint ocol_a) {
	(void)env; (void)cls;
	NVGcolor icol = nvgRGBA(icol_r, icol_g, icol_b, icol_a);
	NVGcolor ocol = nvgRGBA(ocol_r, ocol_g, ocol_b, ocol_a);
	NVGpaint* paint = (NVGpaint*)malloc(sizeof(NVGpaint));
	if (paint) {
		*paint = nvgRadialGradient((NVGcontext*)ctx, cx, cy, inr, outr, icol, ocol);
	}
	return (jlong)paint;
}

JNIEXPORT jlong JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgImagePattern
  (JNIEnv* env, jclass cls, jlong ctx, jfloat ox, jfloat oy, jfloat ex, jfloat ey,
   jfloat angle, jint image, jfloat alpha) {
	(void)env; (void)cls;
	NVGpaint* paint = (NVGpaint*)malloc(sizeof(NVGpaint));
	if (paint) {
		*paint = nvgImagePattern((NVGcontext*)ctx, ox, oy, ex, ey, angle, image, alpha);
	}
	return (jlong)paint;
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFillPaint
  (JNIEnv* env, jclass cls, jlong ctx, jlong paint) {
	(void)env; (void)cls;
	if (paint != 0) {
		nvgFillPaint((NVGcontext*)ctx, *(NVGpaint*)paint);
	}
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgStrokePaint
  (JNIEnv* env, jclass cls, jlong ctx, jlong paint) {
	(void)env; (void)cls;
	if (paint != 0) {
		nvgStrokePaint((NVGcontext*)ctx, *(NVGpaint*)paint);
	}
}

// Image System
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateImage
  (JNIEnv* env, jclass cls, jlong ctx, jstring filename, jint imageFlags) {
	(void)cls;
	const char* cFilename = jstring_to_cstring(env, filename);
	if (cFilename == NULL) return 0;
	
	int image = nvgCreateImage((NVGcontext*)ctx, cFilename, imageFlags);
	free((void*)cFilename);
	return image;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateImageMem
  (JNIEnv* env, jclass cls, jlong ctx, jint imageFlags, jbyteArray data) {
	(void)cls;
	jsize dataSize = (*env)->GetArrayLength(env, data);
	unsigned char* bytes = (unsigned char*)malloc(dataSize);
	if (bytes == NULL) return 0;
	
	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte*)bytes);
	int image = nvgCreateImageMem((NVGcontext*)ctx, imageFlags, bytes, dataSize);
	// Note: nvgCreateImageMem copies the data, so we can free it
	free(bytes);
	return image;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateImageRGBA
  (JNIEnv* env, jclass cls, jlong ctx, jint w, jint h, jint imageFlags, jbyteArray data) {
	(void)cls;
	jsize dataSize = (*env)->GetArrayLength(env, data);
	unsigned char* bytes = (unsigned char*)malloc(dataSize);
	if (bytes == NULL) return 0;
	
	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte*)bytes);
	int image = nvgCreateImageRGBA((NVGcontext*)ctx, w, h, imageFlags, bytes);
	free(bytes);
	return image;
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgUpdateImage
  (JNIEnv* env, jclass cls, jlong ctx, jint image, jbyteArray data) {
	(void)cls;
	jsize dataSize = (*env)->GetArrayLength(env, data);
	unsigned char* bytes = (unsigned char*)malloc(dataSize);
	if (bytes == NULL) return;
	
	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte*)bytes);
	nvgUpdateImage((NVGcontext*)ctx, image, bytes);
	free(bytes);
}

JNIEXPORT jintArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgImageSize
  (JNIEnv* env, jclass cls, jlong ctx, jint image) {
	(void)cls;
	int w, h;
	nvgImageSize((NVGcontext*)ctx, image, &w, &h);
	
	jintArray result = (*env)->NewIntArray(env, 2);
	if (result != NULL) {
		jint size[2] = {w, h};
		(*env)->SetIntArrayRegion(env, result, 0, 2, size);
	}
	return result;
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgDeleteImage
  (JNIEnv* env, jclass cls, jlong ctx, jint image) {
	(void)env; (void)cls;
	nvgDeleteImage((NVGcontext*)ctx, image);
}

// Font Memory Loading
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateFontMem
  (JNIEnv* env, jclass cls, jlong ctx, jstring name, jbyteArray data, jboolean freeData) {
	(void)cls;
	const char* cName = jstring_to_cstring(env, name);
	if (cName == NULL) return -1;
	
	jsize dataSize = (*env)->GetArrayLength(env, data);
	unsigned char* bytes = (unsigned char*)malloc(dataSize);
	if (bytes == NULL) {
		free((void*)cName);
		return -1;
	}
	
	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte*)bytes);
	int font = nvgCreateFontMem((NVGcontext*)ctx, cName, bytes, dataSize, freeData ? 1 : 0);
	free((void*)cName);
	return font;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateFontMemAtIndex
  (JNIEnv* env, jclass cls, jlong ctx, jstring name, jbyteArray data, jboolean freeData, jint fontIndex) {
	(void)cls;
	const char* cName = jstring_to_cstring(env, name);
	if (cName == NULL) return -1;
	
	jsize dataSize = (*env)->GetArrayLength(env, data);
	unsigned char* bytes = (unsigned char*)malloc(dataSize);
	if (bytes == NULL) {
		free((void*)cName);
		return -1;
	}
	
	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte*)bytes);
	int font = nvgCreateFontMemAtIndex((NVGcontext*)ctx, cName, bytes, dataSize, freeData ? 1 : 0, fontIndex);
	free((void*)cName);
	return font;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgAddFallbackFont
  (JNIEnv* env, jclass cls, jlong ctx, jstring baseFont, jstring fallbackFont) {
	(void)cls;
	const char* cBaseFont = jstring_to_cstring(env, baseFont);
	const char* cFallbackFont = jstring_to_cstring(env, fallbackFont);
	if (cBaseFont == NULL || cFallbackFont == NULL) {
		if (cBaseFont) free((void*)cBaseFont);
		if (cFallbackFont) free((void*)cFallbackFont);
		return 0;
	}
	
	int result = nvgAddFallbackFont((NVGcontext*)ctx, cBaseFont, cFallbackFont);
	free((void*)cBaseFont);
	free((void*)cFallbackFont);
	return result;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgAddFallbackFontId
  (JNIEnv* env, jclass cls, jlong ctx, jint baseFont, jint fallbackFont) {
	(void)env; (void)cls;
	return nvgAddFallbackFontId((NVGcontext*)ctx, baseFont, fallbackFont);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgResetFallbackFonts
  (JNIEnv* env, jclass cls, jlong ctx, jstring baseFont) {
	(void)cls;
	const char* cBaseFont = jstring_to_cstring(env, baseFont);
	if (cBaseFont == NULL) return;
	
	nvgResetFallbackFonts((NVGcontext*)ctx, cBaseFont);
	free((void*)cBaseFont);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgFontFaceId
  (JNIEnv* env, jclass cls, jlong ctx, jint font) {
	(void)env; (void)cls;
	nvgFontFaceId((NVGcontext*)ctx, font);
}

// Advanced Text Layout
JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextBoxBounds
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jfloat breakRowWidth, jstring text) {
	(void)cls;
	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return NULL;
	
	float bounds[4];
	nvgTextBoxBounds((NVGcontext*)ctx, x, y, breakRowWidth, cText, NULL, bounds);
	free((void*)cText);
	
	jfloatArray result = (*env)->NewFloatArray(env, 4);
	if (result != NULL) {
		(*env)->SetFloatArrayRegion(env, result, 0, 4, bounds);
	}
	return result;
}

// Miscellaneous
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgShapeAntiAlias
  (JNIEnv* env, jclass cls, jlong ctx, jboolean enabled) {
	(void)env; (void)cls;
	nvgShapeAntiAlias((NVGcontext*)ctx, enabled ? 1 : 0);
}

JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCurrentTransform
  (JNIEnv* env, jclass cls, jlong ctx) {
	(void)cls;
	float xform[6];
	nvgCurrentTransform((NVGcontext*)ctx, xform);

	jfloatArray result = (*env)->NewFloatArray(env, 6);
	if (result != NULL) {
		(*env)->SetFloatArrayRegion(env, result, 0, 6, xform);
	}
	return result;
}

// Color Utilities
JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgHSL
  (JNIEnv* env, jclass cls, jfloat h, jfloat s, jfloat l) {
	(void)cls;
	NVGcolor color = nvgHSL(h, s, l);

	jfloatArray result = (*env)->NewFloatArray(env, 4);
	if (result != NULL) {
		(*env)->SetFloatArrayRegion(env, result, 0, 4, &color.r);
	}
	return result;
}

JNIEXPORT jfloatArray JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgHSLA
  (JNIEnv* env, jclass cls, jfloat h, jfloat s, jfloat l, jint a) {
	(void)cls;
	NVGcolor color = nvgHSLA(h, s, l, (unsigned char)a);

	jfloatArray result = (*env)->NewFloatArray(env, 4);
	if (result != NULL) {
		(*env)->SetFloatArrayRegion(env, result, 0, 4, &color.r);
	}
	return result;
}

// Transform Utilities
JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformIdentity
  (JNIEnv* env, jclass cls, jfloatArray dst) {
	(void)cls;
	float xform[6];
	nvgTransformIdentity(xform);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformTranslate
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloat tx, jfloat ty) {
	(void)cls;
	float xform[6];
	nvgTransformTranslate(xform, tx, ty);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformScale
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloat sx, jfloat sy) {
	(void)cls;
	float xform[6];
	nvgTransformScale(xform, sx, sy);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformRotate
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloat angle) {
	(void)cls;
	float xform[6];
	nvgTransformRotate(xform, angle);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformSkewX
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloat angle) {
	(void)cls;
	float xform[6];
	nvgTransformSkewX(xform, angle);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformSkewY
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloat angle) {
	(void)cls;
	float xform[6];
	nvgTransformSkewY(xform, angle);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, xform);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformMultiply
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloatArray src) {
	(void)cls;
	float d[6], s[6];
	(*env)->GetFloatArrayRegion(env, dst, 0, 6, d);
	(*env)->GetFloatArrayRegion(env, src, 0, 6, s);
	nvgTransformMultiply(d, s);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, d);
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformPremultiply
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloatArray src) {
	(void)cls;
	float d[6], s[6];
	(*env)->GetFloatArrayRegion(env, dst, 0, 6, d);
	(*env)->GetFloatArrayRegion(env, src, 0, 6, s);
	nvgTransformPremultiply(d, s);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, d);
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformInverse
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloatArray src) {
	(void)cls;
	float d[6], s[6];
	(*env)->GetFloatArrayRegion(env, src, 0, 6, s);
	int result = nvgTransformInverse(d, s);
	(*env)->SetFloatArrayRegion(env, dst, 0, 6, d);
	return result;
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTransformPoint
  (JNIEnv* env, jclass cls, jfloatArray dst, jfloatArray xform, jfloat srcx, jfloat srcy) {
	(void)cls;
	float t[6];
	float dx, dy;
	(*env)->GetFloatArrayRegion(env, xform, 0, 6, t);
	nvgTransformPoint(&dx, &dy, t, srcx, srcy);
	float result[2] = {dx, dy};
	(*env)->SetFloatArrayRegion(env, dst, 0, 2, result);
}

// Additional Font Functions
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgCreateFontAtIndex
  (JNIEnv* env, jclass cls, jlong ctx, jstring name, jstring filename, jint fontIndex) {
	(void)cls;
	const char* cName = jstring_to_cstring(env, name);
	const char* cFilename = jstring_to_cstring(env, filename);
	if (cName == NULL || cFilename == NULL) {
		if (cName) free((void*)cName);
		if (cFilename) free((void*)cFilename);
		return -1;
	}

	int font = nvgCreateFontAtIndex((NVGcontext*)ctx, cName, cFilename, fontIndex);
	free((void*)cName);
	free((void*)cFilename);
	return font;
}

JNIEXPORT void JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgResetFallbackFontsId
  (JNIEnv* env, jclass cls, jlong ctx, jint baseFont) {
	(void)env; (void)cls;
	nvgResetFallbackFontsId((NVGcontext*)ctx, baseFont);
}

// Complex Text Layout
JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextGlyphPositions
  (JNIEnv* env, jclass cls, jlong ctx, jfloat x, jfloat y, jstring text, jfloatArray positions, jint maxPositions) {
	(void)cls;
	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return 0;

	NVGglyphPosition* glyphPos = (NVGglyphPosition*)malloc(sizeof(NVGglyphPosition) * maxPositions);
	if (glyphPos == NULL) {
		free((void*)cText);
		return 0;
	}

	int nglyphs = nvgTextGlyphPositions((NVGcontext*)ctx, x, y, cText, NULL, glyphPos, maxPositions);

	jsize posArrayLen = (*env)->GetArrayLength(env, positions);
	int copyCount = nglyphs < (posArrayLen / 4) ? nglyphs : (posArrayLen / 4);

	float* posData = (float*)malloc(sizeof(float) * copyCount * 4);
	if (posData != NULL) {
		for (int i = 0; i < copyCount; i++) {
			posData[i * 4 + 0] = glyphPos[i].x;
			posData[i * 4 + 1] = glyphPos[i].minx;
			posData[i * 4 + 2] = glyphPos[i].maxx;
			posData[i * 4 + 3] = 0.0f;
		}
		(*env)->SetFloatArrayRegion(env, positions, 0, copyCount * 4, posData);
		free(posData);
	}

	free(glyphPos);
	free((void*)cText);
	return nglyphs;
}

JNIEXPORT jint JNICALL Java_org_emmo_ai_nanovg_NanoVG_nvgTextBreakLines
  (JNIEnv* env, jclass cls, jlong ctx, jstring text, jfloat breakRowWidth, jfloatArray rows, jint maxRows) {
	(void)cls;
	const char* cText = jstring_to_cstring(env, text);
	if (cText == NULL) return 0;

	NVGtextRow* textRows = (NVGtextRow*)malloc(sizeof(NVGtextRow) * maxRows);
	if (textRows == NULL) {
		free((void*)cText);
		return 0;
	}

	int nrows = nvgTextBreakLines((NVGcontext*)ctx, cText, NULL, breakRowWidth, textRows, maxRows);

	jsize rowArrayLen = (*env)->GetArrayLength(env, rows);
	int copyCount = nrows < (rowArrayLen / 4) ? nrows : (rowArrayLen / 4);

	float* rowData = (float*)malloc(sizeof(float) * copyCount * 4);
	if (rowData != NULL) {
		for (int i = 0; i < copyCount; i++) {
			rowData[i * 4 + 0] = (float)(textRows[i].start - cText);
			rowData[i * 4 + 1] = (float)(textRows[i].end - cText);
			rowData[i * 4 + 2] = textRows[i].width;
			rowData[i * 4 + 3] = textRows[i].minx;
		}
		(*env)->SetFloatArrayRegion(env, rows, 0, copyCount * 4, rowData);
		free(rowData);
	}

	free(textRows);
	free((void*)cText);
	return nrows;
}
