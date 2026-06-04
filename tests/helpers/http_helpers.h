#ifndef HTTP_TEST_UTILS_H
#define HTTP_TEST_UTILS_H

typedef struct {
    int client_fd;
    int server_fd;
} TestSocketPair;

int make_test_socketpair(TestSocketPair* sp);
char* drain_response(int fd, int* out_len);
int parse_status(const char* resp);
const char* response_body(const char* resp);

#endif