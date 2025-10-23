# NanoVG Vulkan Backend Makefile

# Configuration
NANOVG_DIR ?= .
VULKAN_SDK ?= $(shell pkg-config --variable=prefix vulkan 2>/dev/null || echo "/usr")
HARFBUZZ_DIR ?= /work/java/ai-ide-jvm/harfbuzz
BUILD_DIR ?= build

# Compiler and flags
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g -D_POSIX_C_SOURCE=199309L
INCLUDES := -I./src -I$(NANOVG_DIR)/src -I$(VULKAN_SDK)/include -I$(HARFBUZZ_DIR)/src $(shell pkg-config --cflags fribidi freetype2 libpng)
LIBS := -lvulkan -lpthread $(HARFBUZZ_DIR)/build/libharfbuzz.a $(shell pkg-config --libs fribidi freetype2 libpng) -lstdc++ -lm

# NanoVG source
NANOVG_SRC := $(NANOVG_DIR)/src/nanovg.c
NANOVG_OBJ := $(BUILD_DIR)/nanovg.o

# Virtual Atlas source
VIRTUAL_ATLAS_SRC := src/nanovg_vk_virtual_atlas.c
VIRTUAL_ATLAS_OBJ := $(BUILD_DIR)/nanovg_vk_virtual_atlas.o

# Phase 4: International Text Support
HARFBUZZ_SRC := src/nanovg_vk_harfbuzz.c
HARFBUZZ_OBJ := $(BUILD_DIR)/nanovg_vk_harfbuzz.o
BIDI_SRC := src/nanovg_vk_bidi.c
BIDI_OBJ := $(BUILD_DIR)/nanovg_vk_bidi.o
INTL_TEXT_SRC := src/nanovg_vk_intl_text.c
INTL_TEXT_OBJ := $(BUILD_DIR)/nanovg_vk_intl_text.o

# Phase 5: Advanced Text Effects
TEXT_EFFECTS_SRC := src/nanovg_vk_text_effects.c
TEXT_EFFECTS_OBJ := $(BUILD_DIR)/nanovg_vk_text_effects.o

# Phase 6: Color Emoji Support
EMOJI_TABLES_SRC := src/nanovg_vk_emoji_tables.c
EMOJI_TABLES_OBJ := $(BUILD_DIR)/nanovg_vk_emoji_tables.o
COLOR_ATLAS_SRC := src/nanovg_vk_color_atlas.c
COLOR_ATLAS_OBJ := $(BUILD_DIR)/nanovg_vk_color_atlas.o
BITMAP_EMOJI_SRC := src/nanovg_vk_bitmap_emoji.c
BITMAP_EMOJI_OBJ := $(BUILD_DIR)/nanovg_vk_bitmap_emoji.o
COLR_RENDER_SRC := src/nanovg_vk_colr_render.c
COLR_RENDER_OBJ := $(BUILD_DIR)/nanovg_vk_colr_render.o
EMOJI_INTEGRATION_SRC := src/nanovg_vk_emoji.c
EMOJI_INTEGRATION_OBJ := $(BUILD_DIR)/nanovg_vk_emoji.o
TEXT_EMOJI_SRC := src/nanovg_vk_text_emoji.c
TEXT_EMOJI_OBJ := $(BUILD_DIR)/nanovg_vk_text_emoji.o

# Emoji objects required by backend (for NVG_VK_IMPLEMENTATION)
EMOJI_BACKEND_OBJS := $(TEXT_EMOJI_OBJ) $(EMOJI_INTEGRATION_OBJ) $(BITMAP_EMOJI_OBJ) $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)

# Phase I: MSDF Generation
MSDF_SRC := src/nanovg_vk_msdf.c
MSDF_OBJ := $(BUILD_DIR)/nanovg_vk_msdf.o

# Test programs
SMOKE_TESTS := $(BUILD_DIR)/test_compile $(BUILD_DIR)/test_simple $(BUILD_DIR)/test_init
SMOKE_TEST_OBJS := $(BUILD_DIR)/test_compile.o $(BUILD_DIR)/test_simple.o $(BUILD_DIR)/test_init.o

# Fun tests (Halloween special! 🎃)
FUN_TESTS := $(BUILD_DIR)/test_bad_apple
FUN_TEST_OBJS := $(BUILD_DIR)/test_bad_apple.o

