#ifndef SCRIPTS_H
#define SCRIPTS_H

typedef struct {
    const char* name;       // arg name
    const char* type;       // arg type (for frontend)
    const char* options[8]; // pre-defined options (for frontend)
} ScriptField;

typedef struct {
    const char* name;
    // const char* description;
    const char* command;
    ScriptField* fields;
} ScriptEntry;

#endif