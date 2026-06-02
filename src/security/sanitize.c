#include <string.h>

#include "sanitize.h"

/**
 * Checks if file path is valid
 * Returns 1 for valid path, 0 for invalid
 */
int is_safe_path(const char* path) {
    const char* p = path;

    // "../" or exactly ".."
    if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0'))
        return 0;

    // otherwise look for "/../" or "/.."
    while (*p) {
        if (p[0] == '/' && p[1] == '.' && p[2] == '.' && (p[3] == '/' || p[3] == '\0'))
            return 0;

        p++;
    }

    return 1;
}