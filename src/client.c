#include <stdio.h>
#include <string.h>

#include "api/upload_api.h"
#include "client.h"
#include "http/routing.h"
#include "utils/addr.h"

/**
 * Process a client request by reading, parsing and sending it to router
 */
void handle_client(int client_fd) {
    printf("-- New Request --\n");
    fprintf(stderr, "Address: %s\n", get_client_ip(client_fd));

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

    RequestContext ctx = {
        .client_fd = client_fd, .req = &req, .raw_headers = header_buffer, .header_size = header_size};

    route_request(&ctx);
}