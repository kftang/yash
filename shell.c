#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "signals.h"
#include "parser.h"
#include "runner.h"
#include "jobs.h"

int main() {
  // Assign space for jobs and input
  char* input = (char*) malloc(sizeof(char) * 2001);
  jobs = (struct JobsList*) malloc(sizeof(struct JobsList));
  init_jobs(20);
  setup_handlers();
  do {
    printf("# ");
    fgets(input, 2000, stdin);

    // If we get ^D then quit the terminal
    if (feof(stdin)) {
      break;
    }

    // If the error flag is set, don't parse input and skip line
    if (ferror(stdin)) {
      printf("\n");
      clearerr(stdin);
      continue;
    }

    // Get rid of newline from fgets
    input[strcspn(input, "\n")] = 0;

    // Parse input
    struct ParsedInput* parsedInput = parse_input(input);
    if (parsedInput != NULL) {
      /*
      for (int i = 0; i < parsedInput->numCommands; i++) {
        printf("Command %d\nArguments: ", i);
        struct Command* currentCommand = parsedInput->commands[i];
        for (int j = 0; j < currentCommand->numArgs; j++) {
          printf("%s, ", currentCommand->args[j]->tokenValue);
        }
        if (currentCommand->err != NULL) {
          printf("\nError File: %s", currentCommand->err->tokenValue);
        }
        if (currentCommand->out != NULL) {
          printf("\nOut File: %s", currentCommand->out->tokenValue);
        }
        if (currentCommand->in != NULL) {
          printf("\nInput File: %s", currentCommand->in->tokenValue);
        }
        if (currentCommand->backgrounded) {
          printf("\nBackgrounded");
        }
        printf("\n");
      }
      */
      if (parsedInput->numCommands > 1) {
        run(parsedInput);
      } else {
        run_single(parsedInput->commands[0]);
      }
      // Free the parsed input
      free_tokens(parsedInput);
    }
  } while (1); 

  // Free jobs and input
  free_jobs();
  free(jobs);
  free(input);

  return 0;
}


