#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "request.h"

int read_http_headers(int client_fd, char* buffer, int max_size) {
    int total = 0;

    while (1) {
        int received = recv(client_fd, buffer + total, max_size - total, 0);

        if (received <= 0)
            return -1;

        total += received;

        buffer[total] = '\0';

        if (strstr(buffer, "\r\n\r\n"))
            break;
    }

    return total;
}
