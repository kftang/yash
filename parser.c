#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "parser.h"

// Count number of tokens by looking at # of spaces
int _count_tokens(char* str) {
  int count = str[0] == 0 ? 0 : 1;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == ' ') {
      count++;
    }
  }
  return count;
}

// Returns type of a token from a string
int _get_token_type(char* tokenPtr) { 
  int tokenLength = strlen(tokenPtr);
  // Most tokens are length 1
  if (tokenLength == 1) {
    switch (tokenPtr[0]) {
      case '<':
        return TOKEN_REDIR_STDIN;
        break;
      case '>':
        return TOKEN_REDIR_STDOUT;
        break;
      case '|':
        return TOKEN_PIPE;
        break;
      case '&':
        return TOKEN_BACKGROUND;
        break;
      default:
        return TOKEN_WORD;
    }
  // Redirection to stderr is length 2 :angryreact:
  } else if (tokenLength == 2 && strcmp(tokenPtr, "2>") == 0) {
    return TOKEN_REDIR_STDERR;
  }

  // Default is word
  return TOKEN_WORD;
}

// Returns true if the next token is valid given the current
bool _next_token_valid(int curToken, int nextToken, int* usedTokens) {
  switch (curToken) {
    case TOKEN_BEGINNING:
      if (nextToken != TOKEN_WORD) {
        return false;
      }
    case TOKEN_WORD:
      return true;
    case TOKEN_REDIR_STDOUT:
      if (*usedTokens & 0x01 || nextToken != TOKEN_WORD) {
        return false;
      }
      *usedTokens |= 0x01;
      break;
    case TOKEN_REDIR_STDIN:
      if (*usedTokens & 0x02 || nextToken != TOKEN_WORD) {
        return false;
      }
      *usedTokens |= 0x02;
      break;
    case TOKEN_REDIR_STDERR:
      if (*usedTokens & 0x04 || nextToken != TOKEN_WORD) {
        return false;
      }
      *usedTokens |= 0x04;
      break;
    case TOKEN_PIPE:
      if (nextToken != TOKEN_WORD) {
        return false;
      }
      *usedTokens = 0;
      break;
    case TOKEN_BACKGROUND:
      return false;
  }
  return true;
}

// Parse the tokens in a command into the executable, args, any redirs, and bg
void _parse_command(struct Command* command) {
  // Initialize values
  command->in = NULL;
  command->out = NULL;
  command->err = NULL;
  command->backgrounded = false;
  // Num of args includes executable
  int numArgs = 1;

  // Executable is a word
  int lastTokenType = TOKEN_WORD;

  // Skip executable
  for (int i = 1; i < command->numTokens; i++) {
    // Get current token type to determine if its an arg or ...
    int currentTokenType = command->tokens[i]->tokenType;
    if (currentTokenType == TOKEN_WORD && lastTokenType == TOKEN_WORD) {
      numArgs++;
    } else if (lastTokenType == TOKEN_REDIR_STDIN) {
      command->in = command->tokens[i];
    } else if (lastTokenType == TOKEN_REDIR_STDOUT) {
      command->out = command->tokens[i];
    } else if (lastTokenType == TOKEN_REDIR_STDERR) {
      command->err = command->tokens[i];
    } else if (currentTokenType == TOKEN_BACKGROUND) {
      command->backgrounded = true;
    }
    lastTokenType = currentTokenType;
  }

  // Reset last token type for looping over again to find args
  lastTokenType = TOKEN_WORD;

  // Set number of arguments
  command->numArgs = numArgs;

  // Allocate memory for args array
  command->args = (struct Token**) malloc(sizeof(struct Token*) * numArgs);
  
  // Add token pointers for the arguments
  int argsIterator = 0;
  for (int i = 0; i < command->numTokens; i++) {
    struct Token* currentToken = command->tokens[i];
    int currentTokenType = currentToken->tokenType;
    if (currentTokenType == TOKEN_WORD && lastTokenType == TOKEN_WORD) {
      command->args[argsIterator] = currentToken;
      argsIterator++;
    }
    lastTokenType = currentTokenType;
  }
}

// Parse tokens into commands
struct Command** _parse_tokens(struct Token** tokens, int numTokens, int numCommands) {
  // Create commands array
  struct Command** commands = (struct Command**) malloc(sizeof(struct Command*) * numCommands);
  
  // Store last pipe location to calculate length of command
  // This value is actually equal to the index right after the pipe
  int lastPipe = 0;

