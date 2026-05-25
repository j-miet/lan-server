#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "context.h"

static const char* find_authorization_header(const char* headers) {
    const char* auth = strstr(headers, "Authorization");

    if (!auth)
        return NULL;

    return auth;
}

int authenticate_request(RequestContext* ctx) {
    const char* auth = find_authorization_header(ctx->raw_headers);

    if (!auth)
        return 0;

    const char* bearer = strstr(auth, "Bearer "); // find bearer token

    if (!bearer)
        return 0;

    bearer += 7;

    // token extraction
    char token[256];
    int i = 0;

    while (bearer[i] && bearer[i] != '\r' && bearer[i] != '\n' && i < (int)sizeof(token) - 1) {
        token[i] = bearer[i];
        i++;
    }

    token[i] = '\0';

    // token comparison
    if (strcmp(token, g_config.token) == 0) {
        ctx->authenticated = 1;
        return 1;
    }

    return 0;
}