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

// global lock for creating jobs
pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;

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

        int len = strlen(line);

        // grow buffer dynamically for larger print outputs
        while (job->output_size + len + 1 > job->output_capacity) {
            job->output_capacity *= 2;

            char* new_buf = realloc(job->output, job->output_capacity);

            if (!new_buf) {
                pthread_mutex_unlock(&job->lock);

                pclose(pipe);

                job->status = JOB_FAILED;

                free(args);

                job->completed_at = time(NULL);

                return NULL;
            }

            job->output = new_buf;
        }

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

    job->completed_at = time(NULL);

    return NULL;
}

static void destroy_job(Job* job) {
    pthread_mutex_lock(&jobs_mutex);

    if (!job)
        return;

    free(job->output);

    pthread_mutex_destroy(&job->lock);

    memset(job, 0, sizeof(Job));

    pthread_mutex_unlock(&jobs_mutex);
}

/**
 * Create a new job
 */
Job* create_job() {
    pthread_mutex_lock(&jobs_mutex);

    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].id == 0) {
            jobs[i].output_capacity = 8192;
            jobs[i].output_size = 0;

            jobs[i].output = malloc(jobs[i].output_capacity);
            if (!jobs[i].output) {
                pthread_mutex_unlock(&jobs_mutex);
                return NULL;
            }

            pthread_mutex_init(&jobs[i].lock, NULL);

            jobs[i].output[0] = '\0';
            jobs[i].id = next_job_id++;

            pthread_mutex_unlock(&jobs_mutex);

            return &jobs[i];
        }
    }

    pthread_mutex_unlock(&jobs_mutex);

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

/**
 * Properly dispose of job structs
 */
void cleanup_jobs() {
    time_t now = time(NULL);

    for (int i = 0; i < MAX_JOBS; i++) {
        Job* job = &jobs[i];

        if (job->id == 0)
            continue;

        if (job->status == JOB_RUNNING)
            continue;

        // remove jobs that are finished and older than 5 minutes
        if (now - job->completed_at > 300) {
            destroy_job(job);
        }
    }
}
