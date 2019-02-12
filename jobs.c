#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
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

void fg_job() {
  if (jobs->numJobs < 1) {
    return;
  }
  struct Job* lastJob = jobs->jobs[jobs->lastJob];
  printf("%s\n", lastJob->command);
  kill(-lastJob->pgid, SIGCONT);
  gpidRunning = lastJob->pgid;
  int status;
  waitpid(-lastJob->pgid, &status, WUNTRACED);
  tcsetpgrp(STDIN_FILENO, getpgrp());
  if (WIFSTOPPED(status)) {
    printf("\n");
  }
  if (WIFEXITED(status)) {
    remove_job(jobs->lastJob);
  }
}

void bg_job() {
  if (jobs->numJobs < 1) {
    return;
  }
  struct Job* lastJob = jobs->jobs[jobs->lastJob];
  printf("%s &\n", lastJob->command);
  kill(-lastJob->pgid, SIGCONT);
  set_job_status(jobs->lastJob, JOB_RUNNING);
}

void init_jobs(int maxJobs) {
  jobs->jobs = (struct Job**) malloc(sizeof(struct Job*) * maxJobs);
  for (int i = 0; i < maxJobs; i++) {
    jobs->jobs[i] = NULL;
  }
  jobs->maxJobs = maxJobs;
  jobs->numJobs = 0;
  jobs->highestJob = -1;
  jobs->lastJob = -1;
}

// Returns job number
int add_job(int pgid, char* input) {
  int addIndex = jobs->highestJob + 1;
  jobs->jobs[addIndex] = (struct Job*) malloc(sizeof(struct Job));
  struct Job* addedJob = jobs->jobs[addIndex];
  addedJob->pgid = pgid;
  addedJob->status = JOB_RUNNING;
  addedJob->command = input;
  jobs->numJobs++;
  jobs->highestJob++;
  jobs->lastJob = addIndex;
  return addIndex;
}

void remove_job(int jobNumber) {
  // Free the job being removed
  free(jobs->jobs[jobNumber]);
  jobs->jobs[jobNumber] = NULL;

  if (jobNumber == jobs->highestJob) {
    jobs->highestJob--;
  }

  for (int i = jobs->highestJob; i >= 0; i--) {
    if (jobs->jobs[i] != NULL) {
      jobs->lastJob = i;
      jobs->highestJob = i;
      break;
    }
  }

  // Decrement number of jobs
  jobs->numJobs--;
}

void update_jobs() {
  for (int i = 0; i < jobs->maxJobs; i++) { 
    if (jobs->jobs[i] != NULL) {
      int pgid = jobs->jobs[i]->pgid;
      int status;
      if (waitpid(-pgid, &status, WNOHANG | WUNTRACED) == pgid) {
        if (WIFSIGNALED(status)) {
          if (WTERMSIG(status) == SIGINT) {
            remove_job(jobs->lastJob);
          }
        }
        if (WIFEXITED(status)) {
          jobs->jobs[i]->status = JOB_DONE;
          jobDone = true;
        }
      }
    }
  }
}

void print_jobs() {
  for (int i = 0; i < jobs->maxJobs; i++) {
    struct Job* job = jobs->jobs[i];
    if (job != NULL) {
      char* statusString = _status(job);
      printf("[%d]%c\t%s\t\t%s\n", i + 1, i == jobs->lastJob ? '+' : '-', statusString, job->command);
      if (job->status == JOB_DONE) {
        remove_job(i);
      }
      free(statusString);
    }
  }
  jobDone = false;
}

void set_job_status(int jobNumber, int status) {
  // Find index of element to set status
  jobs->jobs[jobNumber]->status = status;
}

void free_jobs() {
  for (int i = 0; i < jobs->maxJobs; i++) {
    if (jobs->jobs[i] != NULL) {
      free(jobs->jobs[i]->command);
      free(jobs->jobs[i]);
    }
  }
  free(jobs->jobs);
}

