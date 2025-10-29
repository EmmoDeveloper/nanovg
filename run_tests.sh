#!/bin/bash

# NanoVG Vulkan - Comprehensive Test Runner
# Tests all working NanoVG API tests

export VK_INSTANCE_LAYERS=""
export VK_LAYER_PATH=""

TIMEOUT=10
PASSED=0
FAILED=0
SKIPPED=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  NanoVG Vulkan Test Suite"
echo "========================================"
echo ""

# Helper function to run test
run_test() {
	local test_name=$1
	local test_binary=$2
	local build_target=$3

	echo -n "Testing: $test_name ... "

	# Check if binary exists, if not try to build
	if [ ! -f "$test_binary" ]; then
		echo -n "building ... "
		if ! make "$build_target" >/dev/null 2>&1; then
			echo -e "${RED}❌ BUILD FAILED${NC}"
			FAILED=$((FAILED + 1))
			return 1
		fi
	fi

	# Run test
	if timeout $TIMEOUT "$test_binary" >/dev/null 2>&1; then
		local output=$(timeout $TIMEOUT "$test_binary" 2>&1)
		if echo "$output" | grep -q "✓\|Complete\|PASSED\|functional"; then
			echo -e "${GREEN}✅ PASSED${NC}"
			PASSED=$((PASSED + 1))
			return 0
		fi
	fi

	echo -e "${RED}❌ FAILED${NC}"
	FAILED=$((FAILED + 1))
	return 1
}

echo "=== TEXT RENDERING TESTS (6 tests) ==="
echo ""

run_test "Color Emoji (COLR v1)" \
	"./build/test_nvg_color_emoji" \
	"build/test_nvg_color_emoji"

run_test "BiDi Text (Arabic/Hebrew)" \
	"./build/test_nvg_bidi" \
	"build/test_nvg_bidi"

run_test "Chinese Poem (CJK)" \
	"./build/test_nvg_chinese_poem" \
	"build/test_nvg_chinese_poem"

run_test "FreeType Text" \
	"./build/test_nvg_freetype" \
	"build/test_nvg_freetype"

run_test "Text Layout & Wrapping" \
	"./build/test_nvg_text" \
	"build/test_nvg_text"

run_test "MSDF Text" \
	"./build/test_text_msdf" \
	"build/test_text_msdf"

echo ""
echo "=== GRAPHICS RENDERING TESTS (4 tests) ==="
echo ""

run_test "Canvas API (Shapes + Text)" \
	"./build/test_canvas_api" \
	"build/test_canvas_api"

run_test "Shape Primitives" \
	"./build/test_shapes" \
	"build/test_shapes"

run_test "Multiple Shapes" \
	"./build/test_multi_shapes" \
	"build/test_multi_shapes"

run_test "Gradients (Linear/Radial/Box)" \
	"./build/test_gradients" \
	"build/test_gradients"

echo ""
echo "=== FONT SYSTEM TESTS (1 test) ==="
echo ""

run_test "BiDi Low-Level (HarfBuzz + FriBidi)" \
	"./build/test_bidi" \
	"build/test_bidi"

echo ""
echo "=== BACKEND TESTS (1 test) ==="
echo ""

run_test "Virtual Atlas Integration" \
	"./build/test_virtual_atlas_integration" \
	"build/test_virtual_atlas_integration"

echo ""
echo "========================================"
echo "  Test Results Summary"
echo "========================================"
echo -e "  ${GREEN}Passed:  $PASSED${NC}"
echo -e "  ${RED}Failed:  $FAILED${NC}"
echo -e "  ${YELLOW}Skipped: $SKIPPED${NC}"
echo "  Total:   $((PASSED + FAILED + SKIPPED))"
echo "========================================"
echo ""

if [ $FAILED -eq 0 ]; then
	echo -e "${GREEN}✅ All tests passed!${NC}"
	echo ""
	echo "Generated test outputs:"
	echo "  - color_emoji_test.ppm"
	echo "  - bidi_test.ppm"
	echo "  - freetype_test.ppm"
	echo "  - (and other .ppm files)"
	echo ""
	echo "Convert to PNG with: convert <file>.ppm <file>.png"
	exit 0
else
	echo -e "${RED}❌ Some tests failed${NC}"
	echo ""
	echo "Run individual tests with:"
	echo "  VK_INSTANCE_LAYERS=\"\" VK_LAYER_PATH=\"\" ./build/test_<name>"
	exit 1
fi
