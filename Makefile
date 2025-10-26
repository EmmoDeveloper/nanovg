# Simple Makefile for testing Vulkan window

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
INCLUDES := -I./src -I./tests $(shell pkg-config --cflags vulkan glfw3)
LIBS := $(shell pkg-config --libs vulkan glfw3) -lm

BUILD_DIR := build

.PHONY: all clean test

all: $(BUILD_DIR)/test_window

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

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

test: $(BUILD_DIR)/test_window
	@echo "Running test_window..."
	@./$(BUILD_DIR)/test_window

clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"
