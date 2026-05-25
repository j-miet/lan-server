#include <string.h>

#include "../api/files_api.h"
#include "../api/upload_api.h"
#include "context.h"
#include "routing.h"

void route_request(RequestContext* ctx) {
    if (strcmp(ctx->req->path, "/") == 0) {

        serve_static_file(ctx->client_fd, "/index.html");
    } else if (strcmp(ctx->req->path, "/hello") == 0) {

        serve_text(ctx->client_fd, "Hello from server!");
    } else if (strncmp(ctx->req->path, "/download/", 10) == 0) {

        handle_download(ctx->client_fd, ctx->req->path);
    } else if (strcmp(ctx->req->path, "/api/files") == 0) {

        handle_files_api(ctx->client_fd);
    } else if (strcmp(ctx->req->method, "POST") == 0 && strcmp(ctx->req->path, "/api/upload") == 0) {

        handle_stream_upload(ctx->client_fd, ctx->req, ctx->raw_headers, ctx->header_size);
    } else if (strcmp(ctx->req->method, "DELETE") == 0 && strncmp(ctx->req->path, "/api/files/", 11) == 0) {

        handle_delete_file(ctx->client_fd, ctx->req->path);
    } else {
        // for other webUI dependencies
        serve_static_file(ctx->client_fd, ctx->req->path);
    }
}