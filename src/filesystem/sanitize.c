#include <string.h>

#include "sanitize.h"

/**
 * Checks if path is a valid file path
 * Returns 1 for valid path, 0 for invalid
 */
int is_safe_path(const char* path) {
    if (strstr(path, "..") && path[0] != '/')
        return 0;

    return 1;
}