#include <stdio.h>

#include "api/upload_api.h"
#include "client.h"
#include "http/routing.h"
#include "utils/common.h"

/**
 * Process a client request by reading, parsing and sending it to router
 */
void handle_client(int client_fd) {
    printf("-- New Request --\n");
    printf("Address: %s\n", get_client_ip(client_fd));

    char header_buffer[16384];
    int raw_len = read_http_headers(client_fd, header_buffer, sizeof(header_buffer) - 1);
    if (raw_len < 0) {
        printf("Failed to receive request headers\n");
        return;
    }

    HttpRequest req;
    if (parse_http_request(header_buffer, raw_len, &req) < 0) {
        printf("Invalid HTTP request\n");
        return;
    }

    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);

    RequestContext ctx = {.client_fd = client_fd, .req = &req, .raw_headers = header_buffer, .raw_len = raw_len};

    route_request(&ctx);
}