#include <string.h>

#include "sanitize.h"

int is_safe_path(const char* path) {
    if (strstr(path, "..") && path[0] != '/')
        return 0;

    return 1;
}