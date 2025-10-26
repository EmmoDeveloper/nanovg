# Simple Makefile for testing Vulkan window

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
INCLUDES := -I./src -I./tests $(shell pkg-config --cflags vulkan glfw3)
LIBS := $(shell pkg-config --libs vulkan glfw3) -lm

BUILD_DIR := build

.PHONY: all clean test test-nvg test-render

all: $(BUILD_DIR)/test_window $(BUILD_DIR)/test_nvg_vk $(BUILD_DIR)/test_render

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

clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"
