#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include <stddef.h>

typedef struct {
    char* buffer;
    size_t size;
    size_t used;
} JsonBuilder;

int json_init(JsonBuilder* jb, size_t initial_size);
int json_append(JsonBuilder* jb, const char* fmt, ...);

#endif