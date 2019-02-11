#include "parser.h"

#define JOB_RUNNING 0
#define JOB_STOPPED 1
#define JOB_DONE 2

struct Job {
  int firstPid;
  int status;
  char* command;
};

struct JobsList {
  struct Job** jobs;
  int lastJob;
  int maxJobs;
  int numJobs;
};

struct JobsList* jobs;

void init_jobs(int maxJobs);
int add_job(int pid, char* jobString);
void remove_job(int pid);
void set_job_status(int pid, int status);
void free_jobs();

