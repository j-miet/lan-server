#include <stdio.h>
#include <string.h>

#include "../api/files_api.h"
#include "../api/upload_api.h"
#include "../config.h"
#include "../utils/addr.h"
#include "auth.h"
#include "context.h"
#include "response.h"
#include "routing.h"

void route_request(RequestContext* ctx) {

    const char* path = ctx->req->path;
    fprintf(stderr, "Address: %s\n", get_client_ip(ctx->client_fd));

    // public routes
    if (strcmp(path, "/") == 0) {
        if (!authenticate_request(ctx)) {
            send_redirect(ctx->client_fd, "/login.html");
            return;
        }

        serve_static_file(ctx->client_fd, "/index.html");
        return;
    }

    if (strcmp(path, "/hello") == 0) {
        serve_text(ctx->client_fd, "Hello from server!");
        return;
    }

    // login/logout
    if (strcmp(ctx->req->method, "POST") == 0 && strcmp(path, "/api/login") == 0) {
        if (!authenticate_login(ctx)) {
            fprintf(stderr, "=> Login failed from %s\n", get_client_ip(ctx->client_fd));
            send_response(ctx->client_fd, 401, "Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        fprintf(stderr, "=> Login successful from %s\n", get_client_ip(ctx->client_fd));
        send_response_with_cookie(ctx->client_fd, g_config.token);
        return;
    }

    if (strcmp(path, "/api/logout") == 0) {
        send_response_clear_cookie(ctx->client_fd);
        return;
    }

    // auth routes
    if (strcmp(path, "/index.html") == 0) {
        if (!authenticate_request(ctx)) {
            send_redirect(ctx->client_fd, "/login.html");
            return;
        }
        serve_static_file(ctx->client_fd, "/index.html");
        return;
    }

    if (strncmp(path, "/download/", 10) == 0) {

        if (!authenticate_request(ctx)) {
            send_response(ctx->client_fd, 401, "Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        handle_download(ctx->client_fd, path);
        return;
    }

    // api
    if (strncmp(path, "/api/", 5) == 0) {

        if (!authenticate_request(ctx)) {
            send_response(ctx->client_fd, 401, "Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        if (strcmp(path, "/api/files") == 0) {
            handle_files_api(ctx->client_fd);
            return;
        }

        if (strcmp(ctx->req->method, "POST") == 0 && strcmp(path, "/api/upload") == 0) {

            handle_stream_upload(ctx->client_fd, ctx->req, ctx->raw_headers, ctx->header_size);
            return;
        }

        if (strcmp(ctx->req->method, "DELETE") == 0 && strncmp(path, "/api/files/", 11) == 0) {

            handle_delete_file(ctx->client_fd, path);
            return;
        }
    }

    // general end point (e.g. public css & js files)
    serve_static_file(ctx->client_fd, path);
}