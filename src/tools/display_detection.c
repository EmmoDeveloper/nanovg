#include "display_detection.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __linux__
#ifdef HAVE_WAYLAND
#include <wayland-client.h>
#endif

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#endif

#ifdef HAVE_WAYLAND
static DisplayDetectionInfo wayland_display = {0};

static void wayland_output_geometry(void *data, struct wl_output *wl_output,
                                     int32_t x, int32_t y,
                                     int32_t physical_width, int32_t physical_height,
                                     int32_t subpixel,
                                     const char *make, const char *model,
                                     int32_t transform) {
	(void)data; (void)wl_output; (void)x; (void)y; (void)transform;

	snprintf(wayland_display.name, sizeof(wayland_display.name), "%s %s", make, model);
	wayland_display.physicalWidthMM = physical_width;
	wayland_display.physicalHeightMM = physical_height;

	switch (subpixel) {
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_RGB;
			break;
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_BGR;
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_VERTICAL_RGB;
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_VERTICAL_BGR;
			break;
		case WL_OUTPUT_SUBPIXEL_NONE:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_NONE;
			break;
		default:
			wayland_display.subpixel = DISPLAY_SUBPIXEL_UNKNOWN;
			break;
	}
}

static void wayland_output_mode(void *data, struct wl_output *wl_output,
                                 uint32_t flags, int32_t width, int32_t height,
                                 int32_t refresh) {
	(void)data; (void)wl_output;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		wayland_display.width = width;
		wayland_display.height = height;
		wayland_display.refreshRate = refresh / 1000;
	}
}

static void wayland_output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
	(void)data; (void)wl_output;
	wayland_display.scale = (float)factor;
}

static void wayland_output_done(void *data, struct wl_output *wl_output) {
	(void)data; (void)wl_output;
}

static const struct wl_output_listener wayland_output_listener = {
	.geometry = wayland_output_geometry,
	.mode = wayland_output_mode,
	.done = wayland_output_done,
	.scale = wayland_output_scale,
};

static void wayland_registry_global(void *data, struct wl_registry *registry,
                                     uint32_t name, const char *interface,
                                     uint32_t version) {
	(void)data; (void)version;

	if (strcmp(interface, "wl_output") == 0) {
		struct wl_output *output = wl_registry_bind(registry, name,
		                                            &wl_output_interface, 2);
		wl_output_add_listener(output, &wayland_output_listener, NULL);
	}
}

static void wayland_registry_global_remove(void *data, struct wl_registry *registry,
                                            uint32_t name) {
	(void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener wayland_registry_listener = {
	.global = wayland_registry_global,
	.global_remove = wayland_registry_global_remove,
};

static int detect_wayland(DisplayDetectionInfo* info) {
	memset(&wayland_display, 0, sizeof(wayland_display));

	struct wl_display *display = wl_display_connect(NULL);
	if (!display) return 0;

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &wayland_registry_listener, NULL);

	wl_display_roundtrip(display);
	wl_display_roundtrip(display);

	*info = wayland_display;

	wl_registry_destroy(registry);
	wl_display_disconnect(display);

	return (wayland_display.width > 0) ? 1 : 0;
}
#endif

#ifdef HAVE_X11
static int detect_x11(DisplayDetectionInfo* info) {
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) return 0;

	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	XRRScreenResources *resources = XRRGetScreenResources(dpy, root);
	if (!resources || resources->noutput == 0) {
		if (resources) XRRFreeScreenResources(resources);
		XCloseDisplay(dpy);
		return 0;
	}

	XRROutputInfo *output = XRRGetOutputInfo(dpy, resources, resources->outputs[0]);
	if (!output || output->connection != RR_Connected) {
		if (output) XRRFreeOutputInfo(output);
		XRRFreeScreenResources(resources);
		XCloseDisplay(dpy);
		return 0;
	}

	strncpy(info->name, output->name, sizeof(info->name) - 1);

	if (output->crtc) {
		XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, resources, output->crtc);
		if (crtc) {
			info->width = crtc->width;
			info->height = crtc->height;

			for (int i = 0; i < resources->nmode; i++) {
				if (resources->modes[i].id == crtc->mode) {
					if (resources->modes[i].hTotal && resources->modes[i].vTotal) {
						info->refreshRate = (int)((double)resources->modes[i].dotClock /
						                         (resources->modes[i].hTotal * resources->modes[i].vTotal));
					}
					break;
				}
			}
			XRRFreeCrtcInfo(crtc);
		}
	}

	if (output->subpixel_order == 1) info->subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_RGB;
	else if (output->subpixel_order == 2) info->subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_BGR;
	else if (output->subpixel_order == 3) info->subpixel = DISPLAY_SUBPIXEL_VERTICAL_RGB;
	else if (output->subpixel_order == 4) info->subpixel = DISPLAY_SUBPIXEL_VERTICAL_BGR;
	else if (output->subpixel_order == 5) info->subpixel = DISPLAY_SUBPIXEL_NONE;
	else info->subpixel = DISPLAY_SUBPIXEL_UNKNOWN;

	info->physicalWidthMM = output->mm_width;
	info->physicalHeightMM = output->mm_height;

	XRRFreeOutputInfo(output);
	XRRFreeScreenResources(resources);
	XCloseDisplay(dpy);

	return (info->width > 0) ? 1 : 0;
}
#endif

