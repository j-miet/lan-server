#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[8];
    char path[256];
    char version[16];

    const char* body;
    int content_length;
} HttpRequest;

int parse_http_request(const char* raw, HttpRequest* req);

#endif