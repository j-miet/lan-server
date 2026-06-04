#include "../helpers/fixtures.h"
#include "../helpers/http_helpers.h"
#include "../helpers/upload_helpers.h"
#include "../test.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../src/api/files_api.h"
#include "../../src/api/upload_api.h"
#include "../../src/http/request.h"

static char* run_download(const char* req, int* len) {
    TestSocketPair sp;
    assert(make_test_socketpair(&sp) == 0);

    send(sp.client_fd, req, strlen(req), 0);
    shutdown(sp.client_fd, SHUT_WR);

    HttpRequest parsed;
    assert(parse_http_request(req, &parsed) == 0);

    handle_download(sp.server_fd, parsed.path);

    shutdown(sp.server_fd, SHUT_WR);

    char* resp = drain_response(sp.client_fd, len);

    close(sp.client_fd);
    close(sp.server_fd);

    return resp;
}

void test_upload_and_download(void) {
    puts("file upload and download");

    // uploading

    const char* content = "Hello, LAN server!"; // this represents only a very small file size
    assert(do_upload("hello.txt", content, strlen(content)) == 0);

    // download

    const char* dl_req = "GET /download/hello.txt HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "\r\n";
    int resp_len;

    char* dl_resp = run_download(dl_req, &resp_len);

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
}