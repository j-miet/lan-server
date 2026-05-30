#include <time.h>

#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 64

typedef enum {
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED
} JobStatus;

typedef struct {
    int id;
    char script_name[64];
    JobStatus status;
    int exit_code;

    char* output;

    int output_size;
    int output_capacity;

    pthread_mutex_t lock;

    time_t completed_at;
} Job;

Job* create_job();
Job* find_job(int id);
void start_job(Job* job, const char* command);
void cleanup_jobs();

#endif