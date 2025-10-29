# Simple Makefile for testing Vulkan window

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
INCLUDES := -I./src -I./tests \
	-I/opt/freetype/include \
	-I/opt/freetype/cairo/src \
	$(shell pkg-config --cflags vulkan glfw3 harfbuzz fribidi)
LIBS := $(shell pkg-config --libs vulkan glfw3 harfbuzz fribidi) \
	-L/opt/freetype/objs/.libs -lfreetype \
	-L/opt/freetype/cairo/builddir/src -lcairo \
	-lm -lpthread \
	-Wl,-rpath,/opt/freetype/objs/.libs -Wl,-rpath,/opt/freetype/cairo/builddir/src

BUILD_DIR := build

.PHONY: all clean test test-nvg test-render test-shapes test-gradients test-fill \
        test-convexfill test-stroke test-textures test-blending test-scissor test-all-phase4 \
        test-nvg-api

all: $(BUILD_DIR)/test_window $(BUILD_DIR)/test_nvg_vk $(BUILD_DIR)/test_render \
     $(BUILD_DIR)/test_shapes $(BUILD_DIR)/test_gradients $(BUILD_DIR)/test_fill \
     $(BUILD_DIR)/test_convexfill $(BUILD_DIR)/test_stroke $(BUILD_DIR)/test_textures \
     $(BUILD_DIR)/test_blending $(BUILD_DIR)/test_scissor $(BUILD_DIR)/test_nvg_api

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# NanoVG Vulkan backend
NVG_VK_OBJS := $(BUILD_DIR)/nvg_vk_context.o $(BUILD_DIR)/nvg_vk_buffer.o \
               $(BUILD_DIR)/nvg_vk_texture.o $(BUILD_DIR)/nvg_vk_shader.o \
               $(BUILD_DIR)/nvg_vk_pipeline.o $(BUILD_DIR)/nvg_vk_render.o \
               $(BUILD_DIR)/nanovg_vk_virtual_atlas.o

$(BUILD_DIR)/nvg_vk_context.o: src/vulkan/nvg_vk_context.c src/vulkan/nvg_vk_context.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_context.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_buffer.o: src/vulkan/nvg_vk_buffer.c src/vulkan/nvg_vk_buffer.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_buffer.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_texture.o: src/vulkan/nvg_vk_texture.c src/vulkan/nvg_vk_texture.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_texture.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_shader.o: src/vulkan/nvg_vk_shader.c src/vulkan/nvg_vk_shader.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_shader.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_pipeline.o: src/vulkan/nvg_vk_pipeline.c src/vulkan/nvg_vk_pipeline.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_pipeline.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_render.o: src/vulkan/nvg_vk_render.c src/vulkan/nvg_vk_render.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_render.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nanovg_vk_virtual_atlas.o: src/nanovg_vk_virtual_atlas.c src/nanovg_vk_virtual_atlas.h \
                                         src/nanovg_vk_atlas_packing.h src/nanovg_vk_multi_atlas.h \
                                         src/nanovg_vk_atlas_defrag.h | $(BUILD_DIR)
	@echo "Compiling nanovg_vk_virtual_atlas.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/vk_shader.o: src/vulkan/vk_shader.c src/vulkan/vk_shader.h | $(BUILD_DIR)
	@echo "Compiling vk_shader.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/window_utils.o: tests/window_utils.c tests/window_utils.h | $(BUILD_DIR)
	@echo "Compiling window_utils.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_window.o: tests/test_window.c tests/window_utils.h src/vulkan/vk_shader.h | $(BUILD_DIR)
	@echo "Compiling test_window.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_window: $(BUILD_DIR)/test_window.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o
	@echo "Linking test_window..."
	$(CC) $^ $(LIBS) -o $@
	@echo "Build complete!"

$(BUILD_DIR)/test_nvg_vk.o: tests/test_nvg_vk.c src/vulkan/nvg_vk_context.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling test_nvg_vk.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_vk: $(BUILD_DIR)/test_nvg_vk.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_vk..."
	$(CC) $^ $(LIBS) -o $@
	@echo "Build complete!"

$(BUILD_DIR)/test_render.o: tests/test_render.c src/vulkan/nvg_vk_context.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling test_render.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_render: $(BUILD_DIR)/test_render.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_render..."
	$(CC) $^ $(LIBS) -o $@
	@echo "Build complete!"

