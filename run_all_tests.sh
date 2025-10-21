#!/bin/bash
# NanoVG Vulkan Backend - Comprehensive Test Runner

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test and capture results
run_test() {
	local test_name=$1
	local test_binary=$2

	echo -e "${CYAN}Running: ${test_name}${NC}"

	if [ ! -x "./build/${test_binary}" ]; then
		echo -e "${RED}Error: ${test_binary} not found or not executable${NC}"
		((FAILED_TESTS++))
		return 1
	fi

	# Run test and capture output
	output=$(./build/${test_binary} 2>&1)
	exit_code=$?

	# Extract test results from output
	passed=$(echo "$output" | grep "Passed:" | awk '{print $2}')
	total=$(echo "$output" | grep "Total:" | awk '{print $2}')

	if [ -n "$passed" ] && [ -n "$total" ]; then
		TOTAL_TESTS=$((TOTAL_TESTS + total))
		PASSED_TESTS=$((PASSED_TESTS + passed))

		if [ $exit_code -eq 0 ]; then
			echo -e "${GREEN}  ✓ ${passed}/${total} tests passed${NC}"
		else
			echo -e "${RED}  ✗ Failed with exit code ${exit_code}${NC}"
			((FAILED_TESTS++))
		fi
	else
		echo -e "${YELLOW}  ! Could not parse test results${NC}"
	fi

	echo ""
}

# Main execution
echo ""
echo "========================================"
echo " NanoVG Vulkan - Test Suite Runner"
echo "========================================"
echo ""

# Check if tests are built
if [ ! -x "./build/test_unit_texture" ]; then
	echo -e "${YELLOW}Tests not built. Building now...${NC}"
	make tests > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo -e "${RED}Build failed! Run 'make tests' to see errors.${NC}"
		exit 1
	fi
	echo -e "${GREEN}✓ Build successful${NC}"
	echo ""
fi

# Run all test categories
echo -e "${CYAN}=== Unit Tests ===${NC}"
run_test "Texture Tests" "test_unit_texture"
run_test "Platform Tests" "test_unit_platform"
run_test "Memory Tests" "test_unit_memory"
run_test "Memory Leak Tests" "test_unit_memory_leak"

echo -e "${CYAN}=== Integration Tests ===${NC}"
run_test "Rendering Tests" "test_integration_render"
run_test "Text Tests" "test_integration_text"

echo -e "${CYAN}=== Benchmark Tests ===${NC}"
run_test "Performance Benchmarks" "test_benchmark"

# Print summary
echo "========================================"
echo " Test Results Summary"
echo "========================================"
echo ""
echo "Total Tests:  ${TOTAL_TESTS}"
echo -e "Passed:       ${GREEN}${PASSED_TESTS}${NC}"

if [ $FAILED_TESTS -gt 0 ]; then
	echo -e "Failed:       ${RED}${FAILED_TESTS}${NC}"
	echo ""
	echo -e "${RED}Some tests failed!${NC}"
	exit 1
else
	echo ""
	echo -e "${GREEN}✓ All tests passed!${NC}"
	exit 0
fi
