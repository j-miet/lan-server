#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "common.h"

/**
 * Get client ip using their socket file descriptor
 */
const char* get_client_ip(int fd) {
    static char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;

    socklen_t len = sizeof(addr);

    if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0)
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    else
        strncpy(ip, "unknown", sizeof(ip));

    return ip;
}

/**
 * Perform url decoding on source buffer and passes decoded data into dest
 */
void url_decode(char* dest, const char* src) {
    while (*src) {
        // decode hexadecimals %XX e.g. empty space %20
        if (*src == '%' && src[1] != '\0' && src[2] != '\0' && isxdigit((unsigned char)src[1]) &&
            isxdigit((unsigned char)src[2])) {
            unsigned int value;
            sscanf(src + 1, "%2x", &value);

            *dest++ = (char)value;

            src += 3;
        } else if (*src == '+') { // + is also space
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }

    *dest = '\0';
}

/**
 * Get a specific header from raw header string
 * This will not work for parsing header field values!
 * Return 0 for success, -1 for failure
 */
int get_header(const char* raw, const char* key, char* out, int size) {
    size_t key_len = strlen(key);
    const char* p = raw;

    while ((p = strstr(p, key)) != NULL) {
        // must be start of line
        if (p != raw && p[-1] != '\n') {
            p += key_len;
            continue;
        }

        // must be exact key match followed by ':'
        if (strncmp(p, key, key_len) != 0 || p[key_len] != ':') {
            p += key_len;
            continue;
        }

        p += key_len;

        if (*p == ':')
            p++;
        if (*p == ' ')
            p++;

        int i = 0;
        while (*p && *p != '\r' && *p != '\n' && i < size - 1) {
            out[i++] = *p++;
        }

        out[i] = '\0';

        return 0;
    }

    return -1;
}