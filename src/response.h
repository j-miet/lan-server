#ifndef RESPONSE_H
#define RESPONSE_H

void send_text_response(int client_fd, int status_code, const char* status_text, const char* body);
void send_response(int client_fd, int status_code, const char* status_text, const char* content_type, const char* body);
void send_file_stream(int client_fd, const char* path, const char* content_type);

#endif