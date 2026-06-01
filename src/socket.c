#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "socket.h"

/**
 * Create a server socket and return its file descriptor
 */
int create_server_socket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    // allows bind to re-use local addresses, avoiding 'accept: Bad file descriptor' error
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return -1;
    }

    printf("Listening on port %d\n", port);

    return server_fd;
}