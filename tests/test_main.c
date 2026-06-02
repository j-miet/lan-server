#include <stdio.h>

int tests_passed = 0;
int tests_failed = 0;

// test functions
void test_common(void);
void test_response(void);
void test_security(void);

int main(void) {
    puts("=== server unit tests ===\n");

    test_common();
    puts("");

    test_response();
    puts("");

    test_security();
    puts("");

    printf("=== %d passed, %d failed ===\n", tests_passed, tests_failed);

    return tests_failed ? 1 : 0;
}