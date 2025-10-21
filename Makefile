# NanoVG Vulkan Backend Makefile

# Configuration
NANOVG_DIR ?= .
VULKAN_SDK ?= $(shell pkg-config --variable=prefix vulkan 2>/dev/null || echo "/usr")
BUILD_DIR ?= build

# Compiler and flags
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g -D_POSIX_C_SOURCE=199309L
INCLUDES := -I./src -I$(NANOVG_DIR)/src -I$(VULKAN_SDK)/include
LIBS := -lvulkan -lm -lpthread

# NanoVG source
NANOVG_SRC := $(NANOVG_DIR)/src/nanovg.c
NANOVG_OBJ := $(BUILD_DIR)/nanovg.o

# Virtual Atlas source
VIRTUAL_ATLAS_SRC := src/nanovg_vk_virtual_atlas.c
VIRTUAL_ATLAS_OBJ := $(BUILD_DIR)/nanovg_vk_virtual_atlas.o

# Test programs
SMOKE_TESTS := $(BUILD_DIR)/test_compile $(BUILD_DIR)/test_simple $(BUILD_DIR)/test_init
SMOKE_TEST_OBJS := $(BUILD_DIR)/test_compile.o $(BUILD_DIR)/test_simple.o $(BUILD_DIR)/test_init.o

# Unit tests
UNIT_TESTS := $(BUILD_DIR)/test_unit_texture $(BUILD_DIR)/test_unit_platform $(BUILD_DIR)/test_unit_memory $(BUILD_DIR)/test_unit_memory_leak $(BUILD_DIR)/test_atlas_prewarm $(BUILD_DIR)/test_instanced_text $(BUILD_DIR)/test_pipeline_creation $(BUILD_DIR)/test_virtual_atlas $(BUILD_DIR)/test_nvg_virtual_atlas $(BUILD_DIR)/test_cjk_rendering $(BUILD_DIR)/test_real_text_rendering $(BUILD_DIR)/test_cjk_real_rendering $(BUILD_DIR)/test_cjk_eviction
UNIT_TEST_OBJS := $(BUILD_DIR)/test_unit_texture.o $(BUILD_DIR)/test_unit_platform.o $(BUILD_DIR)/test_unit_memory.o $(BUILD_DIR)/test_unit_memory_leak.o $(BUILD_DIR)/test_atlas_prewarm.o $(BUILD_DIR)/test_instanced_text.o $(BUILD_DIR)/test_pipeline_creation.o $(BUILD_DIR)/test_virtual_atlas.o $(BUILD_DIR)/test_nvg_virtual_atlas.o $(BUILD_DIR)/test_cjk_rendering.o $(BUILD_DIR)/test_real_text_rendering.o $(BUILD_DIR)/test_cjk_real_rendering.o $(BUILD_DIR)/test_cjk_eviction.o $(BUILD_DIR)/test_utils.o

# Integration tests
INTEGRATION_TESTS := $(BUILD_DIR)/test_integration_render $(BUILD_DIR)/test_integration_text $(BUILD_DIR)/test_text_optimizations $(BUILD_DIR)/test_batch_text
INTEGRATION_TEST_OBJS := $(BUILD_DIR)/test_integration_render.o $(BUILD_DIR)/test_integration_text.o $(BUILD_DIR)/test_text_optimizations.o $(BUILD_DIR)/test_batch_text.o

# Benchmark tests
BENCHMARK_TESTS := $(BUILD_DIR)/test_benchmark $(BUILD_DIR)/test_benchmark_text_instancing $(BUILD_DIR)/test_performance_baseline
BENCHMARK_TEST_OBJS := $(BUILD_DIR)/test_benchmark.o $(BUILD_DIR)/test_benchmark_text_instancing.o $(BUILD_DIR)/test_performance_baseline.o

# All tests
ALL_TESTS := $(SMOKE_TESTS) $(UNIT_TESTS) $(INTEGRATION_TESTS) $(BENCHMARK_TESTS)
ALL_TEST_OBJS := $(SMOKE_TEST_OBJS) $(UNIT_TEST_OBJS) $(INTEGRATION_TEST_OBJS) $(BENCHMARK_TEST_OBJS) $(BUILD_DIR)/test_utils.o

