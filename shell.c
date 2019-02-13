#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "signals.h"
#include "parser.h"
#include "runner.h"
#include "jobs.h"

int main() {
  // Assign space for jobs and input
  char* input = (char*) malloc(sizeof(char) * 2001);
  jobs = (struct JobsList*) malloc(sizeof(struct JobsList));
  init_jobs(20);
  gpidRunning = 0;
  setup_handlers();
  jobDone = false;
  do {
    if (jobDone) {
      print_jobs();
      jobDone = false;
    }
    printf("# ");
    fgets(input, 2000, stdin);

    // If we get ^D then quit the terminal
    if (feof(stdin)) {
      break;
    }

    // If the error flag is set, don't parse input and skip line
    if (ferror(stdin)) {
      clearerr(stdin);
      continue;
    }

    // Get rid of newline from fgets
    input[strcspn(input, "\n")] = 0;

    // Check input for job control commands
    if (strcmp(input, "jobs") == 0) {
      print_jobs();
    } else if (strcmp(input, "fg") == 0) {
      fg_job();
    } else if (strcmp(input, "bg") == 0) {
      bg_job();
    } else {

      // Parse input
      struct ParsedInput* parsedInput = parse_input(input);
      if (parsedInput != NULL) {
        if (parsedInput->numCommands > 1) {
          run(parsedInput);
        } else {
          run_single(parsedInput);
        }
        // Free the parsed input
        free_tokens(parsedInput);
      }
    }
  } while (1); 

  // Free jobs and input
  free_jobs();
  free(jobs);
  free(input);

  return 0;
}


