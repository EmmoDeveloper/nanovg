#include "nvg_font.h"
#include "nvg_font_internal.h"
#include <stdlib.h>
#include <string.h>

// Variable fonts

int nvgFont__SetVarDesignCoords(NVGFontSystem* fs, int fontId, const float* coords, unsigned int num_coords) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !coords) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_Fixed* ft_coords = (FT_Fixed*)malloc(sizeof(FT_Fixed) * num_coords);
	if (!ft_coords) return 0;

	for (unsigned int i = 0; i < num_coords; i++) {
		ft_coords[i] = (FT_Fixed)(coords[i] * 65536.0f);
	}

	FT_Error err = FT_Set_Var_Design_Coordinates(face, num_coords, ft_coords);
	free(ft_coords);

	// Update HarfBuzz font
	if (fs->fonts[fontId].hb_font) {
		hb_font_destroy(fs->fonts[fontId].hb_font);
		fs->fonts[fontId].hb_font = hb_ft_font_create(face, NULL);
	}

	return err == 0 ? 1 : 0;
}

int nvgFont__GetVarDesignCoords(NVGFontSystem* fs, int fontId, float* coords, unsigned int num_coords) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !coords) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_Fixed* ft_coords = (FT_Fixed*)malloc(sizeof(FT_Fixed) * num_coords);
	if (!ft_coords) return 0;

	FT_Error err = FT_Get_Var_Design_Coordinates(face, num_coords, ft_coords);
	if (err == 0) {
		for (unsigned int i = 0; i < num_coords; i++) {
			coords[i] = (float)ft_coords[i] / 65536.0f;
		}
	}

	free(ft_coords);
	return err == 0 ? 1 : 0;
}

int nvgFont__GetNamedInstanceCount(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_MM_Var* mm_var = NULL;
	if (FT_Get_MM_Var(face, &mm_var)) return 0;

	unsigned int count = mm_var->num_namedstyles;
	FT_Done_MM_Var(face->glyph->library, mm_var);

	return (int)count;
}

int nvgFont__SetNamedInstance(NVGFontSystem* fs, int fontId, unsigned int instance_index) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_Error err = FT_Set_Named_Instance(face, instance_index);

	// Update HarfBuzz font
	if (err == 0 && fs->fonts[fontId].hb_font) {
		hb_font_destroy(fs->fonts[fontId].hb_font);
		fs->fonts[fontId].hb_font = hb_ft_font_create(face, NULL);
	}

	return err == 0 ? 1 : 0;
}

// Font information

const char* nvgFont__GetFamilyName(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return "";
	FT_Face face = fs->fonts[fontId].face;
	return face->family_name ? face->family_name : "";
}

const char* nvgFont__GetStyleName(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return "";
	FT_Face face = fs->fonts[fontId].face;
	return face->style_name ? face->style_name : "";
}

int nvgFont__IsVariable(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;
	FT_Face face = fs->fonts[fontId].face;
	return FT_HAS_MULTIPLE_MASTERS(face) ? 1 : 0;
}

int nvgFont__IsScalable(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;
	FT_Face face = fs->fonts[fontId].face;
	return FT_IS_SCALABLE(face) ? 1 : 0;
}

int nvgFont__IsFixedWidth(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;
	FT_Face face = fs->fonts[fontId].face;
	return FT_IS_FIXED_WIDTH(face) ? 1 : 0;
}

int nvgFont__GetVarAxisCount(NVGFontSystem* fs, int fontId) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_MM_Var* mm_var = NULL;
	if (FT_Get_MM_Var(face, &mm_var)) return 0;

	unsigned int count = mm_var->num_axis;
	FT_Done_MM_Var(face->glyph->library, mm_var);

	return (int)count;
}

int nvgFont__GetVarAxis(NVGFontSystem* fs, int fontId, unsigned int axis_index, NVGVarAxis* axis) {
	if (!fs || fontId < 0 || fontId >= fs->nfonts || !axis) return 0;

	FT_Face face = fs->fonts[fontId].face;
	if (!FT_HAS_MULTIPLE_MASTERS(face)) return 0;

	FT_MM_Var* mm_var = NULL;
	if (FT_Get_MM_Var(face, &mm_var)) return 0;

	if (axis_index >= mm_var->num_axis) {
		FT_Done_MM_Var(face->glyph->library, mm_var);
		return 0;
	}

	FT_Var_Axis* ft_axis = &mm_var->axis[axis_index];

	// Convert tag to string
	unsigned int tag = ft_axis->tag;
	axis->tag[0] = (char)((tag >> 24) & 0xFF);
	axis->tag[1] = (char)((tag >> 16) & 0xFF);
	axis->tag[2] = (char)((tag >> 8) & 0xFF);
	axis->tag[3] = (char)(tag & 0xFF);
	axis->tag[4] = '\0';

	axis->minValue = (float)ft_axis->minimum / 65536.0f;
	axis->defaultValue = (float)ft_axis->def / 65536.0f;
	axis->maxValue = (float)ft_axis->maximum / 65536.0f;

	strncpy(axis->name, ft_axis->name, sizeof(axis->name) - 1);
	axis->name[sizeof(axis->name) - 1] = '\0';

	FT_Done_MM_Var(face->glyph->library, mm_var);
	return 1;
}
