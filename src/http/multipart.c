#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "multipart.h"

int get_boundary(const char* raw, char* boundary, int size) {
    const char* header = strstr(raw, "boundary=");

    if (!header)
        return -1;

    header += 9; // move at the end of "boundary="

    int i = 0;
    while (*header && *header != '\r' && *header != '\n' && i < size - 1)
        boundary[i++] = *header++; // increment both index and pointer after allocation

    boundary[i] = '\0';

    return 0;
}

void trim_multipart_footer(const char* path, const char* boundary) {
    FILE* fp = fopen(path, "rb+");

    if (!fp)
        return;

    fseek(fp, 0, SEEK_END);

    long file_size = ftell(fp);
    long tail_size = file_size < 1024 ? file_size : 1024;

    fseek(fp, file_size - tail_size, SEEK_SET);

    char tail[1024];
    fread(tail, 1, tail_size, fp);

    char marker[512];
    snprintf(marker, sizeof(marker), "\r\n--%s", boundary);

    char* boundary_pos = strstr(tail, marker);

    if (!boundary_pos) {
        fclose(fp);

        return;
    }

    long new_size = file_size - (tail_size - (boundary_pos - tail));

    int fd = fileno(fp);
    ftruncate(fd, new_size);

    fclose(fp);
}