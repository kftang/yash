#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"
#include "jobs.h"
#include "signals.h"

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
int _run_with_pipe(struct Command* command, int infd, int* pipeRead, int pidShell, bool sid) {
  char** commandAsString = _get_command_as_args_array(command);
  int pfd[2];
  pipe(pfd);
  int cpid = fork();
  if (cpid == 0) {
    if (sid) {
      setsid();
    }
    if (!command->backgrounded) {
      tcsetpgrp(STDIN_FILENO, pidShell); 
    }
    dup2(infd, STDIN_FILENO);
    dup2(pfd[1], STDOUT_FILENO);
    close(pfd[0]);
    execvp(commandAsString[0], commandAsString);
    exit(1);
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
  int cpidFirst = _run_with_pipe(input->commands[0], STDIN_FILENO, &lastPipeRead, getpid(), true);
  add_job(cpidFirst, input->originalInput);

  // Run commands in middle
  for (int i = 1; i < input->numCommands - 1; i++) {
    int pid = _run_with_pipe(input->commands[i], lastPipeRead, &lastPipeRead, getpid(), false);
    setpgid(pid, cpidFirst);
  }

  // Run last command
  struct Command* lastCommand = input->commands[input->numCommands - 1];
  char** lastCommandAsString = _get_command_as_args_array(lastCommand);
  int cpidLast = fork();
  if (cpidLast == 0) {
    dup2(lastPipeRead, STDIN_FILENO);
    execvp(lastCommandAsString[0], lastCommandAsString);
    exit(1);
  }
  setpgid(cpidLast, cpidFirst);
  close(lastPipeRead);

  // Wait if command not backgrounded
  if (!lastCommand->backgrounded) {
    int status;
    waitpid(cpidLast, &status, 0);
    if (WIFSIGNALED(status)) {
      if (WTERMSIG(status) == SIGTSTP) {
        printf("\n");
      }
    }
  }
  free(lastCommandAsString);
}

void run_single(struct Command* command) {
  int shellPID = getpid();
  char** commandAsString = _get_command_as_args_array(command);
  int cpid = fork();
  if (cpid == 0) {
    setsid();
    if (!command->backgrounded) {
      tcsetpgrp(STDIN_FILENO, shellPID); 
    }
    reset_handlers();
    execvp(commandAsString[0], commandAsString);
    exit(1);
  }
  free(commandAsString);
  if (!command->backgrounded) {
    int status;
    waitpid(cpid, &status, WUNTRACED);
    if (WIFSTOPPED(status)) {
      printf("yes");
    }
  }
}

