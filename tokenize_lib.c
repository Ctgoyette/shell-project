#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenize_lib.h"

/**
 * Determines if a character is a special character. This function checks for
 * the following characters: '(', ')', '<', '>', ';', '|', and '\t'.
 * 
 * Inputs:
 * char ch - Character to check
 * 
 * Return:
 * 1 if the character is a special character, 0 otherwise
 */
int is_special(char ch) {
  return (ch == '(' || 
	  ch == ')' ||
	  ch == '<' ||
	  ch == '>' ||
	  ch == ';' ||
	  ch == '|' ||
	  ch == '\t') ? 1 : 0;
}

/**
 * Splits the input string into an array of tokens, handling special characters,
 * whitespace, and quoted text.
 * 
 * Inputs:
 * char* str_in - Input string to tokenize
 * char** token_list - Array of strings to store tokens
 * char* quoted_tokens - Array of flags where an element is set to 1 if the
 *                       corresponding token was encased in quotes
 * 
 * Return:
 * The number of tokens extracted from the input string
 */
int tokenize(char *str_in, char **token_list, char *quoted_tokens) {
  int token_index = 0;

  if (str_in[strlen(str_in) - 1] == '\n') {
    str_in[strlen(str_in) - 1] = '\0';
  }
  
  int i = 0;
  while (str_in[i] != '\0') {

    if (str_in[i] == ' ') {
      i++;
    }

    else if (str_in[i] == '"') {
      i++;
      int quote_start = i;
      int quote_len = 0;

      while (str_in[i] != '"' && str_in[i] != '\0') {
        i++;
      }

      quote_len = i - quote_start;
      token_list[token_index] = (char*)malloc((quote_len * sizeof(char)) + 1);
      strncpy(token_list[token_index], &str_in[quote_start], quote_len);
      token_list[token_index][quote_len] = '\0';
      quoted_tokens[token_index] = 1;
      token_index++;
      i++;
    }

    else if (is_special(str_in[i]) == 1) {
      token_list[token_index] = (char*)malloc(2*sizeof(char));
      token_list[token_index][0] = str_in[i];
      token_list[token_index][1] = '\0';
      token_index++;
      i++;
    }

    else {
      int word_start = i;
      int word_len = 0;

      while (is_special(str_in[i]) != 1 && str_in[i] != ' ' && str_in[i] != '\0') {
	      i++;
      }

      word_len = i - word_start;
      token_list[token_index] = (char*)malloc((word_len * sizeof(char)) + 1);
      strncpy(token_list[token_index], &str_in[word_start], word_len);
      token_list[token_index][word_len] ='\0';
      token_index++;
    }
  }

  token_list[token_index] = NULL;
  
  return token_index;
}

/**
 * Prints each token in the provided list of tokens on a new line.
 * 
 * Inputs:
 * char** tokens - Array of tokens to be printed
 */
void print_tokens(char **tokens) {
  int i = 0;
  while (tokens[i] != NULL) {
    printf("%s\n", tokens[i]);
    i++;
  }
}

/**
 * Frees the memory allocated for the tokens in the provided list of tokens.
 * 
 * Inputs:
 * char** tokens - Array of tokens whose memory will be freed
 */
void free_token_mem(char **tokens) {
  int j = 0;
  while (tokens[j] != NULL) {
    free(tokens[j]);
    j++;
  }
}