  // Loop through # commands
  for (int i = 0; i < numCommands; i++) {
    // Create command struct
    struct Command* currentCommand = (struct Command*) malloc(sizeof(struct Command));
    
    // The current command starts at the last pipe
    int commandStart = lastPipe;
    int commandLength;
    
    // Loop to find where the next pipe is (or last word)
    for (int j = lastPipe; ; j++) {
      // Found next pipe
      if (tokens[j]->tokenType == TOKEN_PIPE) {
        commandLength = j - commandStart;
        lastPipe = j + 1;
        break;
      }

      // Reach null terminator
      if (j == numTokens - 1) {
        commandLength = j + 1 - commandStart;
        break;
      }
    }

    // This should never happen, if we reach null terminator but not on the last command
    if (lastPipe == commandStart && i != numCommands - 1) {
      exit(42);
    }

    // Set number of tokens for the command to the command length
    currentCommand->numTokens = commandLength;

    // Allocate space for the tokens array inside the command struct
    currentCommand->tokens = (struct Token**) malloc(sizeof(struct Token*) * commandLength);

    // Point the tokens in the command to the tokens from the parsed input
    for (int j = commandStart; j < commandStart + commandLength; j++) {
      currentCommand->tokens[j - commandStart] = tokens[j];
    }

    // Parse command to get the arguments, redirectors and backgrounder
    _parse_command(currentCommand);

    // Set the command in the commands array
    commands[i] = currentCommand;
  }
  return commands;
}

void _free_tokens(struct Token** tokens, int numTokens) {
  for (int i = 0; i < numTokens; i++) {
    free(tokens[i]->tokenValue);
    free(tokens[i]);
  }
  free(tokens);
}

void free_tokens(struct ParsedInput* parsedInput) {
  _free_tokens(parsedInput->tokens, parsedInput->numTokens);
  for (int i = 0; i < parsedInput->numCommands; i++) {
    free(parsedInput->commands[i]->tokens);
    free(parsedInput->commands[i]->args);
    free(parsedInput->commands[i]);
  }
  free(parsedInput->originalInput);
  free(parsedInput->commands);
  free(parsedInput);
}

struct ParsedInput* parse_input(char* input) {
  // Count tokens to allocate for array of tokens
  int numTokens = _count_tokens(input);
  
  // Count pipes to make it easier for execs
  int numPipes = 0;
  // Create array of tokens
  struct Token** tokens = (struct Token**) malloc(sizeof(struct Token*) * numTokens);

  int lastTokenType = TOKEN_BEGINNING;
  int usedTokens = 0;

  int inputLength = strlen(input);
  char* inputCopy = (char*) malloc(sizeof(char) * (inputLength + 1));
  strcpy(inputCopy, input);

  // Get first token from strtok
  char* tokenPtr = strtok(input, " ");
  int i = 0;
  while (tokenPtr != NULL && i < numTokens) {
    // Create token struct
    struct Token* currentToken = (struct Token*) malloc(sizeof(struct Token));

    // Copy token value into struct
    int tokenLength = strlen(tokenPtr);
    currentToken->tokenValue = (char*) malloc(sizeof(char) * tokenLength + 1);
    strcpy(currentToken->tokenValue, tokenPtr);

    // Get token type
    int curTokenType = _get_token_type(tokenPtr);
    if (!_next_token_valid(lastTokenType, curTokenType, &usedTokens)) {
      _free_tokens(tokens, i);
      return NULL;
    }

    if (curTokenType == TOKEN_PIPE) {
      numPipes++;
    }

    // Set token type
    currentToken->tokenType = curTokenType; 

    // Assign token to list
    tokens[i] = currentToken;

    i++;

    // Advance tokenPtr
    tokenPtr = strtok(NULL, " ");

    // Set last token type to check validity of input
    lastTokenType = curTokenType;
  }

  // Last token can only be a word or ampersand
  if (lastTokenType != TOKEN_WORD && lastTokenType != TOKEN_BACKGROUND) {
    _free_tokens(tokens, numTokens);
    return NULL;
  }

  // Create return struct
  struct ParsedInput* parsedInput = (struct ParsedInput*) malloc(sizeof(struct ParsedInput));
  parsedInput->originalInput = inputCopy;
  parsedInput->tokens = tokens;
  parsedInput->numTokens = numTokens;

  // There are 1 + num pipes commands
  parsedInput->numCommands = numPipes + 1;

  // Parse tokens into commands
  parsedInput->commands = _parse_tokens(tokens, numTokens, numPipes + 1);
  return parsedInput;
}

