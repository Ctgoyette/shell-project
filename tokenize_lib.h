#ifndef TOKENIZE_H
#define TOKENIZE_H

int is_special(char ch);

int tokenize(char *str_in, char **token_list, char *quoted_tokens);

void print_tokens(char **tokens);

void free_token_mem(char **tokens);

#endif
