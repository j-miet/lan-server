#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include "client.h"
#include "server.h"
#include "socket.h"

void start_server(int port) {
    int server_fd = create_server_socket(port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        handle_client(client_fd);

        close(client_fd);
    }
}