#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	SUBPIXEL_UNKNOWN = 0,
	SUBPIXEL_NONE = 1,
	SUBPIXEL_HORIZONTAL_RGB = 2,
	SUBPIXEL_HORIZONTAL_BGR = 3,
	SUBPIXEL_VERTICAL_RGB = 4,
	SUBPIXEL_VERTICAL_BGR = 5
} SubpixelLayout;

typedef enum {
	COLOR_SPACE_SRGB = 0,
	COLOR_SPACE_DISPLAY_P3 = 1,
	COLOR_SPACE_HDR10 = 2
} ColorSpace;

typedef struct {
	char name[256];
	SubpixelLayout subpixel;
	ColorSpace colorSpace;
	int width;
	int height;
	int refreshRate;
	int dpi;
	int bitDepth;
	int rotation;
	int hdrSupported;
} DisplayInfo;

#ifdef __linux__
#include <wayland-client.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <fontconfig/fontconfig.h>

// Wayland detection
static DisplayInfo wayland_display = {0};

static void wayland_output_geometry(void *data, struct wl_output *wl_output,
                                     int32_t x, int32_t y,
                                     int32_t physical_width, int32_t physical_height,
                                     int32_t subpixel,
                                     const char *make, const char *model,
                                     int32_t transform) {
	(void)data; (void)wl_output; (void)x; (void)y;
	(void)physical_width; (void)physical_height; (void)transform;

	snprintf(wayland_display.name, sizeof(wayland_display.name), "%s %s", make, model);

	switch (subpixel) {
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
			wayland_display.subpixel = SUBPIXEL_HORIZONTAL_RGB;
			break;
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
			wayland_display.subpixel = SUBPIXEL_HORIZONTAL_BGR;
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
			wayland_display.subpixel = SUBPIXEL_VERTICAL_RGB;
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
			wayland_display.subpixel = SUBPIXEL_VERTICAL_BGR;
			break;
		case WL_OUTPUT_SUBPIXEL_NONE:
			wayland_display.subpixel = SUBPIXEL_NONE;
			break;
		default:
			wayland_display.subpixel = SUBPIXEL_UNKNOWN;
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

static void wayland_output_done(void *data, struct wl_output *wl_output) {
	(void)data; (void)wl_output;
}

static void wayland_output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
	(void)data; (void)wl_output; (void)factor;
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

int get_displays_wayland(DisplayInfo* infos, int max_count) {
	if (max_count < 1) return 0;

	struct wl_display *display = wl_display_connect(NULL);
	if (!display) return 0;

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &wayland_registry_listener, NULL);

	wl_display_roundtrip(display);
	wl_display_roundtrip(display);

	infos[0] = wayland_display;

	wl_registry_destroy(registry);
	wl_display_disconnect(display);

	return (wayland_display.width > 0) ? 1 : 0;
}

int get_displays_linux_x11(DisplayInfo* infos, int max_count) {
	if (max_count < 1) return 0;

	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) return 0;

	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);

	XRRScreenResources *resources = XRRGetScreenResources(dpy, root);
	if (!resources) {
		XCloseDisplay(dpy);
		return 0;
	}

	int count = 0;
	for (int i = 0; i < resources->noutput && count < max_count; i++) {
		XRROutputInfo *output_info = XRRGetOutputInfo(dpy, resources, resources->outputs[i]);
		if (!output_info || output_info->connection != RR_Connected) {
			if (output_info) XRRFreeOutputInfo(output_info);
			continue;
		}

		DisplayInfo *info = &infos[count];
		snprintf(info->name, sizeof(info->name), "%s", output_info->name);

		// Get current mode
		if (output_info->crtc) {
			XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, resources, output_info->crtc);
			if (crtc_info) {
				info->width = crtc_info->width;
				info->height = crtc_info->height;
				info->rotation = crtc_info->rotation;

				// Find refresh rate
				for (int j = 0; j < resources->nmode; j++) {
					if (resources->modes[j].id == crtc_info->mode) {
						if (resources->modes[j].hTotal && resources->modes[j].vTotal) {
							info->refreshRate = (int)((double)resources->modes[j].dotClock /
							                         (resources->modes[j].hTotal * resources->modes[j].vTotal));
						}
						break;
					}
				}

				XRRFreeCrtcInfo(crtc_info);
			}
		}

		// List all available properties
		int nprop;
		Atom *props = XRRListOutputProperties(dpy, resources->outputs[i], &nprop);
		printf("  Available properties for %s:\n", output_info->name);
		for (int p = 0; p < nprop; p++) {
			char *prop_name = XGetAtomName(dpy, props[p]);
			printf("    - %s\n", prop_name);
			XFree(prop_name);
		}
		XFree(props);

		// Get subpixel layout
		Atom subpixel_atom = XInternAtom(dpy, "subpixel", False);
		if (subpixel_atom != None) {
			Atom actual_type;
			int actual_format;
			unsigned long nitems, bytes_after;
			unsigned char *prop = NULL;

			if (XRRGetOutputProperty(dpy, resources->outputs[i], subpixel_atom,
			                        0, 4, False, False, AnyPropertyType,
			                        &actual_type, &actual_format,
			                        &nitems, &bytes_after, &prop) == Success && prop) {
				if (nitems > 0) {
					int subpixel_order = *(int*)prop;
					printf("  Subpixel property value: %d\n", subpixel_order);
					switch (subpixel_order) {
						case 0: info->subpixel = SUBPIXEL_UNKNOWN; break;
						case 1: info->subpixel = SUBPIXEL_HORIZONTAL_RGB; break;
						case 2: info->subpixel = SUBPIXEL_HORIZONTAL_BGR; break;
						case 3: info->subpixel = SUBPIXEL_VERTICAL_RGB; break;
						case 4: info->subpixel = SUBPIXEL_VERTICAL_BGR; break;
						case 5: info->subpixel = SUBPIXEL_NONE; break;
						default: info->subpixel = SUBPIXEL_UNKNOWN; break;
					}
				}
				XFree(prop);
			} else {
				printf("  Subpixel property not found or failed to read\n");
			}
		}

		XRRFreeOutputInfo(output_info);
		count++;
	}

	XRRFreeScreenResources(resources);
	XCloseDisplay(dpy);

	return count;
}

