#include "parser.h"

struct FDPair {
  int oldfd;
  int newfd;
};

void run_single(struct Command* command);
void run(struct ParsedInput* input);

