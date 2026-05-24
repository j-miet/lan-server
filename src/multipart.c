#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "multipart.h"

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