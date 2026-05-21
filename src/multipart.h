#ifndef MULTIPART_H
#define MULTIPART_H

typedef struct {
    char filename[256];
    const char* data;
    int size;
} UploadedFile;

int parse_multipart(const char* body, int body_size, const char* boundary, UploadedFile* file);

#endif