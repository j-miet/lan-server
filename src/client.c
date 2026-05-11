#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "client.h"

void handle_client(int client_fd) {
    char buffer[4096]; // 4 * 1024 bytes = 4kb buffer, standard size for a memory page

    while (1) {
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // blocking until client input

        if (bytes <= 0)
            return;

        // wrap request data into struct
        Request req;
        req.length = bytes;
        memcpy(req.data, buffer, bytes);
        req.data[bytes] = '\0'; // in recv, leave a single byte for termination character

        printf("Received:\n%s\n", req.data);

        send(client_fd, req.data, req.length, 0);
    }

    printf("Client disconnected\n");
}