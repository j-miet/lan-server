#include "http_test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int make_test_socketpair(TestSocketPair* sp) {
    int fds[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        return -1;

    sp->client_fd = fds[0];
    sp->server_fd = fds[1];

    return 0;
}

char* drain_response(int fd, int* out_len) {
    int cap = 65536;
    int total = 0;

    char* buf = malloc(cap);

    while (1) {
        int n = recv(fd, buf + total, cap - total - 1, 0);

        if (n <= 0)
            break;

        total += n;

        if (total >= cap - 1) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }

    buf[total] = '\0';

    if (out_len)
        *out_len = total;

    return buf;
}

int parse_status(const char* resp) {
    int code = 0;

    sscanf(resp, "HTTP/1.1 %d", &code);

    return code;
}

const char* response_body(const char* resp) {
    const char* p = strstr(resp, "\r\n\r\n");

    return p ? p + 4 : NULL;
}