#include <stdio.h>
#include <string.h>

#include "http.h"

int get_boundary(const char* raw, char* boundary, int size) {
    const char* header = strstr(raw, "boundary=");

    if (!header)
        return -1;

    header += 9; // move at the end of "boundary="

    int i = 0;
    while (*header && *header != '\r' && *header != '\n' && i < size - 1)
        boundary[i++] = *header++; // increment both index and pointer after allocation

    boundary[i] = '\0';

    return 0;
}

long long get_content_length(const char* raw) {
    const char* header = strstr(raw, "Content-Length");

    if (!header)
        return 0;

    long long length = 0;

    sscanf(header, "Content-Length: %lld", &length);

    return length;
}

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