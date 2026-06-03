#include "../test.h"

#include "fixtures.h"
#include "http_test_utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../src/api/files_api.h"
#include "../../src/api/upload_api.h"
#include "../../src/http/request.h"

static int build_upload_request(const char* filename, const char* file_content, int file_len, char** out_buf) {
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

void test_upload_and_download(void) {
    puts("file upload and download");

    test_setup();

    // uploading

    const char* content = "Hello, LAN server!"; // this represents only a very small file size

    char* raw = NULL;
    int raw_len = build_upload_request("hello.txt", content, strlen(content), &raw);

    TestSocketPair up;
    assert(make_test_socketpair(&up) == 0);

    send(up.client_fd, raw, raw_len, 0);
    shutdown(up.client_fd, SHUT_WR);

    HttpRequest req;
    assert(parse_http_request(raw, &req) == 0);

    handle_stream_upload(up.server_fd, &req, raw, raw_len);
    shutdown(up.server_fd, SHUT_WR);

    char* upload_resp = drain_response(up.client_fd, NULL);
    CHECK(parse_status(upload_resp) == 200, "upload returns 200");

    free(upload_resp);
    free(raw);

    close(up.client_fd);
    close(up.server_fd);

    // download

    const char* dl_raw = "GET /download/hello.txt HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "\r\n";

    TestSocketPair dl;
    assert(make_test_socketpair(&dl) == 0);

    send(dl.client_fd, dl_raw, strlen(dl_raw), 0);
    shutdown(dl.client_fd, SHUT_WR);

    HttpRequest dl_req;
    assert(parse_http_request(dl_raw, &dl_req) == 0);

    handle_download(dl.server_fd, dl_req.path);
    shutdown(dl.server_fd, SHUT_WR);

    int resp_len;
    char* dl_resp = drain_response(dl.client_fd, &resp_len);

    // assertions

    CHECK(parse_status(dl_resp) == 200, "download returns HTTP 200");
    CHECK(strstr(dl_resp, "Content-Disposition: attachment") != NULL, "download has attachment header");

    const char* body = response_body(dl_resp);
    CHECK(body != NULL, "download has body");

    if (body) {
        int body_len = resp_len - (int)(body - dl_resp);
        CHECK(body_len == (int)strlen(content), "downloaded size matches");
        CHECK(memcmp(body, content, strlen(content)) == 0, "downloaded content matches upload");
    }

    free(dl_resp);

    close(dl.client_fd);
    close(dl.server_fd);

    test_teardown();
}