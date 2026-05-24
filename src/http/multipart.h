#ifndef MULTIPART_H
#define MULTIPART_H

int get_boundary(const char* raw, char* boundary, int size);
void trim_multipart_footer(const char* path, const char* boundary);

#endif