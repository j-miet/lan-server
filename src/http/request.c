#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "request.h"

/**
 * Read header data into buffer and return its length
 */
int read_http_headers(int client_fd, char* buffer, int max_size) {
    int total = 0;

    while (1) {
        int received = recv(client_fd, buffer + total, max_size - total, 0);

        if (received <= 0)
            return -1;

        total += received;

        buffer[total] = '\0';

        if (strstr(buffer, "\r\n\r\n"))
            break;
    }

    return total;
}

/**
 * Get request body's content length
 */
long long get_content_length(const char* raw) {
    const char* header = strstr(raw, "Content-Length");
    if (!header)
        return 0;

    long long length = 0;
    sscanf(header, "Content-Length: %lld", &length);

    return length;
}

/**
 * Parse raw request and read data into HttpRequest struct.
 * Returns 0 on success, -1 on failure
 */
int parse_http_request(const char* raw, HttpRequest* req) {
    char request_line[1024];

    const char* line_end = strstr(raw, "\r\n");
    if (!line_end)
        return -1;

    int len = line_end - raw;

    memcpy(request_line, raw, len);
    request_line[len] = '\0';

    int parsed = sscanf(request_line, "%7s %255s %15s", req->method, req->path, req->version);
    if (parsed != 3)
        return -1;

    req->content_length = get_content_length(raw);

    // req body still points to recv buffer, could be copied separately
    const char* body_start = strstr(raw, "\r\n\r\n");
    if (body_start) {
        req->body = body_start + 4;
    } else {
        req->body = NULL;
    }

    return 0;
}
