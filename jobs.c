#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "jobs.h"

char* _status(struct Job* job) {
  char* statusString = (char*) malloc(sizeof(char) * 8);
  switch(job->status) {
    case JOB_RUNNING:
      strcpy(statusString, "Running");
      break;
    case JOB_STOPPED:
      strcpy(statusString, "Stopped");
      break;
    case JOB_DONE:
      strcpy(statusString, "Done");
      break;
  }
  return statusString;
}

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
int add_job(int pgid, char* input) {
  jobs->jobs[jobs->numJobs] = (struct Job*) malloc(sizeof(struct Job));
  struct Job* addedJob = jobs->jobs[jobs->numJobs];
  addedJob->pgid = pgid;
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

void update_jobs() {
  for (int i = 0; i < jobs->numJobs; i++) { 
    int pgid = jobs->jobs[i]->pgid;
    int status;
    waitpid(pgid, &status, WNOHANG);
    if (WIFEXITED(status)) {
      jobs->jobs[i]->status = JOB_DONE;
    }
  }
}

void print_jobs() {
  for (int i = 0; i < jobs->numJobs; i++) {
    struct Job* job = jobs->jobs[i];
    char* statusString = _status(job);
    printf("[%d]%c\t%s\t\t%s\n", i + 1, i == jobs->numJobs - 1 ? '+' : '-', statusString, job->command);
    free(statusString);
  }
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

