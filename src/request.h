#ifndef REQUEST_H
#define REQUEST_H

int read_http_headers(int client_fd, char* buffer, int max_size);

#endif