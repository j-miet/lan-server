#define _FILE_OFFSET_BITS 64

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "response.h"

/**
 * Parse range bytes from range headers
 */
HttpRange parse_range(const char* range_header, long long file_size) {
    HttpRange r = {0, file_size - 1, 0};

    if (!range_header)
        return r;

    // expected format is "bytes=START-END"
    if (strncmp(range_header, "bytes=", 6) != 0)
        return r;

    const char* p = range_header + 6;

    char buf[128];
    strncpy(buf, p, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* dash = strrchr(buf, '-');
    if (!dash)
        return r;

    *dash = '\0'; // replace '-' with null terminator in order to split range to start and end

    const char* start_str = buf;    // read until '\0'
    const char* end_str = dash + 1; // continue after '\0' until end

    // handle suffix range "bytes=-END" which translates to last END bytes
    if (strlen(start_str) == 0) {
        if (strlen(end_str) == 0) // "bytes=-" is invalid
            return r;

        long long suffix = strtoll(end_str, NULL, 10);
        if (suffix <= 0)
            return r;

        r.start = file_size - suffix;
        if (r.start < 0)
            r.start = 0;

        r.end = file_size - 1;
        r.valid = 1;

        return r;
    }

    r.start = strtoll(start_str, NULL, 10);

    if (strlen(end_str) > 0)
        r.end = strtoll(end_str, NULL, 10);
    else
        r.end = file_size - 1;

    // apply clamping
    if (r.start < 0)
        r.start = 0;
    if (r.end >= file_size)
        r.end = file_size - 1;
    if (r.start > r.end)
        return r;

    r.valid = 1;
    return r;
}

/**
 * Send a general http response
 */
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

/**
 * Send a text response
 */
void send_text_response(int client_fd, int status_code, const char* status_text, const char* body) {
    send_response(client_fd, status_code, status_text, "text/plain", body);
}

/**
 * Send a redirecting response (302)
 */
void send_redirect(int client_fd, const char* location) {
    char response[512];

    int response_length = snprintf(response, sizeof(response),
                                   "HTTP/1.1 302 Found\r\n"
                                   "Location: %s\r\n"
                                   "Content-Length: 0\r\n"
                                   "\r\n",
                                   location);

    send(client_fd, response, response_length, 0);
}

/**
 * Send a json response with cookie data
 */
void send_response_with_cookie(int client_fd, const char* token) {
    char response[1024];

    int response_length = snprintf(response, sizeof(response),
                                   "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Set-Cookie: token=%s; HttpOnly; Path=/\r\n"
                                   "Content-Length: 2\r\n"
                                   "\r\n"
                                   "{}",
                                   token);

    send(client_fd, response, response_length, 0);
}

/**
 * Send response with cleared (empty) cookie data
 */
void send_response_clear_cookie(int client_fd) {
    char response[512];
    int response_length = snprintf(response, sizeof(response),
                                   "HTTP/1.1 200 OK\r\n"
                                   "Set-Cookie: token=; HttpOnly; Path=/; Max-Age=0\r\n"
                                   "Content-Length: 0\r\n"
                                   "\r\n");

    send(client_fd, response, response_length, 0);
}

/**
 * Send a file to client via streaming
 */
void send_file_stream(int client_fd, const char* path, const char* content_type, const char* range_header,
                      int force_download) {
    FILE* file = fopen(path, "rb");
    if (!file)
        return;

    struct stat st;
    if (stat(path, &st) != 0) {
        fclose(file);
        return;
    }

    long long file_size = st.st_size;
    HttpRange range = parse_range(range_header, file_size);

    long long start = range.valid ? range.start : 0;
    long long end = range.valid ? range.end : file_size - 1;
    long long chunk_size = end - start + 1; // range includes end points i.e. [start, end]

    // move file pointer to start of range
    fseeko(file, start, SEEK_SET);

    // send headers
    char headers[1024];
    int len;
    if (range.valid) {
        len = snprintf(headers, sizeof(headers),
                       "HTTP/1.1 206 Partial Content\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %lld\r\n"
                       "Content-Range: bytes %lld-%lld/%lld\r\n"
                       "Accept-Ranges: bytes\r\n"
                       "\r\n",
                       content_type, chunk_size, start, end, file_size);
    } else {
        len = snprintf(headers, sizeof(headers),
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %lld\r\n"
                       "Accept-Ranges: bytes\r\n"
                       "%s"
                       "\r\n",
                       content_type, file_size, force_download ? "Content-Disposition: attachment\r\n" : "");
    }

    if (send(client_fd, headers, len, 0) <= 0) {
        fclose(file);
        return;
    }

    // then send body in small chunks via streaming, but only the requested range
    char buffer[8192];
    long long remaining = chunk_size;
    size_t to_read = sizeof(buffer);

    while (remaining > 0) {
        if ((long long)to_read > remaining)
            to_read = remaining;

        size_t bytes_read = fread(buffer, 1, to_read, file);
        if (bytes_read == 0)
            break;

        size_t total_sent = 0;

        while (total_sent < bytes_read) {
            // prevent SIGPIPE which ensures even the largest files can be previewed in browser
            int sent = send(client_fd, buffer + total_sent, bytes_read - total_sent, MSG_NOSIGNAL);

            // if client disconnected OR broken pipe error
            if (sent <= 0) {
                fclose(file);
                return;
            }

            total_sent += sent;
        }

        remaining -= bytes_read;
    }

    fclose(file);
}