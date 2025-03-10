#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenize_lib.h"

int main(int argc, char **argv) {
  char input[256];
  char *tokens[256];
  char quoted_tokens[256];
  fgets(input, sizeof(input), stdin);
  input[strlen(input)] = '\0'; 

  tokenize(input, tokens, quoted_tokens);
  
  print_tokens(tokens);

  //free_token_mem(tokens);

  return 0;
}
