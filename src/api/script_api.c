#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../filesystem/json.h"
#include "../http/response.h"
#include "../scripting/jobs.h"
#include "../scripting/scripts.h"

#define ESCAPED_BUF (MAX_OUTPUT * 2)
#define JSON_OVERHEAD 64
#define JSON_BUF (ESCAPED_BUF + JSON_OVERHEAD)

// list all scripts here
ScriptEntry scripts[] = {{"test", "./scripts/test.sh"}, {NULL, NULL}};

void handle_script_execute(int client_fd, const char* path) {
    const char* script_name = path + strlen("/api/scripts/");

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

    start_job(job, script->command);

    char json[128];
    snprintf(json, sizeof(json), "{\"job_id\":%d}", job->id);

    send_response(client_fd, 200, "OK", "application/json", json);
}

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
             "\"output\":\"%s\""
             "}",
             job->id, status, job->exit_code, escaped);

    send_response(client_fd, 200, "OK", "application/json", json);
}
