#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"
#include "jobs.h"
#include "redir.h"

// Returns command as an args array for execv
char** _get_command_as_args_array(struct Command* command) {
  char** commandAsString = (char**) malloc(sizeof(char*) * (command->numArgs + 1));
  for (int i = 0; i < command->numArgs; i++) {
    commandAsString[i] = command->args[i]->tokenValue;
  }

  // Null terminate the args array
  commandAsString[command->numArgs] = NULL;
  return commandAsString;
}

// Runs command with pipe returning the cpid and sets piperead
int _run_with_pipe(struct Command* command, int infd, int* pipeRead, int pgid) {
  // Get command as args array
  char** commandAsString = _get_command_as_args_array(command);

  // Create pipe
  int pfd[2];
  pipe(pfd);

  // Fork and exec
  int cpid = fork();
  if (cpid == 0) {
    // Put child into process group specified
    setpgid(0, pgid);

    // Check if redirectors exist and deal with them
    // < redirector
    if (command->infd != STDIN_FILENO) {
      dup2(command->infd, STDIN_FILENO);
    } else {
      dup2(infd, STDIN_FILENO);
    }

    // > redirector
    if (command->outfd != STDOUT_FILENO) {
      dup2(command->outfd, STDOUT_FILENO);
    } else {
      dup2(pfd[1], STDOUT_FILENO);
    }

    // 2> redirector
    if (command->errfd != STDERR_FILENO) {
      dup2(command->errfd, STDERR_FILENO);
    }

    // Close read end of pipe, this child only writes
    close(pfd[0]);
    
    execvp(commandAsString[0], commandAsString);
    exit(1);
  }
  // Put child into process group specified
  setpgid(cpid, pgid);

  // Give terminal control to child process
  if (!command->backgrounded) {
    tcsetpgrp(STDIN_FILENO, cpid); 
  }

  // Free command as args list
  free(commandAsString);

  // Close write pipe
  close(pfd[1]);

  // Close file descriptors
  closeDescriptors(command);

  // Set the pipe read for use for the next exec
  *pipeRead = pfd[0];

  // Return child pid
  return cpid;
}

// Run input with pipelines
void run(struct ParsedInput* input) {
  // Stores last read descriptor for pipe
  int lastPipeRead;

  // Run first command in pipeline
  int cpidFirst = _run_with_pipe(input->commands[0], STDIN_FILENO, &lastPipeRead, 0);
  setpgid(cpidFirst, cpidFirst);
  gpidRunning = cpidFirst;

  // Run commands in middle of pipeline
  for (int i = 1; i < input->numCommands - 1; i++) {
    _run_with_pipe(input->commands[i], lastPipeRead, &lastPipeRead, cpidFirst);
  }

  // Run last command in pipeline
  struct Command* lastCommand = input->commands[input->numCommands - 1];
  char** lastCommandAsString = _get_command_as_args_array(lastCommand);
  int cpidLast = fork();
  if (cpidLast == 0) {
    // Put child into process group of first command
    setpgid(0, cpidFirst);

    // Pipe output of last command into stdin
    dup2(lastPipeRead, STDIN_FILENO);

    // Check for output redirector
    if (lastCommand->outfd != STDOUT_FILENO) {
      dup2(lastCommand->outfd, STDOUT_FILENO);
    }
    
    // Check for error redirector
    if (lastCommand->errfd != STDERR_FILENO) {
      dup2(lastCommand->errfd, STDERR_FILENO);
    }

    // Run command
    execvp(lastCommandAsString[0], lastCommandAsString);
    exit(1);
  }

  // Put child into process group of first command
  setpgid(cpidLast, cpidFirst);

  // Close the last pipe and all file descriptors
  close(lastPipeRead);
  closeDescriptors(lastCommand);

  // Give terminal access to process group if it is not backgrounded
  if (!lastCommand->backgrounded) {
    tcsetpgrp(STDIN_FILENO, cpidFirst); 
  }

  // Copy original input for printing back in job
  char* originalInput = input->originalInput;
  char* inputCopy = (char*) malloc(sizeof(char) * (strlen(originalInput) + 1));
  strcpy(inputCopy, originalInput);

  // Wait if command not backgrounded
  if (!lastCommand->backgrounded) {
    // Wait for entire pg to finish
    int status;
    while(waitpid(-cpidFirst, &status, WUNTRACED) > 0);
    
    // Get terminal back to shell
    tcsetpgrp(STDIN_FILENO, getpgrp());

    // Check if pg was stopped
    if (WIFSTOPPED(status)) {
      // Add job if process was stopped
      int jobId = add_job(cpidFirst, inputCopy);
      set_job_status(jobId, JOB_STOPPED);
      printf("\n");
    } else {
      free(inputCopy);
    }
  } else {
    // If running in background, add job immediately
    add_job(cpidFirst, inputCopy);
  }

  // Free the args array
  free(lastCommandAsString);
}

// Run input with no pipeline
void run_single(struct ParsedInput* input) {
  // Get first command
  struct Command* command = input->commands[0];

  // Get command as args array for execv
  char** commandAsString = _get_command_as_args_array(command);

  // Fork and exec
  int cpid = fork();
  if (cpid == 0) {
    // Put child in its own process group
    setpgid(0, 0);

    // Give terminal access to child if it is not backgrounded
    if (!command->backgrounded) {
      tcsetpgrp(STDIN_FILENO, getpgrp()); 
    }

    // Check for redirectors
    // < redirector
    if (command->infd != STDIN_FILENO) {
      dup2(command->infd, STDIN_FILENO);
    }

    // > redirector
    if (command->outfd != STDOUT_FILENO) {
      dup2(command->outfd, STDOUT_FILENO);
    }

    // 2> redirector
    if (command->errfd != STDERR_FILENO) {
      dup2(command->errfd, STDERR_FILENO);
    }
    execvp(commandAsString[0], commandAsString);
    exit(1);
  }
  // Put child in its own process group
  setpgid(cpid, 0);

  // Give terminal access to child if it is not backgrounded
  if (!command->backgrounded) {
    tcsetpgrp(STDIN_FILENO, cpid); 
  }

  // Close file descriptors
  closeDescriptors(command);

  // Copy input for job table
  char* originalInput = input->originalInput;
  char* inputCopy = (char*) malloc(sizeof(char) * (strlen(originalInput) + 1));
  strcpy(inputCopy, originalInput);

  // Set the running process so we know how to redirect signals
  gpidRunning = cpid;

  // If command is in foreground, wait for it to finish
  if (!command->backgrounded) {
    int status;
    waitpid(-cpid, &status, WUNTRACED);

    // Get terminal back to shell
    tcsetpgrp(STDIN_FILENO, getpgrp());

    // If child was stopped, add it as a job
    if (WIFSTOPPED(status)) {
      int jobId = add_job(cpid, inputCopy);
      set_job_status(jobId, JOB_STOPPED);
      printf("\n");
    } else {
      free(inputCopy);
    }
  } else {
    // If command was run in background, add it as job immediately
    add_job(cpid, inputCopy);
  }

  // Free args array
  free(commandAsString);
}

