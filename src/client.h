#include "http.h"

#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    char data[4096];
    int length;
} Request;

void handle_client(int client_fd);
void route_request(int client_fd, HttpRequest* req);

#endif