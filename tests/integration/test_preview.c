#include "../helpers/http_helpers.h"
#include "../helpers/upload_helpers.h"
#include "../test.h"

#include "../../src/api/files_api.h"
#include "../../src/http/request.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char* build_preview_request(const char* filename, const char* range) {
    static char req[512];

    if (range) {
        snprintf(req, sizeof(req),
                 "GET /preview/%s HTTP/1.1\r\n"
                 "Host: localhost\r\n"
                 "Range: %s\r\n"
                 "\r\n",
                 filename, range);
    } else {
        snprintf(req, sizeof(req),
                 "GET /preview/%s HTTP/1.1\r\n"
                 "Host: localhost\r\n"
                 "\r\n",
                 filename);
    }

    return req;
}

static char* run_preview(const char* filename, const char* range, int* out_len) {
    const char* req = build_preview_request(filename, range);

    char path[256];
    snprintf(path, sizeof(path), "/preview/%s", filename);

    TestSocketPair sp;
    assert(make_test_socketpair(&sp) == 0);

    send(sp.client_fd, req, strlen(req), 0);
    shutdown(sp.client_fd, SHUT_WR);

    char buffer[16384];
    int rlen = recv(sp.server_fd, buffer, sizeof(buffer) - 1, 0);
    assert(rlen > 0);
    buffer[rlen] = '\0';

    HttpRequest hreq;
    assert(parse_http_request(buffer, rlen, &hreq) == 0);

    handle_preview(sp.server_fd, &hreq);
    shutdown(sp.server_fd, SHUT_WR);

    char* resp = drain_response(sp.client_fd, out_len);

    close(sp.client_fd);
    close(sp.server_fd);

    return resp;
}

void test_preview_full_file(void) {
    puts("preview full file");

    assert(do_upload("preview.txt", "ABCDEFGHIJ", 10) == 0);

    int len;
    char* resp = run_preview("preview.txt", NULL, &len);

    CHECK(parse_status(resp) == 200, "preview returns HTTP 200");
    CHECK(strstr(resp, "Accept-Ranges: bytes") != NULL, "Accept-Ranges header present");
    CHECK(strstr(resp, "Content-Disposition") == NULL, "preview is not attachment");

    const char* body = response_body(resp);
    CHECK(body != NULL, "response has body");

    if (body) {
        int body_len = len - (body - resp);

        CHECK(body_len == 10, "body length correct");
        CHECK(memcmp(body, "ABCDEFGHIJ", 10) == 0, "body content correct");
    }

    free(resp);
}

void test_preview_range(void) {
    puts("preview range request");

    int len;
    char* resp = run_preview("preview.txt", "bytes=2-5", &len);

    CHECK(parse_status(resp) == 206, "range request returns 206");
    CHECK(strstr(resp, "Content-Range: bytes 2-5/10") != NULL, "Content-Range correct");

    const char* body = response_body(resp);
    CHECK(body != NULL, "range response has body");

    if (body) {
        int body_len = len - (body - resp);

        CHECK(body_len == 4, "range body size correct");
        CHECK(memcmp(body, "CDEF", 4) == 0, "range body content correct");
    }

    free(resp);
}

void test_preview_path_traversal(void) {
    puts("preview path traversal");

    int len;
    char* resp = run_preview("../etc/passwd", NULL, &len);

    CHECK(parse_status(resp) == 403, "path traversal rejected");

    free(resp);
}