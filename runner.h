#include "parser.h"

struct FDPair {
  int oldfd;
  int newfd;
};

void run_single(struct ParsedInput* input);
void run(struct ParsedInput* input);

