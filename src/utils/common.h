#ifndef COMMON_H
#define COMMON_H

const char* get_client_ip(int fd);
void url_decode(char* dest, const char* src);
int get_header(const char* raw, const char* key, char* out, int size);

#endif