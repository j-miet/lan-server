#include "context.h"

#ifndef ROUTING_H
#define ROUTING_H

typedef void (*RouteHandler)(RequestContext* ctx); // handler function type

typedef struct {
    const char* method; // GET, POST, DELETE or NULL
    const char* path;
    int prefix;        // 1 = starts with, 0 = exact match
    int auth_required; // 1 = auth route, 0 = public route
    RouteHandler handler;
} Route;

void routing_root(RequestContext* ctx);
void routing_hello(RequestContext* ctx);
void routing_login(RequestContext* ctx);
void routing_logout(RequestContext* ctx);
void routing_index(RequestContext* ctx);
void routing_download(RequestContext* ctx);
void routing_files(RequestContext* ctx);
void routing_upload(RequestContext* ctx);
void routing_delete(RequestContext* ctx);
void routing_script_execute(RequestContext* ctx);

void route_request(RequestContext* ctx);

#endif