.PHONY: all clean tests unit-tests integration-tests benchmark-tests smoke-tests run-tests check-deps help

all: check-deps tests

help:
	@echo "NanoVG Vulkan Backend Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all                 - Build all test programs (default)"
	@echo "  tests               - Build all tests"
	@echo "  smoke-tests         - Build smoke tests only"
	@echo "  unit-tests          - Build unit tests only"
	@echo "  integration-tests   - Build integration tests only"
	@echo "  benchmark-tests     - Build benchmark tests only"
	@echo "  run-tests           - Build and run all tests"
	@echo "  clean               - Remove build artifacts"
	@echo "  check-deps          - Verify dependencies"
	@echo ""
	@echo "Configuration (can override with make VAR=value):"
	@echo "  NANOVG_DIR   - Path to NanoVG (default: ../nanovg)"
	@echo "  VULKAN_SDK   - Path to Vulkan SDK (default: auto-detect)"
	@echo ""
	@echo "Example:"
	@echo "  make NANOVG_DIR=/path/to/nanovg"

check-deps:
	@echo "Checking dependencies..."
	@if [ ! -f "$(NANOVG_DIR)/src/nanovg.h" ]; then \
		echo "ERROR: NanoVG not found at $(NANOVG_DIR)"; \
		echo ""; \
		echo "Please clone NanoVG:"; \
		echo "  cd .. && git clone https://github.com/memononen/nanovg.git"; \
		echo ""; \
		echo "Or specify location: make NANOVG_DIR=/path/to/nanovg"; \
		exit 1; \
	fi
	@if ! pkg-config --exists vulkan 2>/dev/null && [ ! -f "$(VULKAN_SDK)/include/vulkan/vulkan.h" ]; then \
		echo "ERROR: Vulkan SDK not found"; \
		echo "Please install Vulkan SDK from https://vulkan.lunarg.com/"; \
		exit 1; \
	fi
	@echo "✓ All dependencies found"

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(NANOVG_OBJ): $(NANOVG_SRC) | $(BUILD_DIR)
	@echo "Compiling NanoVG..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(VIRTUAL_ATLAS_OBJ): $(VIRTUAL_ATLAS_SRC) | $(BUILD_DIR)
	@echo "Compiling Virtual Atlas..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_compile.o: tests/test_compile.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_simple.o: tests/test_simple.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_init.o: tests/test_init.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_compile: $(BUILD_DIR)/test_compile.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_simple: $(BUILD_DIR)/test_simple.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_init: $(BUILD_DIR)/test_init.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test utilities
