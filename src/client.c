#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"
#include "http.h"
#include "response.h"

void handle_client(int client_fd) {
    char buffer[4096];

    while (1) {
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0)
            return;

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
}

void route_request(int client_fd, HttpRequest* req) {
    if (strcmp(req->path, "/") == 0) {
        send_text_response(client_fd, 200, "OK", "Home page");
    } else if (strcmp(req->path, "/hello") == 0) {
        send_text_response(client_fd, 200, "OK", "Hello!");
    } else {
        send_text_response(client_fd, 404, "Not Found", "404 Not Found");
    }
}