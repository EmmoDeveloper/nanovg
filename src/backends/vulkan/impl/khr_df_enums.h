#ifndef KHR_DF_ENUMS_H
#define KHR_DF_ENUMS_H

// Khronos Data Format Specification enumerations
// Based on official Khronos Data Format Specification v1.4
// Copyright 2014-2025 The Khronos Group Inc.
// https://registry.khronos.org/DataFormat/

#include <stdint.h>

// Transfer function (OETF/EOTF)
typedef enum khr_df_transfer_e {
	KHR_DF_TRANSFER_UNSPECIFIED	= 0,	// No transfer function defined
	KHR_DF_TRANSFER_LINEAR		= 1,	// Linear transfer function
	KHR_DF_TRANSFER_SRGB		= 2,	// sRGB transfer function ~2.4 gamma
	KHR_DF_TRANSFER_ITU		= 3,	// ITU (BT.709/601) ~1/.45 gamma
	KHR_DF_TRANSFER_SMTPE170M	= 3,	// Synonym for ITU
	KHR_DF_TRANSFER_NTSC		= 4,	// Original NTSC 2.2 gamma
	KHR_DF_TRANSFER_SLOG		= 5,	// Sony S-log
	KHR_DF_TRANSFER_SLOG2		= 6,	// Sony S-log 2
	KHR_DF_TRANSFER_BT1886		= 7,	// ITU BT.1886 EOTF
	KHR_DF_TRANSFER_HLG_OETF	= 8,	// ITU BT.2100 HLG OETF
	KHR_DF_TRANSFER_HLG_EOTF	= 9,	// ITU BT.2100 HLG EOTF
	KHR_DF_TRANSFER_PQ_EOTF		= 10,	// SMPTE ST.2084 PQ EOTF (HDR10)
	KHR_DF_TRANSFER_PQ_OETF		= 11,	// SMPTE ST.2084 PQ OETF
	KHR_DF_TRANSFER_DCIP3		= 12,	// DCI P3 transfer function ~2.6 gamma
	KHR_DF_TRANSFER_PAL_OETF	= 13,	// PAL OETF
	KHR_DF_TRANSFER_PAL625_EOTF	= 14,	// PAL 625-line EOTF
	KHR_DF_TRANSFER_ST240		= 15,	// SMPTE ST 240
	KHR_DF_TRANSFER_ACESCC		= 16,	// ACEScc logarithmic
	KHR_DF_TRANSFER_ACESCCT		= 17,	// ACEScct logarithmic
	KHR_DF_TRANSFER_ADOBERGB	= 18,	// Adobe RGB (1998) ~2.19921875 gamma
	KHR_DF_TRANSFER_MAX		= 0xFF
} khr_df_transfer_e;

// Color primaries
typedef enum khr_df_primaries_e {
	KHR_DF_PRIMARIES_UNSPECIFIED	= 0,	// No primaries defined
	KHR_DF_PRIMARIES_BT709		= 1,	// ITU-R BT.709 / sRGB
	KHR_DF_PRIMARIES_SRGB		= 1,	// Synonym for BT709
	KHR_DF_PRIMARIES_BT601_EBU	= 2,	// ITU-R BT.601 625-line (EBU)
	KHR_DF_PRIMARIES_BT601_SMPTE	= 3,	// ITU-R BT.601 525-line (SMPTE)
	KHR_DF_PRIMARIES_BT2020		= 4,	// ITU-R BT.2020 (UHD)
	KHR_DF_PRIMARIES_CIEXYZ		= 5,	// CIE 1931 XYZ
	KHR_DF_PRIMARIES_ACES		= 6,	// ACES primaries
	KHR_DF_PRIMARIES_ACESCC		= 7,	// ACEScc primaries
	KHR_DF_PRIMARIES_NTSC1953	= 8,	// NTSC 1953
	KHR_DF_PRIMARIES_PAL525		= 9,	// PAL 525-line
	KHR_DF_PRIMARIES_DISPLAYP3	= 10,	// Display P3
	KHR_DF_PRIMARIES_ADOBERGB	= 11,	// Adobe RGB (1998)
	KHR_DF_PRIMARIES_MAX		= 0xFF
} khr_df_primaries_e;

// Color model
typedef enum khr_df_model_e {
	KHR_DF_MODEL_UNSPECIFIED	= 0,
	KHR_DF_MODEL_RGBSDA		= 1,	// RGB with optional Alpha
	KHR_DF_MODEL_YUVSDA		= 2,	// YUV with optional Alpha
	KHR_DF_MODEL_YIQSDA		= 3,	// YIQ with optional Alpha
	KHR_DF_MODEL_LABSDA		= 4,	// CIE Lab with optional Alpha
	KHR_DF_MODEL_CMYKA		= 5,	// CMYK with optional Alpha
	KHR_DF_MODEL_XYZW		= 6,	// Coordinates (x, y, z, w)
	KHR_DF_MODEL_HSVA_ANG		= 7,	// HSV with hue in degrees
	KHR_DF_MODEL_HSLA_ANG		= 8,	// HSL with hue in degrees
	KHR_DF_MODEL_HSVA_HEX		= 9,	// HSV with hue in hexadecimal
	KHR_DF_MODEL_HSLA_HEX		= 10,	// HSL with hue in hexadecimal
	KHR_DF_MODEL_YCGCOA		= 11,	// YCgCo with optional Alpha
	KHR_DF_MODEL_YCCBCCRC		= 12,	// ITU BT.2020 constant luminance
	KHR_DF_MODEL_ICTCP		= 13,	// ITU BT.2100 ICtCp
	KHR_DF_MODEL_CIEXYZ		= 14,	// CIE XYZ
	KHR_DF_MODEL_CIEXYY		= 15,	// CIE xyY
	KHR_DF_MODEL_MAX		= 0xFF
} khr_df_model_e;

#endif // KHR_DF_ENUMS_H
