#include <signal.h>
#include <unistd.h>
#include "jobs.h"

void handle_sigint() {
  if (gpidRunning != 0) {
    kill(-gpidRunning, SIGINT);
    gpidRunning = 0;
  }
  return;
}

void handle_sigtstp() {
  if (gpidRunning != 0) {
    kill(-gpidRunning, SIGTSTP);
    gpidRunning = 0;
  }
  return;
}

void handle_sigttou() {
  tcsetpgrp(STDIN_FILENO, getpid());
}

void handle_sigchld() {
  update_jobs();
}

void handle_signal(int signal) {
  switch (signal) {
    case SIGINT:
      handle_sigint();
      break;
    case SIGTSTP:
      handle_sigtstp();
      break;
    case SIGTTOU:
      handle_sigttou();
      break;
    case SIGCHLD:
      handle_sigchld();
      break;
    default:
      break;
  }
}

void setup_handlers() {
  struct sigaction action;
  action.sa_handler = &handle_signal;
  action.sa_flags = SA_RESTART;
  
  sigemptyset(&(action.sa_mask));
  sigaddset(&(action.sa_mask), SIGINT);
  sigaddset(&(action.sa_mask), SIGCHLD);
  sigaddset(&(action.sa_mask), SIGTSTP);
  sigaddset(&(action.sa_mask), SIGTTOU);
  // sigaddset(&(action.sa_mask), SIGTTIN);

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGCHLD, &action, NULL);
  sigaction(SIGTSTP, &action, NULL);
  sigaction(SIGTTOU, &action, NULL);
  //sigaction(SIGTTIN, &action, NULL);
}

void reset_handlers() {
  struct sigaction action;
  action.sa_handler = SIG_DFL;
  action.sa_flags = SA_RESETHAND;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGCHLD, &action, NULL);
  sigaction(SIGTSTP, &action, NULL);
  sigaction(SIGTTOU, &action, NULL);
  sigaction(SIGTTIN, &action, NULL);
}

