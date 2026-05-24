#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[8];
    char path[256];
    char version[16];

    const char* body;
    long long content_length;
} HttpRequest;

int get_boundary(const char* raw, char* boundary, int size);
long long get_content_length(const char* raw);
int parse_http_request(const char* raw, HttpRequest* req);

#endif