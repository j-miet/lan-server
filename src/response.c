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