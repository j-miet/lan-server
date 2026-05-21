#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "http.h"
#include "request.h"

static int find_header_end(const char* buffer) {
    const char* pos = strstr(buffer, "\r\n\r\n");

    if (!pos)
        return -1;

    // calculate distance offset to obtain integer index then add 4 to move at the start of body
    return pos - buffer + 4;
}

void free_request(RawRequest* req) {
    free(req->data);

    req->data = NULL;
    req->size = 0;
}

int read_http_request(int client_fd, RawRequest* req) {
    int capacity = 8192;

    // req->data[req->size] = '\0' overflows if req->size == capacity. Fix this by adding 1
    req->data = malloc(capacity + 1);
    req->size = 0;

    if (!req->data)
        return -1;

    int header_end = -1;
    int content_length = 0;

    while (1) {
        if (req->size >= capacity) {
            capacity *= 2;

            char* new_data = realloc(req->data, capacity + 1); // add 1 here as well

            if (!new_data) {
                free(req->data);
                return -1;
            }

            req->data = new_data;
        }

        int bytes = recv(client_fd, req->data + req->size, capacity - req->size, 0);

        if (bytes <= 0) {
            free(req->data);
            return -1;
        }

        req->size += bytes;
        req->data[req->size] = '\0';

        if (header_end < 0) {
            header_end = find_header_end(req->data);

            // if request includes body
            if (header_end >= 0) {
                content_length = get_content_length(req->data);
            }
        }

        if (header_end >= 0) {
            int total_needed = header_end + content_length;

            // if request has invalid format
            if (req->size >= total_needed)
                break;
        }
    }

    return 0;
}
