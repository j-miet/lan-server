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

void serve_static_file(int client_fd, const char* path) {
    char full_path[512];

    if (!is_safe_path(path)) {
        send_text_response(client_fd, 403, "Forbidden", "Forbidden");

        return;
    }

    snprintf(full_path, sizeof(full_path), "public%s", path);

    long long size;
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

void handle_stream_upload(int client_fd, HttpRequest* req, const char* headers, int header_size) {
    const char* body_start = strstr(headers, "\r\n\r\n");

    if (!body_start)
        return;

    body_start += 4;

    // how many upload bytes already in memory: this includes multipart headers + initial file bytes
    int already_read = header_size - (body_start - headers);
    char boundary[256];

    if (get_boundary(headers, boundary, sizeof(boundary)) < 0) {
        send_text_response(client_fd, 400, "Bad Request", "Missing boundary");

        return;
    }

    const char* file_data_start = strstr(body_start, "\r\n\r\n"); // find multipart header end

    if (!file_data_start) {
        send_text_response(client_fd, 400, "Bad Request", "Invalid multipart");

        return;
    }

    file_data_start += 4; // move over multipart header end to access raw file bytes

    char filename[256];
    const char* filename_start = strstr(body_start, "filename=\"");

    if (!filename_start)
        return;

    filename_start += 10;

    // extract filename
    const char* filename_end = strchr(filename_start, '"');

    if (!filename_end)
        return;

    int filename_length = filename_end - filename_start;
    memcpy(filename, filename_start, filename_length);
    filename[filename_length] = '\0';

    // output file
    char full_path[512];
    long long initial_file_bytes = header_size - (file_data_start - headers); // actual file bytes in memory

    snprintf(full_path, sizeof(full_path), "uploads/%s", filename);

    FILE* fp = fopen(full_path, "wb");

    if (!fp) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to open file");

        return;
    };

    fwrite(file_data_start, 1, initial_file_bytes, fp); // write initial bytes

    // then compute remaining bytes and use a recv loop to write into file
    long long remaining = req->content_length - already_read;
    char buffer[8192];

    while (remaining > 0) {
        int to_read = remaining < (long long)sizeof(buffer) ? (int)remaining : (int)sizeof(buffer);

        int received = recv(client_fd, buffer, to_read, 0);

        if (received <= 0)
            break;

        fwrite(buffer, 1, received, fp);

        remaining -= received;
    }

    fclose(fp);

    trim_multipart_footer(full_path, boundary); // remove multipart boundary from the end of body

    send_response(client_fd, 200, "OK", "application/json", "{\"success:\":true}");
}

void handle_files_api(int client_fd) {
    DIR* dir = opendir("uploads");

    if (!dir) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to open uploads directory");

        return;
    }

    char json[16384];
    strcpy(json, "[");

    struct dirent* dir_entry;
    int first = 1;

    while ((dir_entry = readdir(dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        if (!first)
            strcat(json, ",");

        first = 0;

        char item[512];

        snprintf(item, sizeof(item), "\"%s\"", dir_entry->d_name);

        strcat(json, item);
    }

    strcat(json, "]");
    closedir(dir);

    send_response(client_fd, 200, "OK", "application/json", json);
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

    long long size;
    char* data = read_file(full_path, &size);

    if (!data) {
        send_text_response(client_fd, 404, "Not Found", "File not found");

        return;
    }

    const char* content_type = get_context_type(full_path);

    send_file_response(client_fd, content_type, data, size);

    free(data);
}

void handle_delete_file(int client_fd, const char* path) {
    const char* filename = path + 11;

    char decoded[256];

    url_decode(decoded, filename);

    if (!is_safe_path(decoded)) {
        send_text_response(client_fd, 403, "Forbidden", "Forbidden");

        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "uploads/%s", decoded);

    if (remove(full_path) != 0) {
        send_text_response(client_fd, 404, "Not Found", "Failed to delete file");

        return;
    }

    send_response(client_fd, 200, "OK", "application/json", "{\"success\": true}");
}

void route_request(int client_fd, HttpRequest* req) {
    if (strcmp(req->path, "/") == 0) {

        serve_static_file(client_fd, "/index.html");
    } else if (strcmp(req->path, "/hello") == 0) {

        serve_text(client_fd, "Hello from server!");
    } else if (strncmp(req->path, "/download/", 10) == 0) {

        handle_download(client_fd, req->path);
    } else if (strcmp(req->path, "/api/files") == 0) {

        handle_files_api(client_fd);
    } else if (strcmp(req->method, "DELETE") == 0 && strncmp(req->path, "/api/files/", 11) == 0) {

        handle_delete_file(client_fd, req->path);
    } else {
        // for other webUI dependencies
        serve_static_file(client_fd, req->path);
    }
}