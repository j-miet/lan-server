#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../http/response.h"
#include "jobs.h"

typedef struct {
    Job* job;
    char command[256];
} JobThreadArgs;

Job jobs[MAX_JOBS];
int next_job_id = 1;

// pthread_create requires that both the input value and return type must be void*

static void* job_worker(void* arg) {
    JobThreadArgs* args = (JobThreadArgs*)arg;

    Job* job = args->job;

    FILE* pipe = popen(args->command, "r");

    if (!pipe) {
        job->status = JOB_FAILED;

        free(args);

        return NULL;
    }

    char line[512];

    while (fgets(line, sizeof(line), pipe)) {
        pthread_mutex_lock(&job->lock);

        int space = MAX_OUTPUT - job->output_size - 1;

        int len = strlen(line);
        if (len > space)
            len = space;

        memcpy(job->output + job->output_size, line, len);

        job->output_size += len;
        job->output[job->output_size] = '\0';

        pthread_mutex_unlock(&job->lock);
    }

    int result = pclose(pipe);

    pthread_mutex_lock(&job->lock);

    job->exit_code = result;
    job->status = result == 0 ? JOB_COMPLETED : JOB_FAILED;

    pthread_mutex_unlock(&job->lock);

    free(args);

    return NULL;
}

/**
 * Create a new job
 */
Job* create_job() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].id == 0) {
            jobs[i].id = next_job_id++;
            jobs[i].output_size = 0;
            jobs[i].output[0] = '\0';

            pthread_mutex_init(&jobs[i].lock, NULL);

            return &jobs[i];
        }
    }

    return NULL;
}

/**
 * Find an existing job by its id
 */
Job* find_job(int id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].id == id)
            return &jobs[i];
    }

    return NULL;
}

/**
 * Start a job via threading
 */
void start_job(Job* job, const char* command) {
    job->status = JOB_RUNNING;
    job->output[0] = '\0';

    JobThreadArgs* args = malloc(sizeof(JobThreadArgs));

    args->job = job;
    strcpy(args->command, command);

    pthread_t thread;
    pthread_create(&thread, NULL, job_worker, args);
    pthread_detach(thread);
}
