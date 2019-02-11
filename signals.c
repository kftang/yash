#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handle_sigint() {
  printf("test1 %d\n", getpid());
  return;
}

void handle_sigtstp() {
  printf("test2 %d\n", getpid());
  return;
}

void handle_signal(int signal) {
  switch (signal) {
    case SIGINT:
      handle_sigint();
      break;
    case SIGTSTP:
      handle_sigtstp();
      break;
    default:
      break;
  }
}

void setup_handlers() {
  struct sigaction action;
  action.sa_handler = &handle_signal;
  action.sa_flags = 0;
  
  sigemptyset(&(action.sa_mask));
  sigaddset(&(action.sa_mask), SIGINT);
  sigaddset(&(action.sa_mask), SIGTSTP);

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTSTP, &action, NULL);
}

void reset_handlers() {
  struct sigaction action;
  action.sa_handler = SIG_DFL;
  action.sa_flags = SA_RESETHAND;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTSTP, &action, NULL);
}

