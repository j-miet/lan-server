#ifndef SCRIPTS_H
#define SCRIPTS_H

typedef struct {
    const char* name;
    const char* command;
} ScriptEntry;

extern ScriptEntry scripts[];

#endif