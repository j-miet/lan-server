#include "request.h"

#ifndef CONTEXT_H
#define CONTEXT_H

typedef struct {
    int client_fd;

    HttpRequest* req;

    const char* raw_headers;
    int raw_len;

    int authenticated;
} RequestContext;

#endif