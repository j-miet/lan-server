#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

char* read_file(const char* path, int* size) {
    FILE* file = fopen(path, "rb");

    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    char* buffer = malloc(*size);

    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, *size, file);

    fclose(file);

    return buffer;
}

const char* get_context_type(const char* path) {
    const char* ext = strrchr(path, '.'); // file extension

    if (!ext)
        return "text/plain";

    if (strcmp(ext, ".html") == 0)
        return "text/html";

    if (strcmp(ext, ".css") == 0)
        return "text/css";

    if (strcmp(ext, ".js") == 0)
        return "application/javascript";

    return "application/octet-stream"; // default to unknown binary files
}