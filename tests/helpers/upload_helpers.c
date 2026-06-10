#include "http_helpers.h"

#include "../../src/api/upload_api.h"
#include "../../src/http/request.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int build_upload_request(const char* filename, const char* file_content, int file_len, char** out_buf) {
    const char* boundary = "----TestBoundary1234";

    char part_header[512];
    int part_header_len = snprintf(part_header, sizeof(part_header),
                                   "--%s\r\n"
                                   "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
                                   "Content-Type: application/octet-stream\r\n"
                                   "\r\n",
                                   boundary, filename);

    char footer[128];
    int footer_len = snprintf(footer, sizeof(footer), "\r\n--%s--\r\n", boundary);

    int body_len = part_header_len + file_len + footer_len;

    char http_header[512];
    int http_header_len = snprintf(http_header, sizeof(http_header),
                                   "POST /api/upload HTTP/1.1\r\n"
                                   "Host: localhost\r\n"
                                   "Content-Type: multipart/form-data; boundary=%s\r\n"
                                   "Content-Length: %d\r\n"
                                   "\r\n",
                                   boundary, body_len);

    int total = http_header_len + body_len;
    char* buf = malloc(total);

    int off = 0;

    memcpy(buf + off, http_header, http_header_len);
    off += http_header_len;

    memcpy(buf + off, part_header, part_header_len);
    off += part_header_len;

    memcpy(buf + off, file_content, file_len);
    off += file_len;

    memcpy(buf + off, footer, footer_len);

    *out_buf = buf;

    return total;
}

int do_upload(const char* filename, const char* content, int length) {
    char* raw = NULL;
    int raw_len = build_upload_request(filename, content, length, &raw);

    TestSocketPair sp;
    if (make_test_socketpair(&sp) != 0) {
        free(raw);
        return -1;
    }

    send(sp.client_fd, raw, raw_len, 0);
    shutdown(sp.client_fd, SHUT_WR);

    char buffer[16384];
    int rlen = recv(sp.server_fd, buffer, sizeof(buffer) - 1, 0);
    if (rlen <= 0) {
        free(raw);
        return -1;
    }
    buffer[rlen] = '\0';

    HttpRequest hreq;
    if (parse_http_request(buffer, rlen, &hreq) != 0) {
        free(raw);
        return -1;
    }

    handle_stream_upload(sp.server_fd, &hreq);
    shutdown(sp.server_fd, SHUT_WR);

    char* resp = drain_response(sp.client_fd, NULL);
    int ok = parse_status(resp) == 200;

    free(resp);
    free(raw);

    close(sp.client_fd);
    close(sp.server_fd);

    return ok ? 0 : -1;
}