test: $(BUILD_DIR)/test_window
	@echo "Running test_window..."
	@./$(BUILD_DIR)/test_window

test-nvg: $(BUILD_DIR)/test_nvg_vk
	@echo "Running test_nvg_vk..."
	@./$(BUILD_DIR)/test_nvg_vk

test-render: $(BUILD_DIR)/test_render
	@echo "Running test_render..."
	@./$(BUILD_DIR)/test_render

# Old test targets removed - see complete definitions below with all dependencies

# test_fill
$(BUILD_DIR)/test_fill.o: tests/test_fill.c | $(BUILD_DIR)
	@echo "Compiling test_fill.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_fill: $(BUILD_DIR)/test_fill.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_fill..."
	$(CC) $^ $(LIBS) -o $@

test-fill: $(BUILD_DIR)/test_fill
	@echo "Running test_fill..."
	@./$(BUILD_DIR)/test_fill

# test_convexfill
$(BUILD_DIR)/test_convexfill.o: tests/test_convexfill.c | $(BUILD_DIR)
	@echo "Compiling test_convexfill.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_convexfill: $(BUILD_DIR)/test_convexfill.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_convexfill..."
	$(CC) $^ $(LIBS) -o $@

test-convexfill: $(BUILD_DIR)/test_convexfill
	@echo "Running test_convexfill..."
	@./$(BUILD_DIR)/test_convexfill

# test_stroke
$(BUILD_DIR)/test_stroke.o: tests/test_stroke.c | $(BUILD_DIR)
	@echo "Compiling test_stroke.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_stroke: $(BUILD_DIR)/test_stroke.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_stroke..."
	$(CC) $^ $(LIBS) -o $@

test-stroke: $(BUILD_DIR)/test_stroke
	@echo "Running test_stroke..."
	@./$(BUILD_DIR)/test_stroke

# test_textures
$(BUILD_DIR)/test_textures.o: tests/test_textures.c | $(BUILD_DIR)
	@echo "Compiling test_textures.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_textures: $(BUILD_DIR)/test_textures.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_textures..."
	$(CC) $^ $(LIBS) -o $@

test-textures: $(BUILD_DIR)/test_textures
	@echo "Running test_textures..."
	@./$(BUILD_DIR)/test_textures

# test_blending
$(BUILD_DIR)/test_blending.o: tests/test_blending.c | $(BUILD_DIR)
	@echo "Compiling test_blending.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_blending: $(BUILD_DIR)/test_blending.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_blending..."
	$(CC) $^ $(LIBS) -o $@

test-blending: $(BUILD_DIR)/test_blending
	@echo "Running test_blending..."
	@./$(BUILD_DIR)/test_blending

# Run all Phase 4 tests
test-all-phase4: test-shapes test-gradients test-fill test-convexfill test-stroke test-textures test-blending
	@echo ""
	@echo "=== All Phase 4 Tests Complete ==="

clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# test_scissor
$(BUILD_DIR)/test_scissor.o: tests/test_scissor.c | $(BUILD_DIR)
	@echo "Compiling test_scissor.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_scissor: $(BUILD_DIR)/test_scissor.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_scissor..."
	$(CC) $^ $(LIBS) -o $@

test-scissor: $(BUILD_DIR)/test_scissor
	@echo "Running test_scissor..."
	@./$(BUILD_DIR)/test_scissor

# NanoVG core
$(BUILD_DIR)/nanovg.o: src/nanovg.c src/nanovg.h | $(BUILD_DIR)
	@echo "Compiling nanovg.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -DFONS_USE_FREETYPE -Wno-error=implicit-function-declaration -c $< -o $@

# MSDF generation
$(BUILD_DIR)/vknvg_msdf.o: src/vknvg_msdf.c src/vknvg_msdf.h | $(BUILD_DIR)
	@echo "Compiling vknvg_msdf.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# NanoVG Vulkan API wrapper
$(BUILD_DIR)/nvg_vk.o: src/nvg_vk.c src/nvg_vk.h src/nanovg.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_nvg_api
$(BUILD_DIR)/test_nvg_api.o: tests/test_nvg_api.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_api.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_api: $(BUILD_DIR)/test_nvg_api.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_api..."
	$(CC) $^ $(LIBS) -o $@

