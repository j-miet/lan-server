#ifndef SCRIPT_API_H
#define SCRIPT_API_H

void handle_script_execute(int client_fd, const char* path);
void handle_job_status(int client_fd, const char* path);

#endif