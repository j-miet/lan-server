#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../http/request.h"
#include "../http/response.h"
#include "../scripting/jobs.h"
#include "../scripting/scripts.h"
#include "../utils/json.h"
#include "../utils/json_builder.h"

#define ESCAPED_BUF (MAX_OUTPUT * 2)
#define JSON_OVERHEAD 64
#define JSON_BUF (ESCAPED_BUF + JSON_OVERHEAD)

// clang-format off

/**
 * ScriptFields are used for dynamic argument field generation in web ui interface. Each has 
 * - display name, 
 * - input type (text/select)
 * - descriptions (what this args does)
 * - pre-defined select field options. If type is 'text', this must be { NULL }
 * 
 * In general NULL is used for terminating line parsing in handle_scripts_api
 *
 * Example
 * - "input" is the arg name, "text" means free text input so no fields need to be defined, "Write something here" is
 *   the description, { NULL } as last field because type was "text"
 * - "message" is the arg name, "select" means dropdown select with options defined in fourth parameter, 
 *   "Pick a message" is the description, final parameter defines the select options which are "hello!" or "bye!". 
 *    Then per usual finish this option array with NULL terminator as well
 */
ScriptField script_test[] = {{"input", "text", "Write something here", 1, {NULL}},
                             {"message", "select", "Pick a message", 0, {"hello!", "bye!", NULL}},
                             {NULL}};


/**
 * All scripts must be added here in order to use them in web ui. Each has 
 * - name (button name + script title)
 * - path (script location relative to root dir)
 * - description (short info on what script does)
 * - ScriptField struct to auto-generate input fields for all args
 */
ScriptEntry scripts[] = {
    { "test", "./scripts/test.sh", "Test script", script_test},
    { NULL, NULL, NULL, NULL}
};

// clang-format on

/**
 * Create a json response from ScriptEntry struct
 */
void handle_scripts_api(int client_fd) {
    char json[16384];

    JsonBuilder jb;

    json_init(&jb, json, sizeof(json));

    json_append(&jb, "[");

    for (int i = 0; scripts[i].name != NULL; i++) {
        if (i > 0)
            json_append(&jb, ",");

        json_append(&jb, "{");

        json_append(&jb,
                    "\"name\":\"%s\","
                    "\"description\":\"%s\",",
                    scripts[i].name, scripts[i].description);

        json_append(&jb, "\"fields\":[");

        for (int j = 0; scripts[i].fields[j].name != NULL; j++) {
            if (j > 0)
                json_append(&jb, ",");

            ScriptField* field = &scripts[i].fields[j];

            json_append(&jb,
                        "{"
                        "\"name\":\"%s\","
                        "\"type\":\"%s\","
                        "\"description\":\"%s\","
                        "\"required\":\"%d\"",
                        field->name, field->type, field->description, field->required);

            // optional select options
            if (strcmp(field->type, "select") == 0) {
                json_append(&jb, ",\"options\":[");

                for (int k = 0; field->options[k] != NULL; k++) {
                    if (k > 0)
                        json_append(&jb, ",");

                    json_append(&jb, "\"%s\"", field->options[k]);
                }

                json_append(&jb, "]");
            }

            json_append(&jb, "}");
        }

        json_append(&jb, "]");
        json_append(&jb, "}");
    }

    json_append(&jb, "]");

    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Allocates a job for script, starts the job in a new thread and sends job id for status api access
 */
void handle_script_execute(int client_fd, HttpRequest* req) {
    char script_name[128] = "";

    json_get_string(req->body, "script", script_name, sizeof(script_name));

    if (strlen(script_name) == 0) {
        send_response(client_fd, 400, "Bad Request", "application/json", "{\"error\":\"Missing script name\"}");
        return;
    }

    Job* job = create_job();

    if (!job) {
        send_response(client_fd, 500, "Internal Server Error", "application/json",
                      "{\"error\":\"No job slots available\"}");
        return;
    }

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

    strcpy(job->script_name, script->name);
    job->status = JOB_RUNNING;
    job->output[0] = '\0';

    char command[512] = {0};
    snprintf(command, sizeof(command), "%s", script->command);

    if (req->body && strlen(req->body) > 0) {
        char arg[256];

        for (int i = 0; script->fields[i].name != NULL; i++) {
            ScriptField* field = &script->fields[i];
            memset(arg, 0, sizeof(arg)); // flush previous arg data

            json_get_string(req->body, field->name, arg, sizeof(arg));

            if (arg[0] != '\0') {
                strncat(command, " ", sizeof(command) - strlen(command) - 1);
                strncat(command, arg, sizeof(command) - strlen(command) - 1);
            }
        }
    } else {
        snprintf(command, sizeof(command), "%s", script->command);
    }

    start_job(job, command);

    char json[128];
    snprintf(json, sizeof(json), "{\"job_id\":%d}", job->id);

    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Request status of an existing job
 */
void handle_job_status(int client_fd, const char* path) {
    int id = atoi(path + strlen("/api/jobs/"));

    Job* job = find_job(id);

    if (!job) {
        send_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Job not found\"}");
        return;
    }

    const char* status = "running";

    if (job->status == JOB_COMPLETED)
        status = "completed";

    if (job->status == JOB_FAILED)
        status = "failed";

    // apply json escaping
    char escaped[ESCAPED_BUF]; // double the output size to make room for escapes characters
    json_escape(job->output, escaped, sizeof(escaped));

    char json[JSON_BUF]; // add json overhead
    snprintf(json, sizeof(json),
             "{"
             "\"id\":%d,"
             "\"status\":\"%s\","
             "\"exit_code\":%d,"
             "\"output_size\":\"%s\""
             "}",
             job->id, status, job->exit_code, escaped);

    send_response(client_fd, 200, "OK", "application/json", json);
}
