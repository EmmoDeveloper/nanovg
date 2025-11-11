# Simple Makefile for testing Vulkan window

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
INCLUDES := -I./src -I./src/font -I./tests \
	-I/opt/freetype/include \
	-I/opt/freetype/cairo/src \
	-I/opt/freetype/cairo/builddir/src \
	-I/work/java/ai-ide-jvm/harfbuzz/src \
	-I/opt/fribidi/lib \
	-I/opt/fribidi/build/lib \
	-I/opt/fribidi/build/gen.tab \
	-I/opt/glfw/include \
	-I/opt/lunarg-vulkan-sdk/x86_64/include
LIBS := -L/opt/lunarg-vulkan-sdk/x86_64/lib -lvulkan \
	-L/opt/freetype/objs/.libs -lfreetype \
	-L/opt/freetype/cairo/builddir/src -lcairo \
	-L/work/java/ai-ide-jvm/harfbuzz/build/src -lharfbuzz \
	-L/opt/fribidi/build/lib -lfribidi \
	-L/opt/glfw/build/src -lglfw \
	-lm -lpthread \
	-Wl,-rpath,/opt/lunarg-vulkan-sdk/x86_64/lib \
	-Wl,-rpath,/opt/freetype/objs/.libs \
	-Wl,-rpath,/opt/freetype/cairo/builddir/src \
	-Wl,-rpath,/work/java/ai-ide-jvm/harfbuzz/build/src \
	-Wl,-rpath,/opt/fribidi/build/lib \
	-Wl,-rpath,/opt/glfw/build/src

BUILD_DIR := build
TEST_SCREENDUMP_DIR := screendumps

# Automatically discover all test source files
TEST_SOURCES := $(wildcard tests/test_*.c)
# Exclude broken tests that reference unimplemented APIs
EXCLUDED_TESTS := tests/test_color_space_conversion.c tests/test_compute_shader_dispatch.c
TEST_SOURCES := $(filter-out $(EXCLUDED_TESTS),$(TEST_SOURCES))
TEST_NAMES := $(basename $(notdir $(TEST_SOURCES)))
TEST_BINS := $(addprefix $(BUILD_DIR)/, $(TEST_NAMES))
TEST_TARGETS := $(patsubst test_%,test-%,$(TEST_NAMES))

.PHONY: all clean test-all build-all-tests

all: $(BUILD_DIR)/test_visual_bounds

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_SCREENDUMP_DIR):
	mkdir -p $(TEST_SCREENDUMP_DIR)

# Font system
FONT_OBJS := $(BUILD_DIR)/nvg_font_system.o $(BUILD_DIR)/nvg_font_glyph.o \
             $(BUILD_DIR)/nvg_font_shaping.o $(BUILD_DIR)/nvg_font_info.o \
             $(BUILD_DIR)/nvg_font_colr.o

# NanoVG Vulkan backend
NVG_VK_OBJS := $(BUILD_DIR)/nvg_vk_context.o $(BUILD_DIR)/nvg_vk_buffer.o \
               $(BUILD_DIR)/nvg_vk_texture.o $(BUILD_DIR)/nvg_vk_shader.o \
               $(BUILD_DIR)/nvg_vk_pipeline.o $(BUILD_DIR)/nvg_vk_render.o \
               $(BUILD_DIR)/vknvg_msdf.o \
               $(BUILD_DIR)/nvg_vk_hdr_metadata.o $(BUILD_DIR)/nvg_vk_color_space_ubo.o \
               $(BUILD_DIR)/nvg_vk_color_space.o $(BUILD_DIR)/nvg_vk_color_space_math.o \
               $(FONT_OBJS)

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

