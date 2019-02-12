#include <stdbool.h>

#ifndef PARSER_H
#define PARSER_H

#define TOKEN_BEGINNING 0
#define TOKEN_WORD 1
#define TOKEN_PIPE 2
#define TOKEN_BACKGROUND 3
#define TOKEN_REDIR_STDOUT 4
#define TOKEN_REDIR_STDERR 5
#define TOKEN_REDIR_STDIN 6

struct ParsedInput {
  char* originalInput;
  struct Command** commands;
  struct Token** tokens;
  int numTokens;
  int numCommands;
};

struct Command {
  int numTokens;
  int numArgs;
  struct Token** tokens;
  struct Token** args;
  int outfd;
  int errfd;
  int infd;
  bool backgrounded;
};

struct Token {
  int tokenType;
  char* tokenValue;
};

void free_tokens(struct ParsedInput* parsedInput);
struct ParsedInput* parse_input(char* input);
bool next_token_valid(int curToken, int nextToken);

#endif

