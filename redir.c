#include <unistd.h>
#include "redir.h"

void closeDescriptors(struct Command* command) {
  if (command->infd != STDIN_FILENO) {
    close(command->infd);
  }
  if (command->outfd != STDOUT_FILENO) {
    close(command->outfd);
  }
  if (command->errfd != STDERR_FILENO) {
    close(command->errfd);
  }
}