$(BUILD_DIR)/nvg_vk_hdr_metadata.o: src/vulkan/nvg_vk_hdr_metadata.c src/vulkan/nvg_vk_hdr_metadata.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_hdr_metadata.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_color_space_ubo.o: src/vulkan/nvg_vk_color_space_ubo.c src/vulkan/nvg_vk_color_space_ubo.h src/vulkan/nvg_vk_types.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_color_space_ubo.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_color_space.o: src/vulkan/nvg_vk_color_space.c src/vulkan/nvg_vk_color_space.h src/vulkan/nvg_vk_color_space_math.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_color_space.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_vk_color_space_math.o: src/vulkan/nvg_vk_color_space_math.c src/vulkan/nvg_vk_color_space_math.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk_color_space_math.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_font_system.o: src/font/nvg_font_system.c src/font/nvg_font.h src/font/nvg_font_internal.h | $(BUILD_DIR)
	@echo "Compiling nvg_font_system.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_font_glyph.o: src/font/nvg_font_glyph.c src/font/nvg_font.h src/font/nvg_font_internal.h | $(BUILD_DIR)
	@echo "Compiling nvg_font_glyph.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_font_shaping.o: src/font/nvg_font_shaping.c src/font/nvg_font.h src/font/nvg_font_internal.h | $(BUILD_DIR)
	@echo "Compiling nvg_font_shaping.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_font_info.o: src/font/nvg_font_info.c src/font/nvg_font.h src/font/nvg_font_internal.h | $(BUILD_DIR)
	@echo "Compiling nvg_font_info.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/nvg_font_colr.o: src/font/nvg_font_colr.c src/font/nvg_font_colr.h src/font/nvg_font_internal.h | $(BUILD_DIR)
	@echo "Compiling nvg_font_colr.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/vk_shader.o: src/vulkan/vk_shader.c src/vulkan/vk_shader.h | $(BUILD_DIR)
	@echo "Compiling vk_shader.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/window_utils.o: src/tools/window_utils.c src/tools/window_utils.h | $(BUILD_DIR)
	@echo "Compiling window_utils.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_window.o: tests/test_window.c src/tools/window_utils.h src/vulkan/vk_shader.h | $(BUILD_DIR)
	@echo "Compiling test_window.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_window: $(BUILD_DIR)/test_window.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(BUILD_DIR)/nvg_vk_hdr_metadata.o
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

test-fill: $(BUILD_DIR)/test_fill $(TEST_SCREENDUMP_DIR)
	@echo "Running test_fill..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_fill

# test_convexfill
$(BUILD_DIR)/test_convexfill.o: tests/test_convexfill.c | $(BUILD_DIR)
	@echo "Compiling test_convexfill.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_convexfill: $(BUILD_DIR)/test_convexfill.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_convexfill..."
	$(CC) $^ $(LIBS) -o $@

test-convexfill: $(BUILD_DIR)/test_convexfill $(TEST_SCREENDUMP_DIR)
	@echo "Running test_convexfill..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_convexfill

# test_stroke
$(BUILD_DIR)/test_stroke.o: tests/test_stroke.c | $(BUILD_DIR)
	@echo "Compiling test_stroke.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_stroke: $(BUILD_DIR)/test_stroke.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_stroke..."
	$(CC) $^ $(LIBS) -o $@

test-stroke: $(BUILD_DIR)/test_stroke $(TEST_SCREENDUMP_DIR)
	@echo "Running test_stroke..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_stroke

# test_textures
$(BUILD_DIR)/test_textures.o: tests/test_textures.c | $(BUILD_DIR)
	@echo "Compiling test_textures.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_textures: $(BUILD_DIR)/test_textures.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_textures..."
	$(CC) $^ $(LIBS) -o $@

test-textures: $(BUILD_DIR)/test_textures $(TEST_SCREENDUMP_DIR)
	@echo "Running test_textures..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_textures

# test_blending
$(BUILD_DIR)/test_blending.o: tests/test_blending.c | $(BUILD_DIR)
	@echo "Compiling test_blending.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_blending: $(BUILD_DIR)/test_blending.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_blending..."
	$(CC) $^ $(LIBS) -o $@

test-blending: $(BUILD_DIR)/test_blending $(TEST_SCREENDUMP_DIR)
	@echo "Running test_blending..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_blending

# Pattern rule: compile any test source file
$(BUILD_DIR)/test_%.o: tests/test_%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Pattern rule: link any test binary (with NanoVG + Vulkan backend)
$(BUILD_DIR)/test_%: $(BUILD_DIR)/test_%.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o \
                     $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Pattern rule: run any test (convert hyphens to underscores for test names)
.SECONDEXPANSION:
test-%: $$(BUILD_DIR)/test_$$(subst -,_,$$*) $(TEST_SCREENDUMP_DIR)
	@echo "Running test_$(subst -,_,$*)..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" timeout 30 ./$< || true

# Build all test binaries first
build-all-tests: $(TEST_BINS)
	@echo "All test binaries built"

