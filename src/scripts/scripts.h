#ifndef SCRIPTS_H
#define SCRIPTS_H

typedef struct {
    const char* name;
    const char* command;
} ScriptEntry;

extern ScriptEntry scripts[];

void handle_script_execute(int client_fd, const char* path);

#endif