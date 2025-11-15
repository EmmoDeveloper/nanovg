#!/bin/bash
# Run all generated API coverage tests

PASSED=0
FAILED=0


echo "Running test_beginframe_000..."
make build/test_beginframe_000 && ./build/test_beginframe_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_beginframe_000"
fi

echo "Running test_beginframe_001..."
make build/test_beginframe_001 && ./build/test_beginframe_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_beginframe_001"
fi

echo "Running test_beginframe_002..."
make build/test_beginframe_002 && ./build/test_beginframe_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_beginframe_002"
fi

echo "Running test_save_000..."
make build/test_save_000 && ./build/test_save_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_save_000"
fi

echo "Running test_restore_000..."
make build/test_restore_000 && ./build/test_restore_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_restore_000"
fi

echo "Running test_reset_000..."
make build/test_reset_000 && ./build/test_reset_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_reset_000"
fi

echo "Running test_globalcompositeoperation_000..."
make build/test_globalcompositeoperation_000 && ./build/test_globalcompositeoperation_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_000"
fi

echo "Running test_globalcompositeoperation_001..."
make build/test_globalcompositeoperation_001 && ./build/test_globalcompositeoperation_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_001"
fi

echo "Running test_globalcompositeoperation_002..."
make build/test_globalcompositeoperation_002 && ./build/test_globalcompositeoperation_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_002"
fi

echo "Running test_globalcompositeoperation_003..."
make build/test_globalcompositeoperation_003 && ./build/test_globalcompositeoperation_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_003"
fi

echo "Running test_globalcompositeoperation_004..."
make build/test_globalcompositeoperation_004 && ./build/test_globalcompositeoperation_004
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_004"
fi

echo "Running test_globalcompositeoperation_005..."
make build/test_globalcompositeoperation_005 && ./build/test_globalcompositeoperation_005
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_005"
fi

echo "Running test_globalcompositeoperation_006..."
make build/test_globalcompositeoperation_006 && ./build/test_globalcompositeoperation_006
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_006"
fi

echo "Running test_globalcompositeoperation_007..."
make build/test_globalcompositeoperation_007 && ./build/test_globalcompositeoperation_007
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_007"
fi

echo "Running test_globalcompositeoperation_008..."
make build/test_globalcompositeoperation_008 && ./build/test_globalcompositeoperation_008
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_008"
fi

echo "Running test_globalcompositeoperation_009..."
make build/test_globalcompositeoperation_009 && ./build/test_globalcompositeoperation_009
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_009"
fi

echo "Running test_globalcompositeoperation_010..."
make build/test_globalcompositeoperation_010 && ./build/test_globalcompositeoperation_010
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeoperation_010"
fi

echo "Running test_globalcompositeblendfunc_000..."
make build/test_globalcompositeblendfunc_000 && ./build/test_globalcompositeblendfunc_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeblendfunc_000"
fi

echo "Running test_globalcompositeblendfunc_001..."
make build/test_globalcompositeblendfunc_001 && ./build/test_globalcompositeblendfunc_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeblendfunc_001"
fi

echo "Running test_globalcompositeblendfuncseparate_000..."
make build/test_globalcompositeblendfuncseparate_000 && ./build/test_globalcompositeblendfuncseparate_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalcompositeblendfuncseparate_000"
fi

echo "Running test_globalalpha_000..."
make build/test_globalalpha_000 && ./build/test_globalalpha_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalalpha_000"
fi

echo "Running test_globalalpha_001..."
make build/test_globalalpha_001 && ./build/test_globalalpha_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalalpha_001"
fi

echo "Running test_globalalpha_002..."
make build/test_globalalpha_002 && ./build/test_globalalpha_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalalpha_002"
fi

echo "Running test_globalalpha_003..."
make build/test_globalalpha_003 && ./build/test_globalalpha_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalalpha_003"
fi

echo "Running test_globalalpha_004..."
make build/test_globalalpha_004 && ./build/test_globalalpha_004
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_globalalpha_004"
fi

echo "Running test_shapeantialias_000..."
make build/test_shapeantialias_000 && ./build/test_shapeantialias_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_shapeantialias_000"
fi

echo "Running test_shapeantialias_001..."
make build/test_shapeantialias_001 && ./build/test_shapeantialias_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_shapeantialias_001"
fi

