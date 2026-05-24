#include <string.h>

#include "../api/files_api.h"
#include "../api/upload_api.h"
#include "routing.h"

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