# Unit tests
UNIT_TESTS := $(BUILD_DIR)/test_unit_texture $(BUILD_DIR)/test_unit_platform $(BUILD_DIR)/test_unit_memory $(BUILD_DIR)/test_unit_memory_leak $(BUILD_DIR)/test_atlas_prewarm $(BUILD_DIR)/test_instanced_text $(BUILD_DIR)/test_pipeline_creation $(BUILD_DIR)/test_virtual_atlas $(BUILD_DIR)/test_nvg_virtual_atlas $(BUILD_DIR)/test_cjk_rendering $(BUILD_DIR)/test_real_text_rendering $(BUILD_DIR)/test_cjk_real_rendering $(BUILD_DIR)/test_cjk_eviction $(BUILD_DIR)/test_text_cache $(BUILD_DIR)/test_async_upload $(BUILD_DIR)/test_compute_raster $(BUILD_DIR)/test_atlas_packing $(BUILD_DIR)/test_multi_atlas $(BUILD_DIR)/test_atlas_resize $(BUILD_DIR)/test_atlas_defrag $(BUILD_DIR)/test_harfbuzz $(BUILD_DIR)/test_bidi $(BUILD_DIR)/test_intl_text $(BUILD_DIR)/test_text_effects $(BUILD_DIR)/test_emoji_tables $(BUILD_DIR)/test_color_atlas $(BUILD_DIR)/test_bitmap_emoji $(BUILD_DIR)/test_colr_render $(BUILD_DIR)/test_emoji_integration $(BUILD_DIR)/test_text_emoji_integration $(BUILD_DIR)/test_dual_shader $(BUILD_DIR)/test_visual_emoji
UNIT_TEST_OBJS := $(BUILD_DIR)/test_unit_texture.o $(BUILD_DIR)/test_unit_platform.o $(BUILD_DIR)/test_unit_memory.o $(BUILD_DIR)/test_unit_memory_leak.o $(BUILD_DIR)/test_atlas_prewarm.o $(BUILD_DIR)/test_instanced_text.o $(BUILD_DIR)/test_pipeline_creation.o $(BUILD_DIR)/test_virtual_atlas.o $(BUILD_DIR)/test_nvg_virtual_atlas.o $(BUILD_DIR)/test_cjk_rendering.o $(BUILD_DIR)/test_real_text_rendering.o $(BUILD_DIR)/test_cjk_real_rendering.o $(BUILD_DIR)/test_cjk_eviction.o $(BUILD_DIR)/test_text_cache.o $(BUILD_DIR)/test_async_upload.o $(BUILD_DIR)/test_compute_raster.o $(BUILD_DIR)/test_atlas_packing.o $(BUILD_DIR)/test_multi_atlas.o $(BUILD_DIR)/test_atlas_resize.o $(BUILD_DIR)/test_atlas_defrag.o $(BUILD_DIR)/test_harfbuzz.o $(BUILD_DIR)/test_bidi.o $(BUILD_DIR)/test_intl_text.o $(BUILD_DIR)/test_text_effects.o $(BUILD_DIR)/test_emoji_tables.o $(BUILD_DIR)/test_color_atlas.o $(BUILD_DIR)/test_bitmap_emoji.o $(BUILD_DIR)/test_colr_render.o $(BUILD_DIR)/test_emoji_integration.o $(BUILD_DIR)/test_text_emoji_integration.o $(BUILD_DIR)/test_dual_shader.o $(BUILD_DIR)/test_visual_emoji.o $(BUILD_DIR)/test_utils.o

# Integration tests
INTEGRATION_TESTS := $(BUILD_DIR)/test_integration_render $(BUILD_DIR)/test_integration_text $(BUILD_DIR)/test_text_optimizations $(BUILD_DIR)/test_batch_text $(BUILD_DIR)/test_phase3_integration
INTEGRATION_TEST_OBJS := $(BUILD_DIR)/test_integration_render.o $(BUILD_DIR)/test_integration_text.o $(BUILD_DIR)/test_text_optimizations.o $(BUILD_DIR)/test_batch_text.o $(BUILD_DIR)/test_phase3_integration.o

# Benchmark tests
BENCHMARK_TESTS := $(BUILD_DIR)/test_benchmark $(BUILD_DIR)/test_benchmark_text_instancing $(BUILD_DIR)/test_performance_baseline
BENCHMARK_TEST_OBJS := $(BUILD_DIR)/test_benchmark.o $(BUILD_DIR)/test_benchmark_text_instancing.o $(BUILD_DIR)/test_performance_baseline.o

# All tests
ALL_TESTS := $(SMOKE_TESTS) $(UNIT_TESTS) $(INTEGRATION_TESTS) $(BENCHMARK_TESTS) $(FUN_TESTS)
ALL_TEST_OBJS := $(SMOKE_TEST_OBJS) $(UNIT_TEST_OBJS) $(INTEGRATION_TEST_OBJS) $(BENCHMARK_TEST_OBJS) $(FUN_TEST_OBJS) $(BUILD_DIR)/test_utils.o

