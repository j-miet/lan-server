#include "../http/request.h"

#ifndef SCRIPT_API_H
#define SCRIPT_API_H

void handle_scripts_api(int client_fd);
void handle_script_execute(int client_fd, HttpRequest* req);
void handle_job_status(int client_fd, const char* path);

#endif