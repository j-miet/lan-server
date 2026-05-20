#include "http.h"

#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    char data[4096];
    int length;
} Request;

void handle_client(int client_fd);
void serve_static_file(int client_fd, const char* path);
void route_request(int client_fd, HttpRequest* req);

#endif