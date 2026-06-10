#include "json_builder.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static int json_reserve(JsonBuilder* jb, size_t extra) {
    if (jb->used + extra < jb->size)
        return 1;

    size_t new_size = jb->size;
    while (new_size <= jb->used + extra) {
        new_size *= 2;
    }

    char* new_buf = realloc(jb->buffer, new_size);
    if (!new_buf)
        return 0;

    jb->buffer = new_buf;
    jb->size = new_size;

    return 1;
}

/**
 * Initialize a new JsonBuilder
 * Returns 1 on success, 0 on failure
 */
int json_init(JsonBuilder* jb, size_t initial_size) {
    jb->buffer = malloc(initial_size);
    if (!jb->buffer)
        return 0;

    jb->size = initial_size;
    jb->used = 0;

    if (initial_size > 0)
        jb->buffer[0] = '\0';

    return 1;
}

/**
 * Append formatted string into JsonBuilder
 * Return 1 on success, 0 on failure
 */
int json_append(JsonBuilder* jb, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // create copies of args for another vsnprintf; do not reuse same ones
    va_list args_copy;
    va_copy(args_copy, args);

    int needed = vsnprintf(NULL, 0, fmt, args_copy); // get required space
    va_end(args_copy);

    if (needed < 0) {
        va_end(args_copy);
        return 0;
    }

    if (!json_reserve(jb, (size_t)needed + 1)) {
        va_end(args_copy);
        return 0;
    }

    vsnprintf(jb->buffer + jb->used, jb->size - jb->used, fmt, args); // if needed >= 0, this will return same value
    va_end(args);

    jb->used += needed;

    return 1;
}