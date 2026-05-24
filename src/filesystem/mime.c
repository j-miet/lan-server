#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mime.h"

const char* get_content_type(const char* path) {
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