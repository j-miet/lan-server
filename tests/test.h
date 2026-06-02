#ifndef TEST_H
#define TEST_H

#include <stdio.h>

extern int tests_passed;
extern int tests_failed;

#define CHECK(cond, name)                                                                                              \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            printf("  PASS  %s\n", name);                                                                              \
            tests_passed++;                                                                                            \
        } else {                                                                                                       \
            printf("  FAIL  %s (line %d)\n", name, __LINE__);                                                          \
            tests_failed++;                                                                                            \
        }                                                                                                              \
    } while (0)

#endif