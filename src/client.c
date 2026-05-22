#include <dirent.h>
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
#include "url.h"

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

    send_indirect_response(client_fd, "/upload.html");
}

void handle_file_list(int client_fd) {
    DIR* dir = opendir("uploads");

    if (!dir) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to open uploads directory");

        return;
    }

    char html[16384]; // adapt this eventually from static size to dynamic

    strcpy(html, "<!DOCTYPE html>"
                 "<html><body>"
                 "<h1>Uploaded files</h1>"
                 "<ul>");

    struct dirent* dir_entry;

    while ((dir_entry = readdir(dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        char line[1024];

        snprintf(line, sizeof(line),
                 "<li>"
                 "<a href=\"/download/%s\">%s</a>"
                 "</li>",
                 dir_entry->d_name, dir_entry->d_name);

        strcat(html, line);
    }

    strcat(html, "</ul></body></html>");
    closedir(dir);

    send_response(client_fd, 200, "OK", "text/html", html);
}

void handle_download(int client_fd, const char* path) {
    char decoded[256];

    url_decode(decoded, path + 10); // +10 because /download/ has length 10

    if (!is_safe_path(decoded)) {
        send_text_response(client_fd, 403, "Forbidden", "Forbidden");

        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "uploads/%s", decoded);

    int size;
    char* data = read_file(full_path, &size);

    if (!data) {
        send_text_response(client_fd, 404, "Not Found", "File not found");

        return;
    }

    const char* content_type = get_context_type(full_path);

    send_file_response(client_fd, content_type, data, size);

    free(data);
}

void route_request(int client_fd, HttpRequest* req, RawRequest* raw) {
    if (strcmp(req->path, "/") == 0) {

        serve_static_file(client_fd, "/index.html");
    } else if (strcmp(req->path, "/hello") == 0) {

        serve_text(client_fd, "Hello from server!");
    } else if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/upload") == 0) {

        handle_upload(client_fd, req, raw);
    } else if (strcmp(req->path, "/files") == 0) {

        handle_file_list(client_fd);
    } else if (strncmp(req->path, "/download/", 10) == 0) {

        handle_download(client_fd, req->path);
    } else {
        serve_static_file(client_fd, req->path);
    }
}