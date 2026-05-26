#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../http/response.h"
#include "scripts.h"

ScriptEntry scripts[] = {{"test", "./scripts/test.sh"}, {NULL, NULL}};

void handle_script_execute(int client_fd, const char* path) {

    const char* script_name = path + strlen("/api/scripts/");

    ScriptEntry* script = NULL;

    for (int i = 0; scripts[i].name != NULL; i++) {
        if (strcmp(script_name, scripts[i].name) == 0) {
            script = &scripts[i];
            break;
        }
    }

    if (!script) {
        send_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Script not found\"}");

        return;
    }

    int result = system(script->command);

    char json[256];
    snprintf(json, sizeof(json), "{\"success\":true,\"exit_code\":%d}", result);

    send_response(client_fd, 200, "OK", "application/json", json);
}