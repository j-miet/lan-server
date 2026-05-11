#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"

void handle_client(int client_fd) {
    char buffer[4096]; // 4 * 1024 bytes = 4kb buffer, standard size for a memory page

    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // blocking until client input

    if (bytes <= 0)
        return;

    buffer[bytes] = '\0'; // in recv, leave a single byte for termination character

    printf("Received:\n%s\n", buffer);

    send(client_fd, buffer, bytes, 0);
}