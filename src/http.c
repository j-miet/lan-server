#include <stdio.h>
#include <string.h>

#include "http.h"

int parse_http_request(const char* raw, HttpRequest* req) {
    char request_line[1024];

    const char* line_end = strstr(raw, "\r\n");

    if (!line_end)
        return -1;

    int len = line_end - raw;

    memcpy(request_line, raw, len);
    request_line[len] = '\0';

    int parsed = sscanf(request_line, "%7s %255s %15s", req->method, req->path, req->version);

    return parsed == 3 ? 0 : -1;
}