#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int port;
    char token[256];
} ServerConfig;

extern ServerConfig g_config; // global configuration file

int load_config(const char* path);

#endif