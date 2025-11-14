#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

typedef enum {
	SUBPIXEL_UNKNOWN = 0,
	SUBPIXEL_NONE = 1,
	SUBPIXEL_HORIZONTAL_RGB = 2,
	SUBPIXEL_HORIZONTAL_BGR = 3,
	SUBPIXEL_VERTICAL_RGB = 4,
	SUBPIXEL_VERTICAL_BGR = 5
} SubpixelLayout;

typedef struct {
	char name[256];
	SubpixelLayout subpixel;
	int width;
	int height;
	int refreshRate;
	int rotation;
} DisplayInfo;

static DisplayInfo display_info = {0};

static void output_geometry(void *data, struct wl_output *wl_output,
                           int32_t x, int32_t y,
                           int32_t physical_width, int32_t physical_height,
                           int32_t subpixel,
                           const char *make, const char *model,
                           int32_t transform) {
	(void)data;
	(void)wl_output;
	(void)x;
	(void)y;
	(void)physical_width;
	(void)physical_height;
	(void)transform;

	printf("Display: %s %s\n", make, model);
	snprintf(display_info.name, sizeof(display_info.name), "%s %s", make, model);

	// Map Wayland subpixel values to our enum
	switch (subpixel) {
		case WL_OUTPUT_SUBPIXEL_UNKNOWN:
			display_info.subpixel = SUBPIXEL_UNKNOWN;
			printf("Subpixel: UNKNOWN\n");
			break;
		case WL_OUTPUT_SUBPIXEL_NONE:
			display_info.subpixel = SUBPIXEL_NONE;
			printf("Subpixel: NONE\n");
			break;
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
			display_info.subpixel = SUBPIXEL_HORIZONTAL_RGB;
			printf("Subpixel: HORIZONTAL_RGB\n");
			break;
		case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
			display_info.subpixel = SUBPIXEL_HORIZONTAL_BGR;
			printf("Subpixel: HORIZONTAL_BGR\n");
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
			display_info.subpixel = SUBPIXEL_VERTICAL_RGB;
			printf("Subpixel: VERTICAL_RGB\n");
			break;
		case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
			display_info.subpixel = SUBPIXEL_VERTICAL_BGR;
			printf("Subpixel: VERTICAL_BGR\n");
			break;
		default:
			display_info.subpixel = SUBPIXEL_UNKNOWN;
			printf("Subpixel: UNKNOWN (%d)\n", subpixel);
			break;
	}

	display_info.rotation = transform;
}

static void output_mode(void *data, struct wl_output *wl_output,
                       uint32_t flags, int32_t width, int32_t height,
                       int32_t refresh) {
	(void)data;
	(void)wl_output;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		display_info.width = width;
		display_info.height = height;
		display_info.refreshRate = refresh / 1000; // Convert mHz to Hz
		printf("Resolution: %dx%d @ %d Hz\n", width, height, display_info.refreshRate);
	}
}

static void output_done(void *data, struct wl_output *wl_output) {
	(void)data;
	(void)wl_output;
}

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
	(void)data;
	(void)wl_output;
	printf("Scale factor: %d\n", factor);
}

static const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version) {
	(void)data;
	(void)version;

	printf("Found interface: %s (name=%u, version=%u)\n", interface, name, version);

	if (strcmp(interface, "wl_output") == 0) {
		printf("  -> Binding to wl_output\n");
		struct wl_output *output = wl_registry_bind(registry, name,
		                                            &wl_output_interface, 2);
		wl_output_add_listener(output, &output_listener, NULL);
	}
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name) {
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

int main(void) {
	printf("=== Wayland Display Query ===\n");
	printf("Querying display capabilities via Wayland...\n\n");

	struct wl_display *display = wl_display_connect(NULL);
	if (!display) {
		printf("ERROR: Failed to connect to Wayland display\n");
		printf("Are you running under Wayland?\n");
		return 1;
	}

	printf("Connected to Wayland display\n");

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	// First roundtrip to get registry events
	wl_display_roundtrip(display);

	// Second roundtrip to get output events
	wl_display_roundtrip(display);

	printf("\n=== Display Capabilities ===\n");
	printf("Name: %s\n", display_info.name);
	printf("Resolution: %dx%d\n", display_info.width, display_info.height);
	printf("Refresh Rate: %d Hz\n", display_info.refreshRate);

	printf("\n=== Recommendation for LCD Subpixel Rendering ===\n");
	switch (display_info.subpixel) {
		case SUBPIXEL_HORIZONTAL_RGB:
			printf("✓ Use RGB subpixel rendering (Canvas 3)\n");
			printf("  Your display has RGB stripe layout\n");
			break;
		case SUBPIXEL_HORIZONTAL_BGR:
			printf("✓ Use BGR subpixel rendering (Canvas 4)\n");
			printf("  Your display has BGR stripe layout\n");
			break;
		case SUBPIXEL_VERTICAL_RGB:
		case SUBPIXEL_VERTICAL_BGR:
			printf("⚠ Vertical subpixel layout detected\n");
			printf("  LCD subpixel rendering typically works best with horizontal layouts\n");
			break;
		case SUBPIXEL_NONE:
		case SUBPIXEL_UNKNOWN:
		default:
			printf("⚠ No subpixel information available\n");
			printf("  Fall back to grayscale antialiasing (Canvas 2)\n");
			break;
	}

	wl_registry_destroy(registry);
	wl_display_disconnect(display);

	return 0;
}
