#include "json_builder.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Initialize a new JsonBuilder
 */
void json_init(JsonBuilder* jb, char* buffer, size_t size) {
    jb->buffer = buffer;
    jb->size = size;
    jb->used = 0;

    if (size > 0)
        buffer[0] = '\0';
}

/**
 * Append formatted string into JsonBuilder
 */
void json_append(JsonBuilder* jb, const char* fmt, ...) {
    if (jb->used >= jb->size)
        return;

    va_list args;
    va_start(args, fmt);

    int written = vsnprintf(jb->buffer + jb->used, jb->size - jb->used, fmt, args);

    va_end(args);

    if (written < 0)
        return;

    if ((size_t)written >= jb->size - jb->used) {
        jb->used = jb->size - 1;
        return;
    }

    jb->used += written;
}