#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"
#include "file.h"
#include "http.h"
#include "multipart.h"
#include "response.h"
#include "sanitize.h"

void handle_client(int client_fd) {
    RawRequest raw;
    if (read_http_request(client_fd, &raw) < 0)
        return;

    HttpRequest req;
    if (parse_http_request(raw.data, &req) < 0) {
        printf("Invalid HTTP request\n");
        free_request(&raw);
        return;
    }

    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);

    route_request(client_fd, &req, &raw);

    free_request(&raw);
}

void serve_static_file(int client_fd, const char* path) {
    char full_path[512];

    if (!is_safe_path(path)) {
        send_text_response(client_fd, 403, "Forbidden", "Forbidden");

        return;
    }

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

void serve_text(int client_fd, const char* msg) {
    send_text_response(client_fd, 200, "OK", msg);
}

void handle_upload(int client_fd, HttpRequest* req, RawRequest* raw) {
    char boundary[256];

    if (get_boundary(raw->data, boundary, sizeof(boundary)) < 0) {
        send_text_response(client_fd, 400, "Bad Request", "Missing boundary");

        return;
    }

    UploadedFile file;

    int body_size = req->content_length;

    if (parse_multipart(req->body, body_size, boundary, &file) < 0) {
        send_text_response(client_fd, 400, "Bad Request", "Invalid multipart data");

        return;
    }

    char path[512];

    snprintf(path, sizeof(path), "uploads/%s", file.filename);

    FILE* fp = fopen(path, "wb");

    if (!fp) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to save file");

        return;
    }

    fwrite(file.data, 1, file.size, fp);
    fclose(fp);

    send_text_response(client_fd, 200, "OK", "Upload succesful");
}

void route_request(int client_fd, HttpRequest* req, RawRequest* raw) {
    if (strcmp(req->path, "/") == 0) {

        serve_static_file(client_fd, "/index.html");
    } else if (strcmp(req->path, "/hello") == 0) {

        serve_text(client_fd, "Hello from server!");
    } else if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/upload") == 0) {

        handle_upload(client_fd, req, raw);
    } else {
        serve_static_file(client_fd, req->path);
    }
}