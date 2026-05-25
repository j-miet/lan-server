#include "context.h"

#ifndef AUTH_H
#define AUTH_H

int authenticate_request(RequestContext* ctx);
int authenticate_login(RequestContext* ctx);

#endif