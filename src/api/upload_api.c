#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "../http/multipart.h"
#include "../http/response.h"
#include "upload_api.h"

// tracks how many uploads this session. Used for producing unique temp files
int upload_counter = 0;

static const char* extract_first_file_data(const char* body, const char* boundary) {
    if (!body || !boundary)
        return NULL;

    char delim[512];
    snprintf(delim, sizeof(delim), "--%s", boundary); // construct boundary string

    const char* p = strstr(body, delim); // find first boundary
    if (!p)
        return NULL;

    p = strstr(p, "\r\n"); // move past boundary line
    if (!p)
        return NULL;
    p += 2;

    // skip multipart headers (Content-Disposition, etc.)
    const char* header_end = strstr(p, "\r\n\r\n");
    if (!header_end)
        return NULL;

    return header_end + 4; // return pointer to start of raw file data
}

/**
 * Uploads a file to server via streaming
 */
void handle_stream_upload(int client_fd, HttpRequest* req) {
    const char* body_start = req->body;
    if (!body_start)
        return;

    long long body_start_offset = req->body - req->raw;

    // how many upload bytes already in memory: includes request headers + initial bytes (e.g. some multipart bytes)
    long long already_read = req->raw_len - body_start_offset;

    char boundary[256];
    if (get_boundary(req->raw, boundary, sizeof(boundary)) < 0) {
        send_text_response(client_fd, 400, "Bad Request", "Missing boundary");
        return;
    }

    char end_marker[512];
    snprintf(end_marker, sizeof(end_marker), "\r\n--%s--", boundary);

    // find multipart header end and move pointer to start of raw data
    const char* file_data_start = extract_first_file_data(req->body, boundary);
    if (!file_data_start) {
        send_text_response(client_fd, 400, "Bad Request", "Invalid multipart");
        return;
    }

    // find filename header field
    const char* part_header = body_start;

    part_header = strstr(part_header, "Content-Disposition");
    const char* filename_start = strstr(part_header, "filename=\"");
    if (!filename_start)
        return;

    filename_start += 10;

    // extract filename
    const char* filename_end = strchr(filename_start, '"');
    if (!filename_end)
        return;

    int filename_length = filename_end - filename_start;
    char filename[256];

    memcpy(filename, filename_start, filename_length);
    filename[filename_length] = '\0';

    // output: temp and actual files
    char full_path[512];
    char temp_path[512];

    snprintf(full_path, sizeof(full_path), "uploads/%s", filename);
    snprintf(temp_path, sizeof(temp_path), "uploads/%s.uploading.%d", filename, upload_counter);
    upload_counter++;

    FILE* fp = fopen(temp_path, "wb");
    if (!fp) {
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to open file");
        return;
    };

    long long file_start_offset = file_data_start - req->raw;
    long long initial_file_bytes = req->raw_len - file_start_offset; // amount of actual file bytes in memory

    // a special case where buffer cuts of header end marker is also handled by carrying marker_len-1 amount of bytes
    // over to next loop iteration.
    // why marker_len-1: because marker_len would mean the entire end marker was already consumed
    char carry[512];
    int carry_len = 0;
    int marker_len = strlen(end_marker);

    // write initial bytes into file. This carries
    if (initial_file_bytes > 0) {
        char* boundary_pos = memmem(file_data_start, initial_file_bytes, end_marker, strlen(end_marker));

        if (boundary_pos) {
            initial_file_bytes = boundary_pos - file_data_start;
        } else {
            initial_file_bytes -= marker_len - 1;
            carry_len = marker_len - 1;
            memcpy(carry, file_data_start + initial_file_bytes, carry_len);
        }

        if (initial_file_bytes > 0)
            fwrite(file_data_start, 1, initial_file_bytes, fp);
    }

    // then compute remaining bytes and use a recv loop to write into file
    long long remaining = req->content_length - already_read;
    char buffer[8192];

    while (remaining > 0) {
        int to_read = remaining < (long long)sizeof(buffer) ? (int)remaining : (int)sizeof(buffer);

        int received = recv(client_fd, buffer, to_read, 0);
        if (received <= 0) {
            fclose(fp);
            remove(temp_path);
            return;
        }

        // prepend carry bytes from previous iteration
        char search_buf[sizeof(buffer) + sizeof(carry)];

        // write carry-over bytes and then append received to the end
        memcpy(search_buf, carry, carry_len);
        memcpy(search_buf + carry_len, buffer, received);

        int search_len = carry_len + received;

        // detect final boundary inside stream
        char* boundary_pos = memmem(search_buf, search_len, end_marker, marker_len);

        if (boundary_pos) {
            if (carry_len > 0)
                fwrite(carry, 1, carry_len, fp);

            long valid_bytes = boundary_pos - search_buf - carry_len;
            if (valid_bytes > 0)
                fwrite(buffer, 1, valid_bytes, fp);

            break;
        }

        // write previous carry bytes + newly received data, then repeat carry over on last (marker_len-1) bytes
        if (carry_len > 0)
            fwrite(carry, 1, carry_len, fp);

        int safe_bytes = received - (marker_len - 1);
        if (safe_bytes > 0)
            fwrite(buffer, 1, safe_bytes, fp);

        carry_len = received < (marker_len - 1) ? received : (marker_len - 1);
        memcpy(carry, buffer + received - carry_len, carry_len);

        remaining -= received;
    }

    fclose(fp);

    if (rename(temp_path, full_path) != 0) {
        remove(temp_path);
        send_text_response(client_fd, 500, "Internal Server Error", "Failed to finalize upload");
        return;
    }

    send_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
}