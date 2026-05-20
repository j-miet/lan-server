#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "response.h"

void send_text_response(int client_fd, int status_code, const char* status_text, const char* body) {
    char response[4096];

    int body_length = strlen(body);

    int response_length = snprintf(response, sizeof(response),
                                   "HTTP/1.1 %d %s\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: %d\r\n"
                                   "\r\n"
                                   "%s",
                                   status_code, status_text, body_length, body);

    send(client_fd, response, response_length, 0);
}

void send_file_response(int client_fd, const char* content_type, const char* data, int size) {
    char header[1024];

    int header_length = snprintf(header, sizeof(header),
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %d\r\n"
                                 "\r\n",
                                 content_type, size);

    // for static files: send header and body separately because body might contain binary data
    send(client_fd, header, header_length, 0);
    send(client_fd, data, size, 0);
}