echo "Running test_strokewidth_000..."
make build/test_strokewidth_000 && ./build/test_strokewidth_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_strokewidth_000"
fi

echo "Running test_strokewidth_001..."
make build/test_strokewidth_001 && ./build/test_strokewidth_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_strokewidth_001"
fi

echo "Running test_strokewidth_002..."
make build/test_strokewidth_002 && ./build/test_strokewidth_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_strokewidth_002"
fi

echo "Running test_strokewidth_003..."
make build/test_strokewidth_003 && ./build/test_strokewidth_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_strokewidth_003"
fi

echo "Running test_linecap_000..."
make build/test_linecap_000 && ./build/test_linecap_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linecap_000"
fi

echo "Running test_linecap_001..."
make build/test_linecap_001 && ./build/test_linecap_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linecap_001"
fi

echo "Running test_linecap_002..."
make build/test_linecap_002 && ./build/test_linecap_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linecap_002"
fi

echo "Running test_linejoin_000..."
make build/test_linejoin_000 && ./build/test_linejoin_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linejoin_000"
fi

echo "Running test_linejoin_001..."
make build/test_linejoin_001 && ./build/test_linejoin_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linejoin_001"
fi

echo "Running test_linejoin_002..."
make build/test_linejoin_002 && ./build/test_linejoin_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_linejoin_002"
fi

echo "Running test_miterlimit_000..."
make build/test_miterlimit_000 && ./build/test_miterlimit_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_miterlimit_000"
fi

echo "Running test_miterlimit_001..."
make build/test_miterlimit_001 && ./build/test_miterlimit_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_miterlimit_001"
fi

echo "Running test_resettransform_000..."
make build/test_resettransform_000 && ./build/test_resettransform_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_resettransform_000"
fi

echo "Running test_transform_000..."
make build/test_transform_000 && ./build/test_transform_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_transform_000"
fi

echo "Running test_transform_001..."
make build/test_transform_001 && ./build/test_transform_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_transform_001"
fi

echo "Running test_translate_000..."
make build/test_translate_000 && ./build/test_translate_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_translate_000"
fi

echo "Running test_translate_001..."
make build/test_translate_001 && ./build/test_translate_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_translate_001"
fi

echo "Running test_translate_002..."
make build/test_translate_002 && ./build/test_translate_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_translate_002"
fi

echo "Running test_rotate_000..."
make build/test_rotate_000 && ./build/test_rotate_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rotate_000"
fi

echo "Running test_rotate_001..."
make build/test_rotate_001 && ./build/test_rotate_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rotate_001"
fi

echo "Running test_rotate_002..."
make build/test_rotate_002 && ./build/test_rotate_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rotate_002"
fi

echo "Running test_rotate_003..."
make build/test_rotate_003 && ./build/test_rotate_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rotate_003"
fi

echo "Running test_scale_000..."
make build/test_scale_000 && ./build/test_scale_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scale_000"
fi

echo "Running test_scale_001..."
make build/test_scale_001 && ./build/test_scale_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scale_001"
fi

echo "Running test_scale_002..."
make build/test_scale_002 && ./build/test_scale_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scale_002"
fi

echo "Running test_scale_003..."
make build/test_scale_003 && ./build/test_scale_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scale_003"
fi

echo "Running test_skewx_000..."
make build/test_skewx_000 && ./build/test_skewx_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewx_000"
fi

echo "Running test_skewx_001..."
make build/test_skewx_001 && ./build/test_skewx_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewx_001"
fi

echo "Running test_skewx_002..."
make build/test_skewx_002 && ./build/test_skewx_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewx_002"
fi

echo "Running test_skewy_000..."
make build/test_skewy_000 && ./build/test_skewy_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewy_000"
fi

echo "Running test_skewy_001..."
make build/test_skewy_001 && ./build/test_skewy_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewy_001"
fi

echo "Running test_skewy_002..."
make build/test_skewy_002 && ./build/test_skewy_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_skewy_002"
fi

echo "Running test_scissor_000..."
make build/test_scissor_000 && ./build/test_scissor_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scissor_000"
fi

echo "Running test_scissor_001..."
make build/test_scissor_001 && ./build/test_scissor_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_scissor_001"
fi

echo "Running test_intersectscissor_000..."
make build/test_intersectscissor_000 && ./build/test_intersectscissor_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_intersectscissor_000"
fi

