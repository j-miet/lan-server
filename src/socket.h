#ifndef SOCKET_H
#define SOCKET_H

int create_server_socket(int port);
int socket_recv(int client_fd, char* buffer, int size);
int socket_send(int client_fd, const char* buffer, int size);

#endif