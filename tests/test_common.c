#include <stdio.h>
#include <string.h>

#include "../src/utils/common.h"
#include "test.h"

static void test_url_decode(void) {
    puts("url_decode");

    char out[256];

    url_decode(out, "hello%20world");
    CHECK(strcmp(out, "hello world") == 0, "%20 decode");

    url_decode(out, "hello+world");
    CHECK(strcmp(out, "hello world") == 0, "+ decode");

    url_decode(out, "end%");
    CHECK(strcmp(out, "end%") == 0, "trailing percent");
}

void test_common(void) {
    puts("common.c");

    test_url_decode();
}