#ifndef REQUEST_H
#define REQUEST_H

typedef struct {
    char* data;
    int size;
} RawRequest;

int read_http_request(int client_fd, RawRequest* req);
void free_request(RawRequest* req);

#endif