test-nvg-api: $(BUILD_DIR)/test_nvg_api
	@echo "Running test_nvg_api..."
	@./$(BUILD_DIR)/test_nvg_api

# test_shapes
$(BUILD_DIR)/test_shapes.o: tests/test_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_shapes: $(BUILD_DIR)/test_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_shapes..."
	$(CC) $^ $(LIBS) -o $@

# test_nvg_text
$(BUILD_DIR)/test_nvg_text.o: tests/test_nvg_text.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_text.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_text: $(BUILD_DIR)/test_nvg_text.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_text..."
	$(CC) $^ $(LIBS) -o $@

# test_multi_shapes
$(BUILD_DIR)/test_multi_shapes.o: tests/test_multi_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_multi_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_multi_shapes: $(BUILD_DIR)/test_multi_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_multi_shapes..."
	$(CC) $^ $(LIBS) -o $@

# test_gradients
$(BUILD_DIR)/test_gradients.o: tests/test_gradients.c | $(BUILD_DIR)
	@echo "Compiling test_gradients.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_gradients: $(BUILD_DIR)/test_gradients.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_gradients..."
	$(CC) $^ $(LIBS) -o $@

# test_image_pattern
$(BUILD_DIR)/test_image_pattern.o: tests/test_image_pattern.c | $(BUILD_DIR)
	@echo "Compiling test_image_pattern.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_image_pattern: $(BUILD_DIR)/test_image_pattern.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_image_pattern..."
	$(CC) $^ $(LIBS) -o $@

# test_image_simple
$(BUILD_DIR)/test_image_simple.o: tests/test_image_simple.c | $(BUILD_DIR)
	@echo "Compiling test_image_simple.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_image_simple: $(BUILD_DIR)/test_image_simple.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_image_simple..."
	$(CC) $^ $(LIBS) -o $@

# test_text_msdf
$(BUILD_DIR)/test_text_msdf.o: tests/test_text_msdf.c | $(BUILD_DIR)
	@echo "Compiling test_text_msdf.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_msdf: $(BUILD_DIR)/test_text_msdf.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_text_msdf..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_text_simple_bitmap.o: tests/test_text_simple_bitmap.c | $(BUILD_DIR)
	@echo "Compiling test_text_simple_bitmap.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_simple_bitmap: $(BUILD_DIR)/test_text_simple_bitmap.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_text_simple_bitmap..."
	$(CC) $^ $(LIBS) -o $@

# test_color_emoji
$(BUILD_DIR)/test_color_emoji.o: tests/test_color_emoji.c | $(BUILD_DIR)
	@echo "Compiling test_color_emoji.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_color_emoji: $(BUILD_DIR)/test_color_emoji.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_color_emoji..."
	$(CC) $^ $(LIBS) -o $@

# Custom font system
$(BUILD_DIR)/nvg_font.o: src/nvg_font.c src/nvg_font.h | $(BUILD_DIR)
	@echo "Compiling nvg_font.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# FreeType direct integration
$(BUILD_DIR)/nvg_freetype.o: src/nvg_freetype.c src/nvg_freetype.h | $(BUILD_DIR)
	@echo "Compiling nvg_freetype.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_harfbuzz.o: src/nvg_harfbuzz.c src/nvg_harfbuzz.h | $(BUILD_DIR)
	@echo "Compiling nvg_harfbuzz.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_custom_font
$(BUILD_DIR)/test_custom_font.o: tests/test_custom_font.c | $(BUILD_DIR)
	@echo "Compiling test_custom_font.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_custom_font: $(BUILD_DIR)/test_custom_font.o $(BUILD_DIR)/nvg_font.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_custom_font..."
	$(CC) $^ $(LIBS) -o $@

# test_custom_font_render
$(BUILD_DIR)/test_custom_font_render.o: tests/test_custom_font_render.c | $(BUILD_DIR)
	@echo "Compiling test_custom_font_render.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_custom_font_render: $(BUILD_DIR)/test_custom_font_render.o $(BUILD_DIR)/nvg_font.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_custom_font_render..."
	$(CC) $^ $(LIBS) -o $@

