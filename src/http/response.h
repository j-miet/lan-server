#ifndef RESPONSE_H
#define RESPONSE_H

typedef struct {
    long long start;
    long long end;
    int valid;
} HttpRange;

HttpRange parse_range(const char* range_header, long long file_size);
void send_response(int client_fd, int status_code, const char* status_text, const char* content_type, const char* body);
void send_text_response(int client_fd, int status_code, const char* status_text, const char* body);
void send_redirect(int client_fd, const char* location);
void send_response_with_cookie(int client_fd, const char* token);
void send_response_clear_cookie(int client_fd);
void send_file_stream(int client_fd, const char* path, const char* content_type, const char* range_header,
                      int force_download);

#endif