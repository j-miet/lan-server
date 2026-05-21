#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include "multipart.h"

static int extract_filename(const char* body, char* filename) {
    const char* pos = strstr(body, "filename=\"");

    if (!pos)
        return -1;

    pos += 10;

    int i = 0;

    while (*pos && *pos != '"' && i < 255)
        filename[i++] = *pos++;

    filename[i] = '\0';

    return 0;
}

int parse_multipart(const char* body, int body_size, const char* boundary, UploadedFile* file) {
    if (extract_filename(body, file->filename) < 0)
        return -1;

    const char* data_start = strstr(body, "\r\n\r\n");

    if (!data_start)
        return -1;

    data_start += 4;

    char boundary_marker[256];

    snprintf(boundary_marker, sizeof(boundary_marker), "\r\n--%s", boundary);

    // multipart bodies are a mix of text+binary (can include \0 as data) so use Linux memmem instead
    const char* data_end =
        memmem(data_start, body_size - (data_start - body), boundary_marker, strlen(boundary_marker));

    if (!data_end)
        return -1;

    file->data = data_start;
    file->size = data_end - data_start;

    return 0;
}