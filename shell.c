#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tokenize_lib.h"

#define MAX_LINE_CHARS 255

/**
 * Reads input from the terminal and replaces any trailing newlines with a null
 * terminator
 * 
 * Inputs:
 * char* input - Buffer to store input string
 * 
 * Return
 * -1 if read failed, 0 otherwise
 */
int read_input(char *input) {
  if (fgets(input, MAX_LINE_CHARS, stdin) == NULL) {
    return -1;
  }
  
  if (input[strlen(input) - 1] == '\n') {
    input[strlen(input) - 1] = '\0';
  }

  return 0;
}

/**
 * Executes a system command by forking a new process and executing the
 * specified command (with arguments) using execvp. 
 *
 * Input:
 * char** args - Array of strings containing the system command and its 
 *               arguments
 * 
 * Return:
 * -1 if the fork failed, 0 otherwise
 */
int exec_cmd(char **args) {
  pid_t pid = fork();
  if (pid < 0) {
    printf("Fork failed");
    return -1;
  }

  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      printf("%s: command not found\n", args[0]);
    }
    exit(0);
  }

  else {
    wait(NULL);
  }
  
  return 0;
}

/**
 * Changes the current working directory to the specified file path.
 *
 * Input:
 * char* file_path - Path of the directory to change to
 */
void exec_cd(char *file_path) {
  if (chdir(file_path) != 0) {
    printf("cd: %s: No such file or directory\n", file_path);
  }
}

/**
 * Runs a script file by reading and executing each line one at a time.
 * Overwrites the command line input buffer with the last line of the script
 * file to be copied to the previous command buffer later on.
 * 
 * Input:
 * char* file_name - Name of the script file to run
 * char* input_cli - Command line input buffer to overwrite
 */
void exec_source(char *file_name, char *input_cli) {
  FILE *file = fopen(file_name, "r");
  char *tokens[MAX_LINE_CHARS + 1];
  char quoted_tokens[MAX_LINE_CHARS + 1] = {0};

  if (file == NULL) {
    printf("source: %s: No such file\n", file_name);
    return;
  }
  
  char line_buf[MAX_LINE_CHARS + 1];

  while (fgets(line_buf, sizeof(line_buf), file) != NULL) {
    if (line_buf[strlen(line_buf) - 1] == '\n') {
      line_buf[strlen(line_buf) - 1] = '\0';
    }

    tokenize(line_buf, tokens, quoted_tokens);

    exec_cmd(tokens);

    memset(input_cli, 0, sizeof(input_cli));
    strncpy(input_cli, line_buf, MAX_LINE_CHARS + 1);
  }
  
  free_token_mem(tokens);
}

/**
 * Prints a list of all built-in commands available in the shell.
 */
void exec_help() {
  printf("Available built-in commands:\n");
  printf("exit - exit the shell\n");
  printf("cd [directory] - change the working directory\n");
  printf("source [file] - execute the specified script\n");
  printf("prev - print and execute the previous command line\n");
  printf("help - lists internally defined shell commands\n");
}

/**
 * Processes input tokens for i/o redirection operations. If a redirection
 * operator is found, the function forks a new process and handles the redirection.
 *
 * Input:
 * char** tokens - Array of tokens to process
 * char* quoted_tokens - Array of flags indicating if the corresponding token
 *                       was encased in quotes
 */
