#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include <stddef.h>

typedef struct {
    char* buffer;
    size_t size;
    size_t used;
} JsonBuilder;

void json_init(JsonBuilder* jb, char* buffer, size_t size);
void json_append(JsonBuilder* jb, const char* fmt, ...);

#endif