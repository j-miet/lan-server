#ifndef FILES_API_H
#define FILES_API_H

void handle_files_api(int client_fd);
void handle_download(int client_fd, const char* path);
void handle_preview(int client_fd, const char* path);
void handle_delete_file(int client_fd, const char* path);
void serve_static_file(int client_fd, const char* path);
void serve_text(int client_fd, const char* msg);

#endif