#include "helpers/fixtures.h"

#include <stdio.h>

int tests_passed = 0;
int tests_failed = 0;

// unit
void test_common(void);
void test_response(void);
void test_security(void);

// integration
void test_upload_and_download(void);
void test_upload_and_download_large(void);

void test_preview_full_file(void);
void test_preview_range(void);
void test_preview_path_traversal(void);

int main(void) {
    puts("=== unit tests ===\n");

    test_common();
    puts("");

    test_response();
    puts("");

    test_security();
    puts("");

    puts("=== integration tests ===\n");

    // wrap each test group with setup and teardown to ensure separate file state

    // upload/download
    test_setup();

    test_upload_and_download();
    test_upload_and_download_large();

    test_teardown();
    puts("");

    // file previews
    test_setup();

    test_preview_full_file();
    test_preview_range();
    test_preview_path_traversal();

    test_teardown();
    puts("");

    printf("=== %d passed, %d failed ===\n", tests_passed, tests_failed);

    return tests_failed ? 1 : 0;
}