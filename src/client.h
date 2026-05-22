#include "http.h"
#include "request.h"

#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    char data[4096];
    int length;
} Request;

void handle_client(int client_fd);

void serve_static_file(int client_fd, const char* path);
void serve_text(int client_fd, const char* msg);
void handle_upload(int client_fd, HttpRequest* req, RawRequest* raw);
void handle_file_list(int client_fd);
void handle_download(int client_fd, const char* path);
void route_request(int client_fd, HttpRequest* req, RawRequest* raw);

#endif