echo "Running test_resetscissor_000..."
make build/test_resetscissor_000 && ./build/test_resetscissor_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_resetscissor_000"
fi

echo "Running test_beginpath_000..."
make build/test_beginpath_000 && ./build/test_beginpath_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_beginpath_000"
fi

echo "Running test_moveto_000..."
make build/test_moveto_000 && ./build/test_moveto_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_moveto_000"
fi

echo "Running test_moveto_001..."
make build/test_moveto_001 && ./build/test_moveto_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_moveto_001"
fi

echo "Running test_lineto_000..."
make build/test_lineto_000 && ./build/test_lineto_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_lineto_000"
fi

echo "Running test_lineto_001..."
make build/test_lineto_001 && ./build/test_lineto_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_lineto_001"
fi

echo "Running test_bezierto_000..."
make build/test_bezierto_000 && ./build/test_bezierto_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_bezierto_000"
fi

echo "Running test_quadto_000..."
make build/test_quadto_000 && ./build/test_quadto_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_quadto_000"
fi

echo "Running test_arcto_000..."
make build/test_arcto_000 && ./build/test_arcto_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_arcto_000"
fi

echo "Running test_closepath_000..."
make build/test_closepath_000 && ./build/test_closepath_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_closepath_000"
fi

echo "Running test_pathwinding_000..."
make build/test_pathwinding_000 && ./build/test_pathwinding_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_pathwinding_000"
fi

echo "Running test_pathwinding_001..."
make build/test_pathwinding_001 && ./build/test_pathwinding_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_pathwinding_001"
fi

echo "Running test_arc_000..."
make build/test_arc_000 && ./build/test_arc_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_arc_000"
fi

echo "Running test_arc_001..."
make build/test_arc_001 && ./build/test_arc_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_arc_001"
fi

echo "Running test_rect_000..."
make build/test_rect_000 && ./build/test_rect_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rect_000"
fi

echo "Running test_rect_001..."
make build/test_rect_001 && ./build/test_rect_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_rect_001"
fi

echo "Running test_roundedrect_000..."
make build/test_roundedrect_000 && ./build/test_roundedrect_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_roundedrect_000"
fi

echo "Running test_roundedrect_001..."
make build/test_roundedrect_001 && ./build/test_roundedrect_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_roundedrect_001"
fi

echo "Running test_roundedrectvarying_000..."
make build/test_roundedrectvarying_000 && ./build/test_roundedrectvarying_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_roundedrectvarying_000"
fi

echo "Running test_ellipse_000..."
make build/test_ellipse_000 && ./build/test_ellipse_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_ellipse_000"
fi

echo "Running test_ellipse_001..."
make build/test_ellipse_001 && ./build/test_ellipse_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_ellipse_001"
fi

echo "Running test_circle_000..."
make build/test_circle_000 && ./build/test_circle_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_circle_000"
fi

echo "Running test_circle_001..."
make build/test_circle_001 && ./build/test_circle_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_circle_001"
fi

echo "Running test_fill_000..."
make build/test_fill_000 && ./build/test_fill_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fill_000"
fi

echo "Running test_stroke_000..."
make build/test_stroke_000 && ./build/test_stroke_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_stroke_000"
fi

echo "Running test_fonthinting_000..."
make build/test_fonthinting_000 && ./build/test_fonthinting_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fonthinting_000"
fi

echo "Running test_fonthinting_001..."
make build/test_fonthinting_001 && ./build/test_fonthinting_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fonthinting_001"
fi

echo "Running test_fonthinting_002..."
make build/test_fonthinting_002 && ./build/test_fonthinting_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fonthinting_002"
fi

echo "Running test_textdirection_000..."
make build/test_textdirection_000 && ./build/test_textdirection_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textdirection_000"
fi

echo "Running test_textdirection_001..."
make build/test_textdirection_001 && ./build/test_textdirection_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textdirection_001"
fi

echo "Running test_textdirection_002..."
make build/test_textdirection_002 && ./build/test_textdirection_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textdirection_002"
fi

echo "Running test_fontfeature_000..."
make build/test_fontfeature_000 && ./build/test_fontfeature_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontfeature_000"
fi

echo "Running test_fontfeature_001..."
make build/test_fontfeature_001 && ./build/test_fontfeature_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontfeature_001"
fi

