#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

ServerConfig g_config;

static void trim_newline(char* str) {
    str[strcspn(str, "\r\n")] = '\0';
}

/**
 * Loads server configurations
 * Returns 0 on success, -1 on failure
 */
int load_config(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Failed to open config: %s\n", path);
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);

        if (strncmp(line, "PORT=", 5) == 0) {
            g_config.port = strtol(line + 5, NULL, 10);
        } else if (strncmp(line, "TOKEN=", 6) == 0) {
            strncpy(g_config.token, line + 6, sizeof(g_config.token) - 1);

            g_config.token[sizeof(g_config.token) - 1] = '\0';
        }
    }

    fclose(file);

    if (g_config.port == 0) {
        printf("Invalid port in config\n");
        return -1;
    }

    if (strlen(g_config.token) == 0) {
        printf("Missing token in config\n");
        return -1;
    }

    return 0;
}