.PHONY: all clean tests unit-tests integration-tests benchmark-tests smoke-tests fun-tests run-tests check-deps help

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
	@echo "  fun-tests           - Build fun tests (Bad Apple!! 🎃)"
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
	@echo "  make fun-tests && ./build/test_bad_apple  # Happy Halloween! 🎃"

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

# Phase 4 object files
$(HARFBUZZ_OBJ): $(HARFBUZZ_SRC) | $(BUILD_DIR)
	@echo "Compiling HarfBuzz integration..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIDI_OBJ): $(BIDI_SRC) | $(BUILD_DIR)
	@echo "Compiling BiDi implementation..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(INTL_TEXT_OBJ): $(INTL_TEXT_SRC) | $(BUILD_DIR)
	@echo "Compiling international text integration..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Phase 5 object files
$(TEXT_EFFECTS_OBJ): $(TEXT_EFFECTS_SRC) | $(BUILD_DIR)
	@echo "Compiling text effects..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Phase 6 object files
$(EMOJI_TABLES_OBJ): $(EMOJI_TABLES_SRC) | $(BUILD_DIR)
	@echo "Compiling emoji tables..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(COLOR_ATLAS_OBJ): $(COLOR_ATLAS_SRC) | $(BUILD_DIR)
	@echo "Compiling color atlas..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BITMAP_EMOJI_OBJ): $(BITMAP_EMOJI_SRC) | $(BUILD_DIR)
	@echo "Compiling bitmap emoji..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(COLR_RENDER_OBJ): $(COLR_RENDER_SRC) | $(BUILD_DIR)
	@echo "Compiling COLR renderer..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(EMOJI_INTEGRATION_OBJ): $(EMOJI_INTEGRATION_SRC) | $(BUILD_DIR)
	@echo "Compiling emoji integration..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEXT_EMOJI_OBJ): $(TEXT_EMOJI_SRC) | $(BUILD_DIR)
	@echo "Compiling text-emoji integration..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Phase I: MSDF Generation
$(MSDF_OBJ): $(MSDF_SRC) | $(BUILD_DIR)
	@echo "Compiling MSDF generator..."
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

$(BUILD_DIR)/test_compile: $(BUILD_DIR)/test_compile.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ) $(EMOJI_BACKEND_OBJS)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_simple: $(BUILD_DIR)/test_simple.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ) $(EMOJI_BACKEND_OBJS)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_init: $(BUILD_DIR)/test_init.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ) $(EMOJI_BACKEND_OBJS)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Fun tests (Halloween special! 🎃)
$(BUILD_DIR)/test_bad_apple.o: tests/test_bad_apple.c | $(BUILD_DIR)
	@echo "Compiling $<... (Bad Apple!! Touhou Edition)"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_bad_apple: $(BUILD_DIR)/test_bad_apple.o
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

$(BUILD_DIR)/test_unit_texture: $(BUILD_DIR)/test_unit_texture.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ) $(TEXT_EMOJI_OBJ) $(EMOJI_INTEGRATION_OBJ) $(BITMAP_EMOJI_OBJ) $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
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

# Integration test: Phase 3 integration
$(BUILD_DIR)/test_phase3_integration.o: tests/test_phase3_integration.c src/nanovg_vk_atlas_packing.h src/nanovg_vk_multi_atlas.h src/nanovg_vk_atlas_defrag.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_phase3_integration: $(BUILD_DIR)/test_phase3_integration.o
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

fun-tests: $(FUN_TESTS)
	@echo "✓ Fun tests built successfully (Bad Apple!! 🎃👻)"

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

