#!/bin/bash
set -e

echo "=== Running NanoVG Public API Tests ==="
echo ""

export VK_INSTANCE_LAYERS=""
export VK_LAYER_PATH=""

# Only test the NanoVG public API tests
TESTS=(
    "test_nvg_api"
    "test_shapes"
    "test_multi_shapes"
)

echo "Building tests..."
make build/test_nvg_api build/test_shapes build/test_multi_shapes 2>&1 | grep -E "(Linking|error)" || true
echo ""

PASSED=0
FAILED=0

for test in "${TESTS[@]}"; do
    echo "----------------------------------------"
    echo "Running: $test"
    echo "----------------------------------------"
    
    if timeout 5 build/$test 2>&1 | grep -v "^\[VULKAN\]"; then
        echo "✓ $test PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "✗ $test FAILED"
        FAILED=$((FAILED + 1))
    fi
    echo ""
done

echo "========================================"
echo "Checking generated images..."
echo "========================================"
ls -lh *.png *.ppm 2>/dev/null || echo "No images generated"
echo ""

echo "========================================"
echo "Test Results:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "========================================"

exit $FAILED
