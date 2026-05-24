#include <stdio.h>
#include <string.h>

#include "api/upload_api.h"
#include "client.h"

#include "http/routing.h"

void handle_client(int client_fd) {
    char header_buffer[16384];

    int header_size = read_http_headers(client_fd, header_buffer, sizeof(header_buffer) - 1);

    if (header_size < 0)
        return;

    HttpRequest req;

    if (parse_http_request(header_buffer, &req) < 0) {
        printf("Invalid HTTP request\n");
        return;
    }

    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);

    // handle file uploads separately
    if (strcmp(req.method, "POST") == 0 && strcmp(req.path, "/api/upload") == 0) {
        handle_stream_upload(client_fd, &req, header_buffer, header_size);
        return;
    }

    // otherwise pass to router
    route_request(client_fd, &req);
}