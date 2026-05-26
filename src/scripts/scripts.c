#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../filesystem/json.h"
#include "../http/response.h"
#include "scripts.h"

#define OUTPUT_BUF 8192
#define ESCAPED_BUF (OUTPUT_BUF * 2)
#define JSON_OVERHEAD 64 // {"success":true,"exit_code":NNN,"output":""}
#define JSON_BUF (ESCAPED_BUF + JSON_OVERHEAD)

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

    FILE* pipe = popen(script->command, "r");

    if (!pipe) {
        send_response(client_fd, 500, "Internal Server Error", "application/json",
                      "{\"error\":\"Failed to execute script\"}");

        return;
    }

    char output[OUTPUT_BUF];
    output[0] = '\0';
    char line[512];

    while (fgets(line, sizeof(line), pipe)) {
        strncat(output, line, sizeof(output) - strlen(output) - 1);
    }

    int result = pclose(pipe);

    // apply json escaping
    char escaped[ESCAPED_BUF];
    json_escape(output, escaped, sizeof(escaped));

    char json[JSON_BUF];
    snprintf(json, sizeof(json),
             "{"
             "\"success\":true,"
             "\"exit_code\":%d,"
             "\"output\":\"%s\""
             "}",
             result, escaped);

    send_response(client_fd, 200, "OK", "application/json", json);
}