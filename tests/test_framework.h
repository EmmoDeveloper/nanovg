#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test statistics
typedef struct {
	int total;
	int passed;
	int failed;
	int skipped;
} TestStats;

static TestStats g_test_stats = {0};
static const char* g_current_test = NULL;

// No color output
#define COLOR_RESET   ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_CYAN    ""

// Test definition macro
#define TEST(name) static void test_##name(void)

// Test suite definition
#define TEST_SUITE(name) \
	static const char* g_suite_name = name; \
	static void run_suite(void)

// Assertion macros
#define ASSERT_TRUE(expr) \
	do { \
		if (!(expr)) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s\n", __LINE__, #expr); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_FALSE(expr) \
	do { \
		if (expr) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s should be false\n", __LINE__, #expr); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_EQ(a, b) \
	do { \
		if ((a) != (b)) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s != %s\n", __LINE__, #a, #b); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_NE(a, b) \
	do { \
		if ((a) == (b)) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s == %s\n", __LINE__, #a, #b); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_NULL(ptr) \
	do { \
		if ((ptr) != NULL) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s should be NULL\n", __LINE__, #ptr); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_NOT_NULL(ptr) \
	do { \
		if ((ptr) == NULL) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %s should not be NULL\n", __LINE__, #ptr); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_STR_EQ(a, b) \
	do { \
		if (strcmp((a), (b)) != 0) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: '%s' != '%s'\n", __LINE__, (a), (b)); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
	do { \
		float diff = (a) - (b); \
		if (diff < 0) diff = -diff; \
		if (diff > (epsilon)) { \
			printf(COLOR_RED "  ✗ FAIL" COLOR_RESET " at line %d: %f != %f (epsilon: %f)\n", __LINE__, (float)(a), (float)(b), (float)(epsilon)); \
			g_test_stats.failed++; \
			return; \
		} \
	} while(0)

#define SKIP_TEST(reason) \
	do { \
		printf(COLOR_YELLOW "  ⊘ SKIP" COLOR_RESET " %s: %s\n", g_current_test, reason); \
		g_test_stats.skipped++; \
		return; \
	} while(0)

// Test runner macro
#define RUN_TEST(test_func) \
	do { \
		g_current_test = #test_func; \
		g_test_stats.total++; \
		int prev_failed = g_test_stats.failed; \
		test_func(); \
		if (g_test_stats.failed == prev_failed) { \
			printf(COLOR_GREEN "  ✓ PASS" COLOR_RESET " %s\n", #test_func); \
			g_test_stats.passed++; \
		} \
	} while(0)

// Print test summary
static void print_test_summary(void)
{
	printf("\n");
	printf("========================================\n");
	printf("Test Summary:\n");
	printf("  Total:   %d\n", g_test_stats.total);
	printf(COLOR_GREEN "  Passed:  %d\n" COLOR_RESET, g_test_stats.passed);
	if (g_test_stats.failed > 0) {
		printf(COLOR_RED "  Failed:  %d\n" COLOR_RESET, g_test_stats.failed);
	}
	if (g_test_stats.skipped > 0) {
		printf(COLOR_YELLOW "  Skipped: %d\n" COLOR_RESET, g_test_stats.skipped);
	}
	printf("========================================\n");

	if (g_test_stats.failed == 0) {
		printf(COLOR_GREEN "All tests passed!\n" COLOR_RESET);
	} else {
		printf(COLOR_RED "%d test(s) failed!\n" COLOR_RESET, g_test_stats.failed);
	}
}

// Return exit code based on test results
static int test_exit_code(void)
{
	return (g_test_stats.failed == 0) ? 0 : 1;
}

#endif // TEST_FRAMEWORK_H