echo "Running test_fontfeature_002..."
make build/test_fontfeature_002 && ./build/test_fontfeature_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontfeature_002"
fi

echo "Running test_fontfeaturesreset_000..."
make build/test_fontfeaturesreset_000 && ./build/test_fontfeaturesreset_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontfeaturesreset_000"
fi

echo "Running test_fontsize_000..."
make build/test_fontsize_000 && ./build/test_fontsize_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontsize_000"
fi

echo "Running test_fontsize_001..."
make build/test_fontsize_001 && ./build/test_fontsize_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontsize_001"
fi

echo "Running test_fontsize_002..."
make build/test_fontsize_002 && ./build/test_fontsize_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontsize_002"
fi

echo "Running test_fontsize_003..."
make build/test_fontsize_003 && ./build/test_fontsize_003
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontsize_003"
fi

echo "Running test_fontblur_000..."
make build/test_fontblur_000 && ./build/test_fontblur_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontblur_000"
fi

echo "Running test_fontblur_001..."
make build/test_fontblur_001 && ./build/test_fontblur_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontblur_001"
fi

echo "Running test_fontblur_002..."
make build/test_fontblur_002 && ./build/test_fontblur_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_fontblur_002"
fi

echo "Running test_textletterspacing_000..."
make build/test_textletterspacing_000 && ./build/test_textletterspacing_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textletterspacing_000"
fi

echo "Running test_textletterspacing_001..."
make build/test_textletterspacing_001 && ./build/test_textletterspacing_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textletterspacing_001"
fi

echo "Running test_textletterspacing_002..."
make build/test_textletterspacing_002 && ./build/test_textletterspacing_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textletterspacing_002"
fi

echo "Running test_textlineheight_000..."
make build/test_textlineheight_000 && ./build/test_textlineheight_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textlineheight_000"
fi

echo "Running test_textlineheight_001..."
make build/test_textlineheight_001 && ./build/test_textlineheight_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textlineheight_001"
fi

echo "Running test_textlineheight_002..."
make build/test_textlineheight_002 && ./build/test_textlineheight_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textlineheight_002"
fi

echo "Running test_textalign_000..."
make build/test_textalign_000 && ./build/test_textalign_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textalign_000"
fi

echo "Running test_textalign_001..."
make build/test_textalign_001 && ./build/test_textalign_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textalign_001"
fi

echo "Running test_textalign_002..."
make build/test_textalign_002 && ./build/test_textalign_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textalign_002"
fi

echo "Running test_subpixeltext_000..."
make build/test_subpixeltext_000 && ./build/test_subpixeltext_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_subpixeltext_000"
fi

echo "Running test_subpixeltext_001..."
make build/test_subpixeltext_001 && ./build/test_subpixeltext_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_subpixeltext_001"
fi

echo "Running test_textsubpixelmode_000..."
make build/test_textsubpixelmode_000 && ./build/test_textsubpixelmode_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textsubpixelmode_000"
fi

echo "Running test_textsubpixelmode_001..."
make build/test_textsubpixelmode_001 && ./build/test_textsubpixelmode_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textsubpixelmode_001"
fi

echo "Running test_textsubpixelmode_002..."
make build/test_textsubpixelmode_002 && ./build/test_textsubpixelmode_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_textsubpixelmode_002"
fi

echo "Running test_baselineshift_000..."
make build/test_baselineshift_000 && ./build/test_baselineshift_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_baselineshift_000"
fi

echo "Running test_baselineshift_001..."
make build/test_baselineshift_001 && ./build/test_baselineshift_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_baselineshift_001"
fi

echo "Running test_baselineshift_002..."
make build/test_baselineshift_002 && ./build/test_baselineshift_002
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_baselineshift_002"
fi

echo "Running test_kerningenabled_000..."
make build/test_kerningenabled_000 && ./build/test_kerningenabled_000
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_kerningenabled_000"
fi

echo "Running test_kerningenabled_001..."
make build/test_kerningenabled_001 && ./build/test_kerningenabled_001
if [ $? -eq 0 ]; then
	((PASSED++))
else
	((FAILED++))
	echo "FAILED: test_kerningenabled_001"
fi

echo ""
echo "===================="
echo "Tests passed: $PASSED"
echo "Tests failed: $FAILED"
echo "Total: $((PASSED + FAILED))"
