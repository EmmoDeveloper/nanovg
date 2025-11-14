#ifndef DISPLAY_DETECTION_H
#define DISPLAY_DETECTION_H

typedef enum {
	DISPLAY_SUBPIXEL_UNKNOWN = 0,
	DISPLAY_SUBPIXEL_NONE = 1,
	DISPLAY_SUBPIXEL_HORIZONTAL_RGB = 2,
	DISPLAY_SUBPIXEL_HORIZONTAL_BGR = 3,
	DISPLAY_SUBPIXEL_VERTICAL_RGB = 4,
	DISPLAY_SUBPIXEL_VERTICAL_BGR = 5
} DisplaySubpixelLayout;

typedef struct {
	char name[256];
	DisplaySubpixelLayout subpixel;
	int width;
	int height;
	int refreshRate;
	int physicalWidthMM;
	int physicalHeightMM;
	float scale;
} DisplayDetectionInfo;

int detect_display_info(DisplayDetectionInfo* info);
int display_subpixel_to_nvg(DisplaySubpixelLayout layout);
const char* display_subpixel_name(DisplaySubpixelLayout layout);

#endif
