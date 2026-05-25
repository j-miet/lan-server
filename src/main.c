#include "config.h"
#include "http/auth.h"
#include "server.h"

int main() {
    if (load_config("config/server.conf") < 0)
        return -1;

    start_server(g_config.port);

    return 0;
}