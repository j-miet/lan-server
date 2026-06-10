#include <string.h>

#include "../config.h"
#include "../http/context.h"

static const char* find_cookie_header(const char* headers) {
    const char* cookie = strstr(headers, "Cookie:");
    if (!cookie)
        return NULL;

    return cookie;
}

/**
 * Verifies a request includes valid auth token in cookies
 * Returns 1 on success, 0 on false authentication
 */
int authenticate_request(RequestContext* ctx) {
    const char* cookie = find_cookie_header(ctx->raw_headers);
    if (!cookie)
        return 0;

    const char* t = strstr(cookie, "token=");
    if (!t)
        return 0;

    t += 6;

    // token extraction
    char token[256];
    int i = 0;

    // cookies also include other fields such as HttpOnly and Path, which are separated with semicolons
    while (t[i] && t[i] != ';' && t[i] != '\r' && t[i] != '\n' && i < (int)sizeof(token) - 1) {
        token[i] = t[i];
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

/**
 * Verifies a login attempt with auth token
 * Returns 0 on success, non-zero on differing auth tokens
 */
int authenticate_login(RequestContext* ctx) {
    if (!ctx->req->body)
        return 0;

    char token[256];
    int i = 0;
    const char* t = ctx->req->body;

    while (t[i] && t[i] != ';' && t[i] != '\r' && t[i] != '\n' && i < (int)sizeof(token) - 1) {
        token[i] = t[i];
        i++;
    }

    token[i] = '\0';

    return strcmp(token, g_config.token) == 0;
}