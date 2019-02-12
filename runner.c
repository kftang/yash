#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "parser.h"
#include "jobs.h"
#include "signals.h"
#include "redir.h"

char** _get_command_as_args_array(struct Command* command) {
  char** commandAsString = (char**) malloc(sizeof(char*) * (command->numArgs + 1));
  for (int i = 0; i < command->numArgs; i++) {
    commandAsString[i] = command->args[i]->tokenValue;
  }

  // Null terminate the args array
  commandAsString[command->numArgs] = NULL;
  return commandAsString;
}

void _close_pipes(int* pipes, int numPipes) {
  for (int i = 0; i < numPipes * 2; i++) {
    close(pipes[i]);
  }
}

// Runs command with pipe returning the cpid and outfd
int _run_with_pipe(struct Command* command, int infd, int* pipeRead, int pgid, bool sid) {
  char** commandAsString = _get_command_as_args_array(command);
  int pfd[2];
  pipe(pfd);
  int cpid = fork();
  if (cpid == 0) {
    if (sid) {
      setsid();
    } 
    setpgid(0, pgid);
    if (command->infd != STDIN_FILENO) {
      dup2(command->infd, STDIN_FILENO);
    } else {
      dup2(infd, STDIN_FILENO);
    }
    if (command->outfd != STDOUT_FILENO) {
      dup2(command->outfd, STDOUT_FILENO);
    } else {
      dup2(pfd[1], STDOUT_FILENO);
    }
    if (command->errfd != STDERR_FILENO) {
      dup2(command->errfd, STDERR_FILENO);
    }
    close(pfd[0]);
    closeDescriptors(command);
    execvp(commandAsString[0], commandAsString);
    exit(1);
  }
  if (!command->backgrounded) {
    tcsetpgrp(STDIN_FILENO, cpid); 
  }
  free(commandAsString);
  close(pfd[1]);
  *pipeRead = pfd[0];
  return cpid;
}

void run(struct ParsedInput* input) {
  // Stores last read descriptor for pipe
  int lastPipeRead;

  // Run first command
  int cpidFirst = _run_with_pipe(input->commands[0], STDIN_FILENO, &lastPipeRead, 0, true);
  setpgid(cpidFirst, cpidFirst);
  gpidRunning = cpidFirst;

  // Run commands in middle
  for (int i = 1; i < input->numCommands - 1; i++) {
    int pid = _run_with_pipe(input->commands[i], lastPipeRead, &lastPipeRead, cpidFirst, false);
    setpgid(pid, cpidFirst);
  }

  // Run last command
  struct Command* lastCommand = input->commands[input->numCommands - 1];
  char** lastCommandAsString = _get_command_as_args_array(lastCommand);
  int cpidLast = fork();
  if (cpidLast == 0) {
    setpgid(0, cpidFirst);
    dup2(lastPipeRead, STDIN_FILENO);
    if (lastCommand->outfd != STDOUT_FILENO) {
      dup2(lastCommand->outfd, STDOUT_FILENO);
    }
    if (lastCommand->errfd != STDERR_FILENO) {
      dup2(lastCommand->errfd, STDERR_FILENO);
    }
    execvp(lastCommandAsString[0], lastCommandAsString);
    exit(1);
  }
  setpgid(cpidLast, cpidFirst);
  close(lastPipeRead);
  closeDescriptors(lastCommand);

  // Give terminal access to process group
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
    while(waitpid(-cpidFirst, &status, WUNTRACED) == 0);
    
    // Get terminal back to shell
    tcsetpgrp(STDIN_FILENO, getpgrp());

    // Check if pg was stopped
    if (WIFSTOPPED(status)) {
      int jobId = add_job(cpidFirst, inputCopy);
      set_job_status(jobId, JOB_STOPPED);
      printf("\n");
    }
  } else {
    add_job(cpidFirst, inputCopy);
  }
  free(lastCommandAsString);
}

void run_single(struct ParsedInput* input) {
  struct Command* command = input->commands[0];
  char** commandAsString = _get_command_as_args_array(command);
  int cpid = fork();
  if (cpid == 0) {
    setsid();
    setpgid(0, 0);
    if (!command->backgrounded) {
      tcsetpgrp(STDIN_FILENO, getpgrp()); 
    }
    if (command->infd != STDIN_FILENO) {
      dup2(command->infd, STDIN_FILENO);
    }
    if (command->outfd != STDOUT_FILENO) {
      dup2(command->outfd, STDOUT_FILENO);
    }
    if (command->errfd != STDERR_FILENO) {
      dup2(command->errfd, STDERR_FILENO);
    }
    execvp(commandAsString[0], commandAsString);
    exit(1);
  }
  setpgid(cpid, 0);
  if (!command->backgrounded) {
    tcsetpgrp(STDIN_FILENO, cpid); 
  }
  closeDescriptors(command);

  char* originalInput = input->originalInput;
  char* inputCopy = (char*) malloc(sizeof(char) * (strlen(originalInput) + 1));
  strcpy(inputCopy, originalInput);
  gpidRunning = cpid;
  if (!command->backgrounded) {
    int status;
    waitpid(-cpid, &status, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, getpgrp());
    if (WIFSTOPPED(status)) {
      int jobId = add_job(cpid, inputCopy);
      set_job_status(jobId, JOB_STOPPED);
      printf("\n");
    }
  } else {
    add_job(cpid, inputCopy);
  }
  free(commandAsString);
}


