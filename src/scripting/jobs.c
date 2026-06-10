#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../http/response.h"
#include "jobs.h"

typedef struct {
    Job* job;
    char* argv[32];
} JobThreadArgs;

Job jobs[MAX_JOBS];
int next_job_id = 1;

// global lock for creating jobs
pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;

// pthread_create requires that both the input value and return type must be void*

static void* job_worker(void* arg) {
    JobThreadArgs* args = (JobThreadArgs*)arg;
    Job* job = args->job;

    int pipefd[2];

    if (pipe(pipefd) == -1) {
        job->status = JOB_FAILED;

        free(args);

        return NULL;
    }

    // this creates a copy of parent process and return process id 0 on success
    // importantly both will share the same pipe buffer: child redirect stdout to itself -> parent reads from same pipe
    pid_t pid = fork();

    if (pid == -1) {
        job->status = JOB_FAILED;

        close(pipefd[0]);
        close(pipefd[1]);

        free(args);

        return NULL;
    }

    if (pid == 0) {
        // child process

        // close output and redirect input to stdout
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[1]);

        // then replace child process with script execution. If successful, the parent (= server) proceeds as usual
        execvp(args->argv[0], args->argv);

        // if execvp fails:
        // 1. display error message
        // 2. properly terminate child: otherwise
        perror("execvp failed");
        exit(1);
    }

    // parent process

    close(pipefd[1]); // close input/write file descriptor for parent, it only needs to read script output

    FILE* pipe_stream = fdopen(pipefd[0], "r"); // child and parent point to same pipe: read script output from index 0

    if (!pipe_stream) {
        close(pipefd[0]);

        job->status = JOB_FAILED;

        free(args);

        return NULL;
    }

    char line[512];

    while (fgets(line, sizeof(line), pipe_stream)) {
        pthread_mutex_lock(&job->lock);

        int len = strlen(line);

        // grow buffer dynamically for larger print outputs
        while (job->output_size + len + 1 > job->output_capacity) {
            job->output_capacity *= 2;

            char* new_buf = realloc(job->output, job->output_capacity);
            if (!new_buf) {
                pthread_mutex_unlock(&job->lock);

                fclose(pipe_stream);
                free(args);

                job->status = JOB_FAILED;
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

    fclose(pipe_stream);

    int status;
    waitpid(pid, &status, 0); // properly clean up child process

    pthread_mutex_lock(&job->lock);

    if (WIFEXITED(status)) {
        job->exit_code = WEXITSTATUS(status);
        job->status = (job->exit_code == 0) ? JOB_COMPLETED : JOB_FAILED;
    } else {
        job->exit_code = -1;
        job->status = JOB_FAILED;
    }

    pthread_mutex_unlock(&job->lock);

    // cleanup argv strdup allocations
    for (int i = 0; args->argv[i] != NULL; i++) {
        free(args->argv[i]);
    }

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
Job* create_job(void) {
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

            int ret = pthread_mutex_init(&jobs[i].lock, NULL);
            if (ret != 0) {
                pthread_mutex_unlock(&jobs_mutex);
                free(jobs[i].output);
                return NULL;
            }

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
void start_job(Job* job, char* argv[]) {
    job->status = JOB_RUNNING;
    job->output[0] = '\0';

    JobThreadArgs* args = malloc(sizeof(JobThreadArgs));
    if (!args) {
        job->status = JOB_FAILED;
        return;
    }

    args->job = job;
    int i = 0;

    while (argv[i] != NULL && i < 31) {
        args->argv[i] = strdup(argv[i]); // pointer duplication to transfer ownership
        i++;
    }

    args->argv[i] = NULL;

    pthread_t thread;
    pthread_create(&thread, NULL, job_worker, args);
    pthread_detach(thread);
}

/**
 * Properly dispose of finished jobs
 */
void cleanup_jobs(void) {
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