# Run all discovered tests
test-all: $(TEST_SCREENDUMP_DIR) build-all-tests
	@echo "Running all $(words $(TEST_TARGETS)) tests..."
	@for test in $(TEST_TARGETS); do \
		$(MAKE) -s $$test || true; \
	done
	@echo ""
	@echo "=== All $(words $(TEST_TARGETS)) Tests Complete ==="
	@echo "Screenshots saved to $(TEST_SCREENDUMP_DIR)/"
	@ls -1 $(TEST_SCREENDUMP_DIR)/*.ppm 2>/dev/null | wc -l | xargs echo "Total PPM screenshots:"
	@ls -1 $(TEST_SCREENDUMP_DIR)/*.png 2>/dev/null | wc -l | xargs echo "Total PNG screenshots:"

clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# test_scissor
$(BUILD_DIR)/test_scissor.o: tests/test_scissor.c | $(BUILD_DIR)
	@echo "Compiling test_scissor.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_scissor: $(BUILD_DIR)/test_scissor.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_scissor..."
	$(CC) $^ $(LIBS) -o $@

test-scissor: $(BUILD_DIR)/test_scissor $(TEST_SCREENDUMP_DIR)
	@echo "Running test_scissor..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_scissor

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

# test_color_space_api
$(BUILD_DIR)/test_color_space_api.o: tests/test_color_space_api.c | $(BUILD_DIR)
	@echo "Compiling test_color_space_api.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_color_space_api: $(BUILD_DIR)/test_color_space_api.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_color_space_api..."
	$(CC) $^ $(LIBS) -o $@

test-color-space-api: $(BUILD_DIR)/test_color_space_api $(TEST_SCREENDUMP_DIR)
	@echo "Running test_color_space_api..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_color_space_api

# test_shapes
$(BUILD_DIR)/test_shapes.o: tests/test_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_shapes: $(BUILD_DIR)/test_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_shapes..."
	$(CC) $^ $(LIBS) -o $@

test-shapes: $(BUILD_DIR)/test_shapes $(TEST_SCREENDUMP_DIR)
	@echo "Running test_shapes..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_shapes

# test_multi_shapes
$(BUILD_DIR)/test_multi_shapes.o: tests/test_multi_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_multi_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_multi_shapes: $(BUILD_DIR)/test_multi_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_multi_shapes..."
	$(CC) $^ $(LIBS) -o $@

test-multi-shapes: $(BUILD_DIR)/test_multi_shapes $(TEST_SCREENDUMP_DIR)
	@echo "Running test_multi_shapes..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_multi_shapes

# test_gradients
$(BUILD_DIR)/test_gradients.o: tests/test_gradients.c | $(BUILD_DIR)
	@echo "Compiling test_gradients.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_gradients: $(BUILD_DIR)/test_gradients.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_gradients..."
	$(CC) $^ $(LIBS) -o $@

test-gradients: $(BUILD_DIR)/test_gradients $(TEST_SCREENDUMP_DIR)
	@echo "Running test_gradients..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_gradients

# test_image_pattern
$(BUILD_DIR)/test_image_pattern.o: tests/test_image_pattern.c | $(BUILD_DIR)
	@echo "Compiling test_image_pattern.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_image_pattern: $(BUILD_DIR)/test_image_pattern.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_image_pattern..."
	$(CC) $^ $(LIBS) -o $@

test-image-pattern: $(BUILD_DIR)/test_image_pattern $(TEST_SCREENDUMP_DIR)
	@echo "Running test_image_pattern..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_image_pattern

# test_image_simple
$(BUILD_DIR)/test_image_simple.o: tests/test_image_simple.c | $(BUILD_DIR)
	@echo "Compiling test_image_simple.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_image_simple: $(BUILD_DIR)/test_image_simple.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_image_simple..."
	$(CC) $^ $(LIBS) -o $@

test-image-simple: $(BUILD_DIR)/test_image_simple $(TEST_SCREENDUMP_DIR)
	@echo "Running test_image_simple..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_image_simple

$(BUILD_DIR)/test_text_msdf: $(BUILD_DIR)/test_text_msdf.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_text_msdf..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/nvg_harfbuzz.o: src/nvg_harfbuzz.c src/nvg_harfbuzz.h | $(BUILD_DIR)
	@echo "Compiling nvg_harfbuzz.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_custom_font
$(BUILD_DIR)/test_custom_font.o: tests/test_custom_font.c | $(BUILD_DIR)
	@echo "Compiling test_custom_font.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_custom_font_render
$(BUILD_DIR)/test_custom_font_render.o: tests/test_custom_font_render.c | $(BUILD_DIR)
	@echo "Compiling test_custom_font_render.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_canvas_api
$(BUILD_DIR)/test_canvas_api.o: tests/test_canvas_api.c | $(BUILD_DIR)
	@echo "Compiling test_canvas_api.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_canvas_api: $(BUILD_DIR)/test_canvas_api.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/vknvg_msdf.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_canvas_api..."
	$(CC) $^ $(LIBS) -o $@

test-canvas-api: $(BUILD_DIR)/test_canvas_api
	@echo "Running test_canvas_api..."
	@VK_INSTANCE_LAYERS="" VK_LAYER_PATH="" ./$(BUILD_DIR)/test_canvas_api

# test_bidi
$(BUILD_DIR)/test_bidi.o: tests/test_bidi.c | $(BUILD_DIR)
	@echo "Compiling test_bidi.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

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