void process_redirects(char **tokens, char *quoted_tokens) {
  int tok_idx = 0;

  while (tokens[tok_idx] != NULL) {
    if ((strcmp(tokens[tok_idx], "<") == 0 || 
        strcmp(tokens[tok_idx], ">") == 0) && 
        quoted_tokens[tok_idx] != 1) {
      if (tokens[tok_idx + 1] == NULL) {
        perror("No redirect file specified");
        return;
      }

      pid_t pid = fork();
      if (pid == -1) {
        perror("Error - fork failed");
        exit(1);
      }
      else if (pid == 0) {
        fflush(stdout);
        if (strcmp(tokens[tok_idx], "<") == 0) {
          if (close(0) == -1) {
            perror("Error closing stdin");
            exit(1);
          }
          if (open(tokens[tok_idx + 1], O_RDONLY) != 0) {
            perror("Error opening input file");
            exit(1);
          }
        }
        else {
          if (close(1) == -1) {
            perror("Error closing stdout");
            exit(1);
          }
          if (open(tokens[tok_idx + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644) != 1) {
            perror("Error opening output file");
            exit(1);
          }
        }
        tokens[tok_idx] = NULL;
        process_redirects(tokens, quoted_tokens);
        exit(0);
      }
      else {
        wait(NULL);
        tok_idx += 2;
        process_redirects(&tokens[tok_idx], &quoted_tokens[tok_idx]);
        return;
      }
    }
    tok_idx++;
  }

  exec_cmd(tokens);
}

/**
 * Processes input tokens for pipe operations. If a pipe operator is found, the
 * function forks two child processes and creates a pipe between them. If the 
 * first token is "source", the function executes a "source" command instead.
 *
 * Input:
 * char** tokens - Array of tokens to process
 * char* quoted_tokens - Array of flags indicating if the corresponding token
 *                       was encased in quotes
 * char* input_cli - Command line input buffer (required for executing source
 *                   command)
 */
void process_pipes(char **tokens, char *quoted_tokens, char *input_cli) {
  int tok_idx = 0;

  if (strcmp(tokens[0], "cd") == 0) {
    if (tokens[1] != NULL) {
      exec_cd(tokens[1]);
    }
    return;
  }
  else if (strcmp(tokens[0], "source") == 0) {
    if (tokens[1] != NULL) {
      exec_source(tokens[1], input_cli);
    }
    return;
  }
  else if (strcmp(tokens[0], "help") == 0) {
    exec_help();
    return;
  }

  while (tokens[tok_idx] != NULL) {
    if (strcmp(tokens[tok_idx], "|") == 0 && quoted_tokens[tok_idx] != 1) {
      pid_t pid_a = fork();
      if (pid_a == -1) {
        perror("Error - fork failed");
        exit(1);
      }
      else if (pid_a == 0) {
        int pipe_fds[2];
        if (pipe(pipe_fds) != 0) {
          perror("Error - pipe failed");
          exit(1);
        }

        int read_fd = pipe_fds[0];
        int write_fd = pipe_fds[1];

        pid_t pid_b = fork();
        if (pid_b == -1) {
          perror("Error - fork failed");
          exit(1);
        }
        else if (pid_b == 0) {
          fflush(stdout);
          if (close(1) == -1) {
            perror("Error closing stdout");
            exit(1);
          }
          assert(dup(write_fd) == 1);

          close(read_fd);
          
          tokens[tok_idx] = NULL;
          process_redirects(tokens, quoted_tokens);

          close(write_fd);
          exit(0);
        }
        else {
          if (close(0) == -1) {
            perror("Error closing stdin");
            exit(1);
          }
          assert(dup(read_fd) == 0);

          close(write_fd);
          
          tok_idx++;
          process_pipes(&tokens[tok_idx], &quoted_tokens[tok_idx], NULL);

          wait(NULL);
          close(read_fd);
          exit(0);
        }
      }
      else {
        wait(NULL);
        return;
      }
    }
    tok_idx++;
  }
  process_redirects(tokens, quoted_tokens);
}

/**
 * Processes input tokens for command sequences (separated by ";"). If a
 * semicolon is found, the function forks a child process to execute each
 * command in sequence.
 *
 * char** tokens - Array of tokens to process
 * char* quoted_tokens - Array of flags indicating if the corresponding token
 *                       was encased in quotes
 * char* input_cli - Command line input buffer
 */
void process_sequence(char **tokens, char *quoted_tokens, char *input_cli) {
  int tok_idx = 0;

  while (tokens[tok_idx] != NULL) {
    if (strcmp(tokens[tok_idx], ";") == 0 && quoted_tokens[tok_idx] != 1) {
      pid_t pid = fork();
      if (pid == -1) {
        perror("Error - fork failed");
        exit(1);
      }
      else if (pid == 0) {
        tokens[tok_idx] = NULL;
        process_pipes(tokens, quoted_tokens, input_cli);
        exit(0);
      }
      else {
        wait(NULL);
        tok_idx++;
        process_sequence(&tokens[tok_idx], &quoted_tokens[tok_idx], input_cli);
        return;
      }
    }

    tok_idx++;
  }
  process_pipes(tokens, quoted_tokens, input_cli);
}

/**
 * Processes the input string by tokenizing it and handling the "prev" built-in
 * command. If the input doesn't match the "prev" built-in command, it sends the
 * tokenized input to be processed for command sequences, pipes, redirects, and
 * execution.
 *
 * char* input - Buffer containing the user input string
 * char* prev_input - Buffer containing the previous command line string
 */
void process_input(char *input, char *prev_input) { 
  char quoted_args[MAX_LINE_CHARS + 1] = {0};
  char *tokens[MAX_LINE_CHARS + 1] = {0};
  int num_tokens = tokenize(input, tokens, quoted_args);

  if (strcmp(tokens[0], "prev") == 0) {
    if (prev_input[0] != '\0') {
      printf("%s\n", prev_input);
      strncpy(input, prev_input, strlen(prev_input) + 1);
      process_input(input, prev_input);
    }
    else {
      printf("No previous commands run, please run a command before running \"prev\"\n");
    }
  }
  else{
    process_sequence(tokens, quoted_args, input);
  }

  free_token_mem(tokens);
}

/**
 * Main function to run the shell. The function continuously prompts the user
 * for input, processes the input, and executes commands accordingly. The\
 * function exits when the user types "exit" or inputs 'Ctrl+D'.
 */
void run_shell() {
  char input_buf[MAX_LINE_CHARS + 1] = {0};
  char prev_input_buf[MAX_LINE_CHARS + 1] = {0};

  printf("Welcome to mini-shell\n");
  while (1) {
    printf("shell $ ");
    memset(prev_input_buf, 0, sizeof(prev_input_buf));
    strncpy(prev_input_buf, input_buf, strlen(input_buf));	     
	    
    if (read_input(input_buf) == -1) {
      printf("\nBye bye.\n");
      break;
    }

    if (strcmp(input_buf, "exit") == 0) {
      printf("Bye bye.\n");
      break;
    }

    if (input_buf[0] == '\0') {
      continue;
    }  

    process_input(input_buf, prev_input_buf);
  }
}

int main(int argc, char **argv) {
  run_shell();
  return 0;
}
