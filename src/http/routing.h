#include "context.h"

#ifndef ROUTING_H
#define ROUTING_H

typedef void (*RouteHandler)(RequestContext* ctx); // route handler function type

typedef struct {
    const char* method; // GET, POST, DELETE or NULL for all
    const char* path;   // route path
    int is_prefix;      // 1 = starts with, 0 = exact match
    int auth_required;  // 1 = auth route, 0 = public route
    RouteHandler handler;
} Route;

void route_request(RequestContext* ctx);

#endif