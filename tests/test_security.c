#include <stdio.h>

#include "../src/security/sanitize.h"
#include "test.h"

static void test_is_safe_path(void) {
    puts("is_safe_path");

    CHECK(is_safe_path("../etc/passwd") == 0, "relative traversal unsafe");

    CHECK(is_safe_path("/var/www/../etc") == 0, "absolute traversal unsafe");

    CHECK(is_safe_path("index.html") == 1, "simple file safe");

    CHECK(is_safe_path("status..log") == 1, "embedded dots safe");
}

void test_security(void) {
    puts("security.c");

    test_is_safe_path();
}