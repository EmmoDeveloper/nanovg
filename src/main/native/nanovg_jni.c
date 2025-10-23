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
