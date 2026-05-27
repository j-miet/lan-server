#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

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