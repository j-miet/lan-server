#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 64
#define MAX_OUTPUT 65536

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
    char output[MAX_OUTPUT];
} Job;

Job* create_job();
Job* find_job(int id);
void start_job(Job* job, const char* command);

#endif