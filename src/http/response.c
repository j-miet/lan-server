#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "response.h"

void send_text_response(int client_fd, int status_code, const char* status_text, const char* body) {
    send_response(client_fd, status_code, status_text, "text/plain", body);
}

void send_response(int client_fd, int status_code, const char* status_text, const char* content_type,
                   const char* body) {
    char response[16384];

    int body_length = strlen(body);
    int response_length = snprintf(response, sizeof(response),
                                   "HTTP/1.1 %d %s\r\n"
                                   "Content-Type: %s\r\n"
                                   "Content-Length: %d\r\n"
                                   "\r\n"
                                   "%s",
                                   status_code, status_text, content_type, body_length, body);

    send(client_fd, response, response_length, 0);
}

void send_file_stream(int client_fd, const char* path, const char* content_type) {
    FILE* file = fopen(path, "rb");

    if (!file)
        return;

    struct stat st;

    if (stat(path, &st) != 0) {
        fclose(file);

        return;
    }

    long long file_size = st.st_size;

    // send headers
    char headers[1024];

    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lld\r\n"
             "Connection: close\r\n"
             "\r\n",
             content_type, file_size);

    send(client_fd, headers, strlen(headers), 0);

    // then send body in small chunks via streaming
    char buffer[8192];

    while (1) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);

        if (bytes_read == 0)
            break;

        size_t total_sent = 0;

        while (total_sent < bytes_read) {
            int sent = send(client_fd, buffer + total_sent, bytes_read - total_sent, 0);

            if (sent <= 0) {
                fclose(file);

                return;
            }

            total_sent += sent;
        }
    }

    fclose(file);
}