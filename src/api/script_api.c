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
    JsonBuilder jb;
    if (!json_init(&jb, 4096)) {
        send_text_response(client_fd, 500, "Internal Server Error", "Out of memory");
        return;
    }

    json_append(&jb, "[");

    for (int i = 0; scripts[i].name != NULL; i++) {
        if (i > 0)
            json_append(&jb, ",");

        json_append(&jb,
                    "{"
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

        json_append(&jb, "]}");
    }

    json_append(&jb, "]");

    send_response(client_fd, 200, "OK", "application/json", jb.buffer);

    free(jb.buffer);
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

    char* argv[32];
    int argc = 0;

    argv[argc++] = (char*)script->command; // argv[0] = path to executable

    // arguments
    for (int i = 0; script->fields[i].name != NULL; i++) {
        ScriptField* field = &script->fields[i];

        char arg[256] = {0};

        json_get_string(req->body, field->name, arg, sizeof(arg));

        if (arg[0] != '\0') {
            argv[argc++] = strdup(arg); // create copies to heap; otherwise arg pointers are wiped after this loop
        }
    }

    argv[argc] = NULL;

    start_job(job, argv);

    // free temporary strdup allocations after start_job has made its own duplicates
    for (int i = 1; i < argc; i++) {
        free(argv[i]);
    }

    char json[128];
    snprintf(json, sizeof(json), "{\"job_id\":%d}", job->id);

    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Request status of an existing job
 */
void handle_job_status(int client_fd, const char* path) {
    int id = strtol(path + strlen("/api/jobs/status/"), NULL, 10);

    Job* job = find_job(id);
    if (!job) {
        send_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Job not found\"}");
        return;
    }

    const char* status = "running";

    pthread_mutex_lock(&job->lock);

    if (job->status == JOB_COMPLETED)
        status = "completed";

    if (job->status == JOB_FAILED)
        status = "failed";

    char json[256];

    snprintf(json, sizeof(json),
             "{"
             "\"id\":%d,"
             "\"status\":\"%s\","
             "\"exit_code\":%d,"
             "\"output_size\":%d"
             "}",
             job->id, status, job->exit_code, job->output_size);

    pthread_mutex_unlock(&job->lock);

    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Request output data of an existing job
 */
void handle_job_output(int client_fd, const char* path) {
    int id = 0;
    int offset = 0;

    sscanf(path, "/api/jobs/output/%d?offset=%d", &id, &offset);

    Job* job = find_job(id);
    if (!job) {
        send_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Job not found\"}");
        return;
    }

    pthread_mutex_lock(&job->lock);

    if (offset > job->output_size)
        offset = job->output_size;

    int chunk_size = job->output_size - offset;
    const char* chunk = job->output + offset;

    char* escaped = malloc(chunk_size * 2 + 1);
    if (!escaped) {
        pthread_mutex_unlock(&job->lock);
        send_response(client_fd, 500, "Internal Server Error", "application/json",
                      "{\"error\":\"Memory allocation failed\"}");
        return;
    }

    json_escape(chunk, escaped, chunk_size * 2 + 1);

    pthread_mutex_unlock(&job->lock);

    int json_size = strlen(escaped) + 128;
    char* json = malloc(json_size);
    if (!json) {
        free(escaped);
        send_response(client_fd, 500, "Internal Server Error", "application/json",
                      "{\"error\":\"Memory allocation failed\"}");
        return;
    }

    snprintf(json, json_size,
             "{\"data\":\"%s\","
             "\"next_offset\":%d"
             "}",
             escaped, job->output_size);

    send_response(client_fd, 200, "OK", "application/json", json);

    free(escaped);
    free(json);
}
