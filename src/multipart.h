#ifndef MULTIPART_H
#define MULTIPART_H

typedef struct {
    char filename[256];
    const char* data;
    long long size;
} UploadedFile;

int parse_multipart(const char* body, int body_size, const char* boundary, UploadedFile* file);
void trim_multipart_footer(const char* path, const char* boundary);

#endif