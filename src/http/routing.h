#include "request.h"

#ifndef ROUTING_H
#define ROUTING_H

void route_request(int client_fd, HttpRequest* req);

#endif