$(BUILD_DIR)/test_utils.o: tests/test_utils.c tests/test_utils.h tests/test_framework.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Unit test: Texture operations
$(BUILD_DIR)/test_unit_texture.o: tests/test_unit_texture.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_unit_texture: $(BUILD_DIR)/test_unit_texture.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Platform detection
$(BUILD_DIR)/test_unit_platform.o: tests/test_unit_platform.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_unit_platform: $(BUILD_DIR)/test_unit_platform.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Memory management
$(BUILD_DIR)/test_unit_memory.o: tests/test_unit_memory.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_unit_memory: $(BUILD_DIR)/test_unit_memory.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Memory leak detection
$(BUILD_DIR)/test_unit_memory_leak.o: tests/test_unit_memory_leak.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_unit_memory_leak: $(BUILD_DIR)/test_unit_memory_leak.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Atlas pre-warming
$(BUILD_DIR)/test_atlas_prewarm.o: tests/test_atlas_prewarm.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_atlas_prewarm: $(BUILD_DIR)/test_atlas_prewarm.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_instanced_text.o: tests/test_instanced_text.c tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_instanced_text: $(BUILD_DIR)/test_instanced_text.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_pipeline_creation.o: tests/test_pipeline_creation.c tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_pipeline_creation: $(BUILD_DIR)/test_pipeline_creation.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Virtual atlas (CJK support)
$(BUILD_DIR)/test_virtual_atlas.o: tests/test_virtual_atlas.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_virtual_atlas: $(BUILD_DIR)/test_virtual_atlas.o $(BUILD_DIR)/nanovg_vk_virtual_atlas.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: NanoVG virtual atlas integration
$(BUILD_DIR)/test_nvg_virtual_atlas.o: tests/test_nvg_virtual_atlas.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_nvg_virtual_atlas: $(BUILD_DIR)/test_nvg_virtual_atlas.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: CJK rendering with virtual atlas
$(BUILD_DIR)/test_cjk_rendering.o: tests/test_cjk_rendering.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_cjk_rendering: $(BUILD_DIR)/test_cjk_rendering.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Real text rendering with nvgText()
$(BUILD_DIR)/test_real_text_rendering.o: tests/test_real_text_rendering.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_real_text_rendering: $(BUILD_DIR)/test_real_text_rendering.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Integration test: Rendering
$(BUILD_DIR)/test_integration_render.o: tests/test_integration_render.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_integration_render: $(BUILD_DIR)/test_integration_render.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Integration test: Text rendering
$(BUILD_DIR)/test_integration_text.o: tests/test_integration_text.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_integration_text: $(BUILD_DIR)/test_integration_text.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Benchmark test
$(BUILD_DIR)/test_benchmark.o: tests/test_benchmark.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_benchmark: $(BUILD_DIR)/test_benchmark.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Benchmark test: Text instancing
$(BUILD_DIR)/test_benchmark_text_instancing.o: tests/test_benchmark_text_instancing.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_benchmark_text_instancing: $(BUILD_DIR)/test_benchmark_text_instancing.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Integration test: Text optimizations
$(BUILD_DIR)/test_text_optimizations.o: tests/test_text_optimizations.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_optimizations: $(BUILD_DIR)/test_text_optimizations.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Integration test: Batch text rendering
$(BUILD_DIR)/test_batch_text.o: tests/test_batch_text.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_batch_text: $(BUILD_DIR)/test_batch_text.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Benchmark test: Performance baseline
$(BUILD_DIR)/test_performance_baseline.o: tests/test_performance_baseline.c tests/test_framework.h tests/test_utils.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_performance_baseline: $(BUILD_DIR)/test_performance_baseline.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Test targets
smoke-tests: $(SMOKE_TESTS)
	@echo "✓ Smoke tests built successfully"

unit-tests: $(UNIT_TESTS)
	@echo "✓ Unit tests built successfully"

integration-tests: $(INTEGRATION_TESTS)
	@echo "✓ Integration tests built successfully"

benchmark-tests: $(BENCHMARK_TESTS)
	@echo "✓ Benchmark tests built successfully"

tests: $(ALL_TESTS)
	@echo "✓ All tests built successfully"

run-tests: tests
	@echo ""
	@echo "Running all tests..."
	@echo "===================="
	@$(BUILD_DIR)/test_unit_texture
	@$(BUILD_DIR)/test_unit_platform
	@$(BUILD_DIR)/test_unit_memory
	@$(BUILD_DIR)/test_unit_memory_leak
	@$(BUILD_DIR)/test_atlas_prewarm
	@$(BUILD_DIR)/test_instanced_text
	@$(BUILD_DIR)/test_integration_render
	@$(BUILD_DIR)/test_integration_text
	@$(BUILD_DIR)/test_benchmark

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

# Dependencies for header changes
$(BUILD_DIR)/test_compile.o: tests/test_compile.c src/nanovg_vk.h src/nanovg_vk_*.h
$(BUILD_DIR)/test_simple.o: tests/test_simple.c src/nanovg_vk.h src/nanovg_vk_*.h
$(BUILD_DIR)/test_init.o: tests/test_init.c src/nanovg_vk.h src/nanovg_vk_*.h

$(BUILD_DIR)/test_cjk_real_rendering.o: tests/test_cjk_real_rendering.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_cjk_real_rendering: $(BUILD_DIR)/test_cjk_real_rendering.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@


$(BUILD_DIR)/test_cjk_eviction.o: tests/test_cjk_eviction.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_cjk_eviction: $(BUILD_DIR)/test_cjk_eviction.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@


$(BUILD_DIR)/test_visual_atlas.o: tests/test_visual_atlas.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_visual_atlas: $(BUILD_DIR)/test_visual_atlas.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

