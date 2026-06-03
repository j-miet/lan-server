#include "../test.h"

#include <stdio.h>

#include "../src/http/response.h"

static void test_parse_range(void) {
    puts("parse_range");

    HttpRange r;

    r = parse_range(NULL, 1000);
    CHECK(r.valid == 0, "null header -> invalid");
    CHECK(r.start == 0, "null header -> start");
    CHECK(r.end == 999, "null header -> end");

    r = parse_range("bytes=0-499", 1000);
    CHECK(r.valid == 1, "full range valid");
    CHECK(r.start == 0, "full range start");
    CHECK(r.end == 499, "full range end");

    r = parse_range("bytes=200-", 1000);
    CHECK(r.valid == 1, "open ended valid");
    CHECK(r.start == 200, "open ended start");
    CHECK(r.end == 999, "open ended end");

    r = parse_range("bytes=-300", 1000);
    CHECK(r.valid == 1, "suffix valid");
    CHECK(r.start == 700, "suffix start");
    CHECK(r.end == 999, "suffix end");

    r = parse_range("bytes=800-200", 1000);
    CHECK(r.valid == 0, "inverted invalid");
}

void test_response(void) {
    puts("response.c");

    test_parse_range();
}