#ifndef UPLOAD_HELPERS_H
#define UPLOAD_HELPERS_H

int build_upload_request(const char* filename, const char* file_content, int file_len, char** out_buf);
int do_upload(const char* filename, const char* content, int length);

#endif