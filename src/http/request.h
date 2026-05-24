#ifndef REQUEST_H
#define REQUEST_H

typedef struct {
    char method[8];
    char path[256];
    char version[16];

    const char* body;
    long long content_length;
} HttpRequest;

int read_http_headers(int client_fd, char* buffer, int max_size);
int parse_http_request(const char* raw, HttpRequest* req);
long long get_content_length(const char* raw);

#endif