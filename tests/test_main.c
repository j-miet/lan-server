#include <stdio.h>

int tests_passed = 0;
int tests_failed = 0;

// unit
void test_common(void);
void test_response(void);
void test_security(void);

// integration
void test_upload_and_download(void);

int main(void) {
    puts("=== unit tests ===\n");

    test_common();
    puts("");

    test_response();
    puts("");

    test_security();
    puts("");

    puts("=== integration tests ===\n");

    test_upload_and_download();
    puts("");

    printf("=== %d passed, %d failed ===\n", tests_passed, tests_failed);

    return tests_failed ? 1 : 0;
}