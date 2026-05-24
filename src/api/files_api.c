#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "../filesystem/mime.h"
#include "../filesystem/sanitize.h"
#include "../filesystem/url.h"
#include "../http/request.h"
#include "../http/response.h"
#include "files_api.h"

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

    const char* content_type = get_content_type(full_path);

    send_file_stream(client_fd, full_path, content_type);
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

void serve_static_file(int client_fd, const char* path) {
    char full_path[512];

    if (!is_safe_path(path)) {
        send_text_response(client_fd, 403, "Forbidden", "Forbidden");

        return;
    }

    snprintf(full_path, sizeof(full_path), "public%s", path);

    const char* content_type = get_content_type(full_path);

    send_file_stream(client_fd, full_path, content_type);
}

void serve_text(int client_fd, const char* msg) {
    send_text_response(client_fd, 200, "OK", msg);
}