# test_canvas_api
$(BUILD_DIR)/test_canvas_api.o: tests/test_canvas_api.c | $(BUILD_DIR)
	@echo "Compiling test_canvas_api.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_canvas_api: $(BUILD_DIR)/test_canvas_api.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_canvas_api..."
	$(CC) $^ $(LIBS) -o $@

# test_freetype_system
$(BUILD_DIR)/test_freetype_system.o: tests/test_freetype_system.c | $(BUILD_DIR)
	@echo "Compiling test_freetype_system.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_freetype_system: $(BUILD_DIR)/test_freetype_system.o $(BUILD_DIR)/nvg_freetype.o
	@echo "Linking test_freetype_system..."
	$(CC) $^ $(LIBS) -o $@

# test_freetype_rendering
$(BUILD_DIR)/test_freetype_rendering.o: tests/test_freetype_rendering.c | $(BUILD_DIR)
	@echo "Compiling test_freetype_rendering.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_freetype_rendering: $(BUILD_DIR)/test_freetype_rendering.o $(BUILD_DIR)/nvg_freetype.o
	@echo "Linking test_freetype_rendering..."
	$(CC) $^ $(LIBS) -o $@

# test_bidi
$(BUILD_DIR)/test_bidi.o: tests/test_bidi.c | $(BUILD_DIR)
	@echo "Compiling test_bidi.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_bidi: $(BUILD_DIR)/test_bidi.o $(BUILD_DIR)/nvg_freetype.o
	@echo "Linking test_bidi..."
	$(CC) $^ $(LIBS) -o $@

# test_nvg_freetype
$(BUILD_DIR)/test_nvg_freetype.o: tests/test_nvg_freetype.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_freetype.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_freetype: $(BUILD_DIR)/test_nvg_freetype.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_freetype..."
	$(CC) $^ $(LIBS) -o $@

test-nvg-freetype: $(BUILD_DIR)/test_nvg_freetype
	@echo "Running test_nvg_freetype..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" timeout 3 ./$(BUILD_DIR)/test_nvg_freetype

# test_nvg_bidi
$(BUILD_DIR)/test_nvg_bidi.o: tests/test_nvg_bidi.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_bidi.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_bidi: $(BUILD_DIR)/test_nvg_bidi.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_bidi..."
	$(CC) $^ $(LIBS) -o $@

# test_nvg_color_emoji
$(BUILD_DIR)/test_nvg_color_emoji.o: tests/test_nvg_color_emoji.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_color_emoji.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_color_emoji: $(BUILD_DIR)/test_nvg_color_emoji.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_color_emoji..."
	$(CC) $^ $(LIBS) -o $@

# test_unicode_limits
$(BUILD_DIR)/test_unicode_limits.o: tests/test_unicode_limits.c | $(BUILD_DIR)
	@echo "Compiling test_unicode_limits.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_unicode_limits: $(BUILD_DIR)/test_unicode_limits.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_unicode_limits..."
	$(CC) $^ $(LIBS) -o $@

test-unicode-limits: $(BUILD_DIR)/test_unicode_limits
	@echo "Running test_unicode_limits..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" timeout 3 ./$(BUILD_DIR)/test_unicode_limits

# test_harfbuzz_demo
$(BUILD_DIR)/test_harfbuzz_demo.o: tests/test_harfbuzz_demo.c | $(BUILD_DIR)
	@echo "Compiling test_harfbuzz_demo.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_harfbuzz_demo: $(BUILD_DIR)/test_harfbuzz_demo.o $(BUILD_DIR)/nvg_harfbuzz.o
	@echo "Linking test_harfbuzz_demo..."
	$(CC) $^ $(LIBS) -o $@

test-harfbuzz-demo: $(BUILD_DIR)/test_harfbuzz_demo
	@echo "Running test_harfbuzz_demo..."
	@./$(BUILD_DIR)/test_harfbuzz_demo

# test_shaped_text
$(BUILD_DIR)/test_shaped_text.o: tests/test_shaped_text.c | $(BUILD_DIR)
	@echo "Compiling test_shaped_text.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_shaped_text: $(BUILD_DIR)/test_shaped_text.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_freetype.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_shaped_text..."
	$(CC) $^ $(LIBS) -o $@

test-shaped-text: $(BUILD_DIR)/test_shaped_text
	@echo "Running test_shaped_text..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" timeout 5 ./$(BUILD_DIR)/test_shaped_text

