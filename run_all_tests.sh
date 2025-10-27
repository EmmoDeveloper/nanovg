#!/bin/bash
set -e

echo "=== Running Full NanoVG Vulkan Test Suite ==="
echo ""

export VK_INSTANCE_LAYERS=""
export VK_LAYER_PATH=""

TESTS=(
    "test_init"
    "test_simple"
    "test_render_pass"
    "test_fill"
    "test_convexfill"
    "test_stroke"
    "test_blending"
    "test_textures"
    "test_gradients"
    "test_shapes"
    "test_scissor"
    "test_nvg_api"
    "test_shapes"
    "test_multi_shapes"
)

PASSED=0
FAILED=0
FAILED_TESTS=""

for test in "${TESTS[@]}"; do
    echo "----------------------------------------"
    echo "Running: $test"
    echo "----------------------------------------"
    
    if timeout 5 build/$test 2>&1 | tail -5; then
        echo "✓ $test PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "✗ $test FAILED"
        FAILED=$((FAILED + 1))
        FAILED_TESTS="$FAILED_TESTS\n  - $test"
    fi
    echo ""
done

echo "========================================"
echo "Test Results:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
if [ $FAILED -gt 0 ]; then
    echo "Failed tests:$FAILED_TESTS"
fi
echo "========================================"

exit $FAILED
