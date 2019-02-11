#include <stdlib.h>
#include "jobs.h"

void init_jobs(int maxJobs) {
  jobs->jobs = (struct Job**) malloc(sizeof(struct Job*) * maxJobs);
  for (int i = 0; i < maxJobs; i++) {
    jobs->jobs[i] = NULL;
  }
  jobs->lastJob = 0;
  jobs->maxJobs = maxJobs;
  jobs->numJobs = 0;
}

// Returns job number
int add_job(int pid, char* input) {
  jobs->jobs[jobs->numJobs] = (struct Job*) malloc(sizeof(struct Job));
  struct Job* addedJob = jobs->jobs[jobs->numJobs];
  addedJob->firstPid = pid;
  addedJob->status = JOB_RUNNING;
  addedJob->command = input;
  jobs->numJobs++;
  return jobs->numJobs - 1;
}

void remove_job(int jobNumber) {
  // Free the job being removed
  free(jobs->jobs[jobNumber]);

  // Shift elements down one
  for (int i = jobNumber; i < jobs->numJobs - 1; i++) {
    jobs->jobs[i] = jobs->jobs[i + 1];
  }

  // Decrement number of jobs
  jobs->numJobs--;
}

void set_job_status(int jobNumber, int status) {
  // Find index of element to set status
  jobs->jobs[jobNumber]->status = status;
}

void free_jobs() {
  for (int i = 0; i < jobs->numJobs; i++) {
    if (jobs->jobs[i] != NULL) {
      free(jobs->jobs[i]);
    }
  }
  free(jobs->jobs);
}

