#ifndef RESPONSE_H
#define RESPONSE_H

void send_text_response(int client_fd, int status_code, const char* status_text, const char* body);

#endif