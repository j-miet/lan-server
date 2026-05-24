#include "http.h"
#include "request.h"

#ifndef CLIENT_H
#define CLIENT_H

void handle_client(int client_fd);

void serve_static_file(int client_fd, const char* path);
void serve_text(int client_fd, const char* msg);
void handle_download(int client_fd, const char* path);

void handle_stream_upload(int client_fd, HttpRequest* req, const char* headers, int header_size);
void handle_files_api(int client_fd);
void handle_delete_file(int client_fd, const char* path);

void route_request(int client_fd, HttpRequest* req);

#endif