int get_subpixel_from_fontconfig(SubpixelLayout* layout) {
	if (!FcInit()) {
		return 0;
	}

	FcPattern *pattern = FcPatternCreate();
	FcConfigSubstitute(NULL, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	FcResult result;
	FcPattern *match = FcFontMatch(NULL, pattern, &result);

	if (match) {
		int rgba;
		if (FcPatternGetInteger(match, FC_RGBA, 0, &rgba) == FcResultMatch) {
			printf("Fontconfig RGBA value: %d\n", rgba);
			switch (rgba) {
				case FC_RGBA_RGB:
					*layout = SUBPIXEL_HORIZONTAL_RGB;
					printf("  -> Interpreted as HORIZONTAL_RGB\n");
					break;
				case FC_RGBA_BGR:
					*layout = SUBPIXEL_HORIZONTAL_BGR;
					printf("  -> Interpreted as HORIZONTAL_BGR\n");
					break;
				case FC_RGBA_VRGB:
					*layout = SUBPIXEL_VERTICAL_RGB;
					printf("  -> Interpreted as VERTICAL_RGB\n");
					break;
				case FC_RGBA_VBGR:
					*layout = SUBPIXEL_VERTICAL_BGR;
					printf("  -> Interpreted as VERTICAL_BGR\n");
					break;
				case FC_RGBA_NONE:
					*layout = SUBPIXEL_NONE;
					printf("  -> Interpreted as NONE\n");
					break;
				default:
					*layout = SUBPIXEL_UNKNOWN;
					printf("  -> Interpreted as UNKNOWN\n");
					break;
			}
			FcPatternDestroy(match);
			FcPatternDestroy(pattern);
			FcFini();
			return 1;
		}
		FcPatternDestroy(match);
	}

	FcPatternDestroy(pattern);
	FcFini();
	return 0;
}
#endif

int main() {
	DisplayInfo infos[16];
	int count = 0;

	printf("=== Unified Display Query ===\n\n");

#ifdef __linux__
	// Try Wayland first
	if (getenv("WAYLAND_DISPLAY")) {
		printf("Detected Wayland environment\n");
		printf("Attempting Wayland query...\n");
		count = get_displays_wayland(infos, 16);
		printf("Wayland returned %d display(s)\n", count);

		if (count > 0 && infos[0].subpixel == SUBPIXEL_UNKNOWN) {
			printf("Wayland returned UNKNOWN subpixel layout\n");
			printf("Falling back to X11/XRandR...\n");
			count = get_displays_linux_x11(infos, 16);
			printf("X11 returned %d display(s)\n", count);
		}
	} else {
		printf("Detected X11 environment\n");
		count = get_displays_linux_x11(infos, 16);
	}

	// Final fallback: Try fontconfig
	if (count > 0 && infos[0].subpixel == SUBPIXEL_UNKNOWN) {
		printf("\nBoth Wayland and X11 returned UNKNOWN\n");
		printf("Falling back to fontconfig...\n");
		if (get_subpixel_from_fontconfig(&infos[0].subpixel)) {
			printf("Successfully retrieved subpixel layout from fontconfig\n");
		} else {
			printf("Fontconfig query failed\n");
		}
	}
#else
	printf("Platform not supported.\n");
#endif

	if (count == 0) {
		printf("ERROR: No displays detected!\n");
		return 1;
	}

	printf("\n=== Display Information ===\n");
	for (int i = 0; i < count; i++) {
		printf("\nDisplay %d: %s\n", i, infos[i].name);
		printf("  Resolution: %dx%d\n", infos[i].width, infos[i].height);
		printf("  Refresh rate: %d Hz\n", infos[i].refreshRate);
		printf("  Rotation: %d degrees\n", infos[i].rotation);

		printf("  Subpixel layout: ");
		switch (infos[i].subpixel) {
			case SUBPIXEL_HORIZONTAL_RGB:
				printf("HORIZONTAL_RGB ✓\n");
				break;
			case SUBPIXEL_HORIZONTAL_BGR:
				printf("HORIZONTAL_BGR ✓\n");
				break;
			case SUBPIXEL_VERTICAL_RGB:
				printf("VERTICAL_RGB\n");
				break;
			case SUBPIXEL_VERTICAL_BGR:
				printf("VERTICAL_BGR\n");
				break;
			case SUBPIXEL_NONE:
				printf("NONE\n");
				break;
			default:
				printf("UNKNOWN\n");
				break;
		}
	}

	printf("\n=== Recommendation ===\n");
	if (infos[0].subpixel == SUBPIXEL_HORIZONTAL_RGB) {
		printf("✓ Use RGB subpixel rendering (Canvas 3)\n");
	} else if (infos[0].subpixel == SUBPIXEL_HORIZONTAL_BGR) {
		printf("✓ Use BGR subpixel rendering (Canvas 4)\n");
	} else {
		printf("⚠ Use grayscale antialiasing (Canvas 2)\n");
	}

	return 0;
}
