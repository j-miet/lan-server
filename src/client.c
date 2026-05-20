#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"
#include "file.h"
#include "http.h"
#include "response.h"

void handle_client(int client_fd) {
    char buffer[4096];

    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0)
        return;

    buffer[bytes] = '\0';
    HttpRequest req;

    if (parse_http_request(buffer, &req) < 0) {
        printf("Invalid HTTP request\n");
        return;
    }

    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);

    route_request(client_fd, &req);
}

void serve_static_file(int client_fd, const char* path) {
    char full_path[512];

    snprintf(full_path, sizeof(full_path), "public%s", path);

    int size;
    char* data = read_file(full_path, &size);

    if (!data) {
        send_text_response(client_fd, 404, "Not Found", "404 Not Found");
        return;
    }

    const char* content_type = get_context_type(full_path);

    send_file_response(client_fd, content_type, data, size);

    free(data); // empty the buffer whicn was dynamically allocated in read_file
}

void route_request(int client_fd, HttpRequest* req) {
    if (strcmp(req->path, "/") == 0) {
        serve_static_file(client_fd, "/index.html");
    } else if (strcmp(req->path, "/hello") == 0) {
        send_text_response(client_fd, 200, "OK", "Hello from server!");
    } else {
        serve_static_file(client_fd, req->path);
    }
}