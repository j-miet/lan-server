#include <stdio.h>
#include <string.h>

#include "../api/files_api.h"
#include "../api/script_api.h"
#include "../api/upload_api.h"
#include "../auth/auth.h"
#include "../config.h"
#include "../scripting/jobs.h"
#include "../utils/common.h"
#include "context.h"
#include "response.h"
#include "routing.h"

typedef enum {
    DIRECT,
    PREFIX,
} RoutePrefix;

typedef enum {
    PUBLIC,
    AUTH
} RouteAuth;

static int match_route(Route* r, RequestContext* ctx) {
    const char* method = ctx->req->method;
    const char* path = ctx->req->path;

    if (r->method && strcmp(r->method, method) != 0)
        return 0;

    if (r->is_prefix) {
        return strncmp(path, r->path, strlen(r->path)) == 0;
    }

    return strcmp(path, r->path) == 0;
}

// route handlers

static void routing_root(RequestContext* ctx) {
    if (!authenticate_request(ctx)) {
        send_redirect(ctx->client_fd, "/login.html");
        return;
    }

    serve_static_file(ctx->client_fd, "/index.html");
}

static void routing_login(RequestContext* ctx) {
    if (!authenticate_login(ctx)) {
        fprintf(stderr, "=> Login failed from %s\n", get_client_ip(ctx->client_fd));
        send_response(ctx->client_fd, 401, "Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    fprintf(stderr, "=> Login successful from %s\n", get_client_ip(ctx->client_fd));
    send_response_with_cookie(ctx->client_fd, g_config.token);
}

static void routing_logout(RequestContext* ctx) {
    send_response_clear_cookie(ctx->client_fd);
}

static void routing_hello(RequestContext* ctx) {
    serve_text(ctx->client_fd, "Hello from server!");
}

static void routing_index(RequestContext* ctx) {
    if (!authenticate_request(ctx)) {
        send_redirect(ctx->client_fd, "/login.html");
        return;
    }

    serve_static_file(ctx->client_fd, "/index.html");
}

static void routing_download(RequestContext* ctx) {
    handle_download(ctx->client_fd, ctx->req->path);
}

static void routing_preview(RequestContext* ctx) {
    handle_preview(ctx->client_fd, ctx->req);
}

static void routing_files(RequestContext* ctx) {
    handle_files_api(ctx->client_fd);
}

static void routing_upload(RequestContext* ctx) {
    handle_stream_upload(ctx->client_fd, ctx->req);
}

static void routing_delete(RequestContext* ctx) {
    handle_delete_file(ctx->client_fd, ctx->req->path);
}

static void routing_script_api(RequestContext* ctx) {
    handle_scripts_api(ctx->client_fd);
}

static void routing_script_execute(RequestContext* ctx) {
    handle_script_execute(ctx->client_fd, ctx->req);
}

static void routing_job_status(RequestContext* ctx) {
    handle_job_status(ctx->client_fd, ctx->req->path);
}

static void routing_job_output(RequestContext* ctx) {
    handle_job_output(ctx->client_fd, ctx->req->path);
}

// all routes
static Route routes[] = {{"GET", "/", DIRECT, PUBLIC, routing_root},
                         {"GET", "/hello", DIRECT, PUBLIC, routing_hello},
                         {"POST", "/api/login", DIRECT, PUBLIC, routing_login},
                         {"POST", "/api/logout", DIRECT, PUBLIC, routing_logout},
                         {"GET", "/index.html", DIRECT, PUBLIC, routing_index},
                         {"GET", "/download", PREFIX, AUTH, routing_download},
                         {"GET", "/preview", PREFIX, AUTH, routing_preview},
                         {"GET", "/api/files", DIRECT, AUTH, routing_files},
                         {"POST", "/api/upload", DIRECT, AUTH, routing_upload},
                         {"DELETE", "/api/files", PREFIX, AUTH, routing_delete},
                         {"GET", "/api/scripts", DIRECT, AUTH, routing_script_api},
                         {"POST", "/api/scripts/run", DIRECT, AUTH, routing_script_execute},
                         {"GET", "/api/jobs/status", PREFIX, AUTH, routing_job_status},
                         {"GET", "/api/jobs/output", PREFIX, AUTH, routing_job_output},
                         {NULL, NULL, DIRECT, PUBLIC, NULL}};

/**
 * Router function
 */
void route_request(RequestContext* ctx) {
    for (int i = 0; routes[i].handler != NULL; i++) {
        Route* r = &routes[i];

        if (!match_route(r, ctx))
            continue;

        if (r->auth_required) {
            if (!authenticate_request(ctx)) {
                send_response(ctx->client_fd, 401, "Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
                return;
            }
        }

        cleanup_jobs(); // clean-up

        r->handler(ctx); // call appropriate route handler
        return;
    }

    // general end point (e.g. public css & js files)
    serve_static_file(ctx->client_fd, ctx->req->path);
}