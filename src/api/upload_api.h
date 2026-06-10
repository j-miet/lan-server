#include "../http/request.h"

#ifndef UPLOAD_API_H
#define UPLOAD_API_H

void handle_stream_upload(int client_fd, HttpRequest* req);

#endif