# Unit test: Text cache
$(BUILD_DIR)/test_text_cache.o: tests/test_text_cache.c tests/test_framework.h src/nanovg_vk_text_cache.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_cache: $(BUILD_DIR)/test_text_cache.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Async upload
$(BUILD_DIR)/test_async_upload.o: tests/test_async_upload.c tests/test_framework.h src/nanovg_vk_async_upload.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_async_upload: $(BUILD_DIR)/test_async_upload.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Compute rasterization
$(BUILD_DIR)/test_compute_raster.o: tests/test_compute_raster.c tests/test_framework.h src/nanovg_vk_compute_raster.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_compute_raster: $(BUILD_DIR)/test_compute_raster.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Atlas packing
$(BUILD_DIR)/test_atlas_packing.o: tests/test_atlas_packing.c src/nanovg_vk_atlas_packing.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_atlas_packing: $(BUILD_DIR)/test_atlas_packing.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Multi-atlas
$(BUILD_DIR)/test_multi_atlas.o: tests/test_multi_atlas.c src/nanovg_vk_multi_atlas.h src/nanovg_vk_atlas_packing.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_multi_atlas: $(BUILD_DIR)/test_multi_atlas.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Atlas resize
$(BUILD_DIR)/test_atlas_resize.o: tests/test_atlas_resize.c src/nanovg_vk_multi_atlas.h src/nanovg_vk_atlas_packing.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_atlas_resize: $(BUILD_DIR)/test_atlas_resize.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Atlas defragmentation
$(BUILD_DIR)/test_atlas_defrag.o: tests/test_atlas_defrag.c src/nanovg_vk_atlas_defrag.h src/nanovg_vk_multi_atlas.h src/nanovg_vk_atlas_packing.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_atlas_defrag: $(BUILD_DIR)/test_atlas_defrag.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_visual_atlas.o: tests/test_visual_atlas.c tests/test_utils.h src/nanovg_vk_virtual_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_visual_atlas: $(BUILD_DIR)/test_visual_atlas.o $(BUILD_DIR)/test_utils.o $(NANOVG_OBJ) $(VIRTUAL_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Phase 4: International Text Support Tests

# Unit test: HarfBuzz integration
$(BUILD_DIR)/test_harfbuzz.o: tests/test_harfbuzz.c src/nanovg_vk_harfbuzz.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_harfbuzz: $(BUILD_DIR)/test_harfbuzz.o $(HARFBUZZ_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: BiDi algorithm
$(BUILD_DIR)/test_bidi.o: tests/test_bidi.c src/nanovg_vk_bidi.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_bidi: $(BUILD_DIR)/test_bidi.o $(BIDI_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: International text integration
$(BUILD_DIR)/test_intl_text.o: tests/test_intl_text.c src/nanovg_vk_intl_text.h src/nanovg_vk_harfbuzz.h src/nanovg_vk_bidi.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_intl_text: $(BUILD_DIR)/test_intl_text.o $(INTL_TEXT_OBJ) $(HARFBUZZ_OBJ) $(BIDI_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Phase 5: Advanced Text Effects Tests

# Unit test: Text effects
$(BUILD_DIR)/test_text_effects.o: tests/test_text_effects.c src/nanovg_vk_text_effects.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_effects: $(BUILD_DIR)/test_text_effects.o $(TEXT_EFFECTS_OBJ) $(NANOVG_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Phase 6: Color Emoji Support Tests

# Unit test: Emoji table parsing
$(BUILD_DIR)/test_emoji_tables.o: tests/test_emoji_tables.c src/nanovg_vk_emoji_tables.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_emoji_tables: $(BUILD_DIR)/test_emoji_tables.o $(EMOJI_TABLES_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Unit test: Color atlas
$(BUILD_DIR)/test_color_atlas.o: tests/test_color_atlas.c src/nanovg_vk_color_atlas.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_color_atlas: $(BUILD_DIR)/test_color_atlas.o $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_bitmap_emoji.o: tests/test_bitmap_emoji.c src/nanovg_vk_bitmap_emoji.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_bitmap_emoji: $(BUILD_DIR)/test_bitmap_emoji.o $(BITMAP_EMOJI_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_colr_render.o: tests/test_colr_render.c src/nanovg_vk_colr_render.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_colr_render: $(BUILD_DIR)/test_colr_render.o $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_emoji_integration.o: tests/test_emoji_integration.c src/nanovg_vk_emoji.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_emoji_integration: $(BUILD_DIR)/test_emoji_integration.o $(EMOJI_INTEGRATION_OBJ) $(BITMAP_EMOJI_OBJ) $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

$(BUILD_DIR)/test_text_emoji_integration.o: tests/test_text_emoji_integration.c src/nanovg_vk_text_emoji.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_text_emoji_integration: $(BUILD_DIR)/test_text_emoji_integration.o $(TEXT_EMOJI_OBJ) $(EMOJI_INTEGRATION_OBJ) $(BITMAP_EMOJI_OBJ) $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Dual-mode shader test (Phase 6.6)
$(BUILD_DIR)/test_dual_shader.o: tests/test_dual_shader.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_dual_shader: $(BUILD_DIR)/test_dual_shader.o
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

# Visual emoji test (Phase 6.7)
$(BUILD_DIR)/test_visual_emoji.o: tests/test_visual_emoji.c src/nanovg_vk_text_emoji.h src/nanovg_vk_emoji.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_visual_emoji: $(BUILD_DIR)/test_visual_emoji.o $(TEXT_EMOJI_OBJ) $(EMOJI_INTEGRATION_OBJ) $(BITMAP_EMOJI_OBJ) $(COLR_RENDER_OBJ) $(EMOJI_TABLES_OBJ) $(COLOR_ATLAS_OBJ)
	@echo "Linking $@..."
	$(CC) $^ $(LIBS) -o $@

