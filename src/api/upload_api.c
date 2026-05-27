#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "../http/multipart.h"
#include "../http/response.h"
#include "upload_api.h"

/**
 * Uploads a file to server via streaming
 */
void handle_stream_upload(int client_fd, HttpRequest* req, const char* headers, int header_size) {
    const char* body_start = strstr(headers, "\r\n\r\n");

    if (!body_start)
        return;

    body_start += 4;

    // how many upload bytes already in memory: this includes multipart headers + initial file bytes
    int already_read = header_size - (body_start - headers);
    char boundary[256];

    if (get_boundary(headers, boundary, sizeof(boundary)) < 0) {
        send_text_response(client_fd, 400, "Bad Request", "Missing boundary");
        return;
    }

    const char* file_data_start = strstr(body_start, "\r\n\r\n"); // find multipart header end

    if (!file_data_start) {
        send_text_response(client_fd, 400, "Bad Request", "Invalid multipart");
        return;
    }

    file_data_start += 4; // move over multipart header end to access raw file bytes

    char filename[256];
    const char* filename_start = strstr(body_start, "filename=\"");

    if (!filename_start)
        return;

    filename_start += 10;

    // extract filename
    const char* filename_end = strchr(filename_start, '"');

    if (!filename_end)
        return;

    int filename_length = filename_end - filename_start;
    memcpy(filename, filename_start, filename_length);
    filename[filename_length] = '\0';

    // output file
    char full_path[512];
    long long initial_file_bytes = header_size - (file_data_start - headers); // actual file bytes in memory

    snprintf(full_path, sizeof(full_path), "uploads/%s", filename);

    FILE* fp = fopen(full_path, "wb");

    if (!fp) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to open file");
        return;
    };

    fwrite(file_data_start, 1, initial_file_bytes, fp); // write initial bytes

    // then compute remaining bytes and use a recv loop to write into file
    long long remaining = req->content_length - already_read;
    char buffer[8192];

    while (remaining > 0) {
        int to_read = remaining < (long long)sizeof(buffer) ? (int)remaining : (int)sizeof(buffer);
        int received = recv(client_fd, buffer, to_read, 0);

        if (received <= 0)
            break;

        fwrite(buffer, 1, received, fp);

        remaining -= received;
    }

    fclose(fp);

    trim_multipart_footer(full_path, boundary); // remove multipart boundary from the end of body

    send_response(client_fd, 200, "OK", "application/json", "{\"success:\":true}");
}