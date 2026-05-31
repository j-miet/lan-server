#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "server.h"
#include "socket.h"

static void* client_thread(void* arg) {
    int client_fd = *(int*)arg;

    free(arg); // free the passed fd pointer

    handle_client(client_fd);

    close(client_fd);

    return NULL;
}

/**
 * Create a server socket which starts accepting client connections
 */
void start_server(int port) {
    int server_fd = create_server_socket(port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // must declare fd pointer here, otherwise &client_fd in pthread_create gets overridden after each accept call
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;

        pthread_t thread;

        pthread_create(&thread, NULL, client_thread, fd_ptr);
        pthread_detach(thread);
    }
}