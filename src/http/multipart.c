#define _FILE_OFFSET_BITS 64

#include <string.h>

#include "multipart.h"

/**
 * Get boundary header field of a multipart request
 * Returns 0 on success, -1 on failure
 */
int get_boundary(const char* raw, char* boundary, int size) {
    const char* header = strstr(raw, "boundary=");
    if (!header)
        return -1;

    header += 9; // move at the end of "boundary="

    int i = 0;
    while (*header && *header != '"' && *header != '\r' && *header != '\n' && i < size - 1) {
        if (*header == '"') {
            header++;
            continue;
        }

        boundary[i++] = *header++;
    }

    boundary[i] = '\0';

    return 0;
}