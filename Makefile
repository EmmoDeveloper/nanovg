# Simple Makefile for testing Vulkan window

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
INCLUDES := -I./src -I./tests $(shell pkg-config --cflags vulkan glfw3)
LIBS := $(shell pkg-config --libs vulkan glfw3) -lm

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
               $(BUILD_DIR)/nvg_vk_pipeline.o $(BUILD_DIR)/nvg_vk_render.o

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

$(BUILD_DIR)/test_shapes.o: tests/test_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_shapes: $(BUILD_DIR)/test_shapes.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_shapes..."
	$(CC) $^ $(LIBS) -o $@

test-shapes: $(BUILD_DIR)/test_shapes
	@echo "Running test_shapes..."
	@./$(BUILD_DIR)/test_shapes

$(BUILD_DIR)/test_gradients.o: tests/test_gradients.c | $(BUILD_DIR)
	@echo "Compiling test_gradients.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_gradients: $(BUILD_DIR)/test_gradients.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_gradients..."
	$(CC) $^ $(LIBS) -o $@

test-gradients: $(BUILD_DIR)/test_gradients
	@echo "Running test_gradients..."
	@./$(BUILD_DIR)/test_gradients

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
	$(CC) $(CFLAGS) $(INCLUDES) -Wno-error=implicit-function-declaration -c $< -o $@

# MSDF stubs (until text rendering implemented)
$(BUILD_DIR)/stb_truetype_msdf_stubs.o: src/stb_truetype_msdf_stubs.c | $(BUILD_DIR)
	@echo "Compiling stb_truetype_msdf_stubs.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# NanoVG Vulkan API wrapper
$(BUILD_DIR)/nvg_vk.o: src/nvg_vk.c src/nvg_vk.h src/nanovg.h | $(BUILD_DIR)
	@echo "Compiling nvg_vk.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# test_nvg_api
$(BUILD_DIR)/test_nvg_api.o: tests/test_nvg_api.c | $(BUILD_DIR)
	@echo "Compiling test_nvg_api.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_api: $(BUILD_DIR)/test_nvg_api.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/stb_truetype_msdf_stubs.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_nvg_api..."
	$(CC) $^ $(LIBS) -o $@

test-nvg-api: $(BUILD_DIR)/test_nvg_api
	@echo "Running test_nvg_api..."
	@./$(BUILD_DIR)/test_nvg_api

# test_shapes
$(BUILD_DIR)/test_shapes.o: tests/test_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_shapes: $(BUILD_DIR)/test_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/stb_truetype_msdf_stubs.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_shapes..."
	$(CC) $^ $(LIBS) -o $@

# test_multi_shapes
$(BUILD_DIR)/test_multi_shapes.o: tests/test_multi_shapes.c | $(BUILD_DIR)
	@echo "Compiling test_multi_shapes.c..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_multi_shapes: $(BUILD_DIR)/test_multi_shapes.o $(BUILD_DIR)/nanovg.o $(BUILD_DIR)/nvg_vk.o $(BUILD_DIR)/stb_truetype_msdf_stubs.o $(BUILD_DIR)/window_utils.o $(BUILD_DIR)/vk_shader.o $(NVG_VK_OBJS)
	@echo "Linking test_multi_shapes..."
	$(CC) $^ $(LIBS) -o $@