#ifdef HAVE_FONTCONFIG
static int detect_fontconfig(DisplayDetectionInfo* info) {
	if (!FcInit()) return 0;

	FcPattern* pattern = FcPatternCreate();
	FcConfigSubstitute(NULL, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	FcResult result;
	FcPattern* match = FcFontMatch(NULL, pattern, &result);

	if (match) {
		int rgba = FC_RGBA_UNKNOWN;
		if (FcPatternGetInteger(match, FC_RGBA, 0, &rgba) == FcResultMatch) {
			switch (rgba) {
				case FC_RGBA_RGB: info->subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_RGB; break;
				case FC_RGBA_BGR: info->subpixel = DISPLAY_SUBPIXEL_HORIZONTAL_BGR; break;
				case FC_RGBA_VRGB: info->subpixel = DISPLAY_SUBPIXEL_VERTICAL_RGB; break;
				case FC_RGBA_VBGR: info->subpixel = DISPLAY_SUBPIXEL_VERTICAL_BGR; break;
				case FC_RGBA_NONE: info->subpixel = DISPLAY_SUBPIXEL_NONE; break;
				default: info->subpixel = DISPLAY_SUBPIXEL_UNKNOWN; break;
			}
		}
		FcPatternDestroy(match);
	}

	FcPatternDestroy(pattern);
	FcFini();

	return (info->subpixel != DISPLAY_SUBPIXEL_UNKNOWN) ? 1 : 0;
}
#endif

int detect_display_info(DisplayDetectionInfo* info) {
	memset(info, 0, sizeof(*info));

#ifdef HAVE_WAYLAND
	if (detect_wayland(info)) {
		if (info->subpixel != DISPLAY_SUBPIXEL_UNKNOWN) {
			return 1;
		}
	}
#endif

#ifdef HAVE_X11
	DisplayDetectionInfo x11_info = {0};
	if (detect_x11(&x11_info)) {
		if (info->width == 0) {
			*info = x11_info;
		} else if (x11_info.subpixel != DISPLAY_SUBPIXEL_UNKNOWN) {
			info->subpixel = x11_info.subpixel;
		}
		if (info->subpixel != DISPLAY_SUBPIXEL_UNKNOWN) {
			return 1;
		}
	}
#endif

#ifdef HAVE_FONTCONFIG
	if (info->subpixel == DISPLAY_SUBPIXEL_UNKNOWN) {
		DisplayDetectionInfo fc_info = {0};
		if (detect_fontconfig(&fc_info)) {
			info->subpixel = fc_info.subpixel;
		}
	}
#endif

	return (info->width > 0 || info->subpixel != DISPLAY_SUBPIXEL_UNKNOWN) ? 1 : 0;
}

int display_subpixel_to_nvg(DisplaySubpixelLayout layout) {
	switch (layout) {
		case DISPLAY_SUBPIXEL_NONE: return 0;
		case DISPLAY_SUBPIXEL_HORIZONTAL_RGB: return 1;
		case DISPLAY_SUBPIXEL_HORIZONTAL_BGR: return 2;
		case DISPLAY_SUBPIXEL_VERTICAL_RGB: return 3;
		case DISPLAY_SUBPIXEL_VERTICAL_BGR: return 4;
		default: return 0;
	}
}

const char* display_subpixel_name(DisplaySubpixelLayout layout) {
	switch (layout) {
		case DISPLAY_SUBPIXEL_NONE: return "None";
		case DISPLAY_SUBPIXEL_HORIZONTAL_RGB: return "RGB (Horizontal)";
		case DISPLAY_SUBPIXEL_HORIZONTAL_BGR: return "BGR (Horizontal)";
		case DISPLAY_SUBPIXEL_VERTICAL_RGB: return "RGB (Vertical)";
		case DISPLAY_SUBPIXEL_VERTICAL_BGR: return "BGR (Vertical)";
		default: return "Unknown";
	}
}
