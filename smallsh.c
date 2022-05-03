/**
 * NAME: smallsh - a small shell program
 * SYNOPSIS: smallsh
 * DESCRIPTION:
 * Implements a subset of features of well-known shells, such as bash:
 * - Provides a prompt for running commands
 * - Handles blank lines for comments (beginning with '#')
 * - Provides expansion for the variable $$
 * - Executes 3 commands built into the shell: exit, cd, and status
 * - Executes other commands by creating new processes using a function
 *   from the exec family of functions
 * - Supports input and output redirection
 * - Supports running commands in foreground and background processes
 * - Uses custom handlers for 2 signals: SIGINT and SIGTSTP
 * AUTHOR: Allen Blanton (CS 344, Spring 2022)
 */

/**
 * TODO:
 * - "cd" built-in
 * - "exit" built-in
 * - function to handle non-built-in commands (foregound)
 * - background functionality
 * - input/output redirection
 * - signals
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

char *get_pidstr(void);
struct Arguments *get_userinput(char *pidstr);
char **tokenize_input(char *buf);
struct Arguments *get_args(char **input);
void cleanup_args(struct Arguments *args);
char *strdup(const char *s);
int builtin_exit(struct Arguments *args);
void builtin_status(struct Arguments *args, int exit_status);
void builtin_cd(struct Arguments *args);
int exec_args(struct Arguments *args);

struct Arguments
{
  char *command;
  char **options;
  char **parameters;
  int num_options;
  int num_params;
  int background; // Boolean
};

int
main(void)
{
  int exit_status = 0;
  char *pidstr = get_pidstr();
  printf("PID: %s\n", pidstr);
  struct Arguments *args = get_userinput(pidstr);
  while (1)
  {
    if (!strcmp(args->command, "exit") && !builtin_exit(args))
    {
      break;
    }
    else if (!strcmp(args->command, "status"))
    {
      builtin_status(args, exit_status);
    }
    else if (!strcmp(args->command, "cd"))
    {
      builtin_cd(args);
    }
    else
    {
      exit_status = exec_args(args);
    }
    cleanup_args(args);
    args = get_userinput(pidstr);
  }
  cleanup_args(args);
  free(pidstr);
  return EXIT_SUCCESS;
}

/**
 * https://edstem.org/us/courses/21025/discussion/1437434?answer=3250618
 * Converts the current process ID into a string and returns the pointer
 * to it.
 *
 * @return The pointer to the PID string
 */
char *
get_pidstr(void)
{
  pid_t pid = getpid();
  char *pidstr;
  {
    int n = snprintf(NULL, 0, "%jd", pid);
    pidstr = malloc((n + 1) * sizeof *pidstr);
    sprintf(pidstr, "%jd", pid);
  }
  return pidstr;
}

/**
 * Reads a line of input from the user character by character, checking
 * for '$$' variable expansion along the way.
 * 
 * @param pidstr The current process ID as a string
 * @return The input divided up into a struct of arguments
 */
struct Arguments * 
get_userinput(char *pidstr)
{
    size_t buf_size = 64;
    char *buf = malloc((buf_size)); 
    int count = 0;
    char c;
    char next;
    printf(": ");
    while ((c = fgetc(stdin)) != '\n')
    {
      if (count == buf_size - 1)
      {
        buf_size *= 2;
        buf = realloc(buf, buf_size);
      }
      if (c == '$')
      {
        if ((next = fgetc(stdin)) == '$')
        {
          sprintf(buf + count, "%s", pidstr);
          count += strlen(pidstr);
        }
        else
        {
            buf[count] = c;
            if (next != '\n') buf[count++] = next;
            else break;
        }
      }
      else
      {
        buf[count++] = c;
      }
    }
    buf[count] = '\0';
    char **tokens = tokenize_input(buf);
    /* sort input into a struct of arguments */
    struct Arguments *args = get_args(tokens);
    free(buf);
    free(tokens);
    return args;
}

/**
 * Breaks the given buffer into an array of tokens using ' ' as the
 * delimiter.
 *
 * @param buf The buffer to be split
 * @return The array of tokens
 */
char ** 
tokenize_input(char *buf)
{
  size_t array_size = 4 * (sizeof(char*));
  char **tokens = malloc(array_size);
  int count = 0;
  tokens[count++] = strtok(buf, " ");
  while (tokens[count - 1] != NULL)
  {
    if (count * sizeof(char*) == array_size - sizeof(char*))
    {
      array_size *= 2;
      tokens = realloc(tokens, array_size);
    }
    tokens[count++] = strtok(NULL, " ");
  }
  return tokens;
}

/**
 * Initializes and returns a pointer to an Arguments struct from the given
 * user input.
 *
 * @param input The tokenized user input as an array of strings
 * @return The pointer to the initialized Arguments struct
 */
struct Arguments *
get_args(char **input)
{
  struct Arguments *args = malloc(sizeof(struct Arguments));
  // the first element is the command
  args->command = strdup(input[0]);
  size_t options_size = 4 * sizeof(char*);
  size_t params_size = 4 * sizeof(char*);
  args->options = malloc(options_size);
  args->parameters = malloc(params_size);
  int i = 1; // input index
  int j = 0; // options index
  int k = 0; // params index
  while (input[i] != NULL)
  {
    if (input[i][0] == '-')
    { // add to options
      if (j * sizeof(char*) == options_size - sizeof(char*))
      {
        options_size *= 2;
        args->options = realloc(args->options, options_size);
      }
      args->options[j++] = strdup(input[i]);
    }
    else if (strcmp(input[i], "&"))
    { // add to params
      if (k * sizeof(char*) == params_size - sizeof(char*))
      {
        params_size *= 2;
        args->parameters = realloc(args->parameters, params_size);
      }
      args->parameters[k++] = strdup(input[i]);
    }
    ++i;
  }
  // NULL terminate options and params
  args->options[j] = NULL;
  args->num_options = j;
  args->parameters[k] = NULL;
  args->num_params = k;
  // background -> 1 if final element == '&', 0 otherwise
  if (!strcmp(input[i - 1], "&")) 
  {
    args->background = 1;
  }
  else 
  {
    args->background = 0;
  }
  return args;
}

/**
 * Frees the memory used by the given Arguments struct and its members.
 *
 * @param args The pointer to the Arguments struct
 */
void 
cleanup_args(struct Arguments *args)
{
  free(args->command); // free the single command
  for (int i = 0; i <= args->num_options; ++i) // free all options
  {
    free(args->options[i]);
  }
  free(args->options); // free the options array
  for (int i = 0; i <= args->num_params; ++i) // free all params
  {
    free(args->parameters[i]);
  }
  free(args->parameters); // free the params array
  free(args); // free the struct itself
}

/**
 * https://edstem.org/us/courses/21025/discussion/1360952?answer=3085214
 * Duplicates the given string and returns a pointer to the copy.
 *
 * @param s The pointer to the string to duplicate
 * @return The pointer to the copy
 */
char *
strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);
  if (new == NULL)
    return NULL;
  return (char *) memcpy (new, s, len);
}

/**
 * Returns a 0 if the command to exit the shell is successful, or 1
 * otherwise.
 *
 * @param args The full user command
 * @return 0 or 1 to signal success or failure
 */
int 
builtin_exit(struct Arguments *args)
{
  if (args->options[0] != NULL || args->parameters[0] != NULL || 
      args->background)
  {
    fprintf(stderr, "Invalid options/parameters\nUsage: exit\n");
    return 1;
  }
  return 0;
}

/**
 * TODO: print terminating signal if applicable
 * Prints out either the exit status or the terminating signal of the
 * last foreground process ran by the shell.
 *
 * @param args The full user command
 */
void 
builtin_status(struct Arguments *args, int exit_status)
{
  if (args->options[0] != NULL || args->parameters[0] != NULL || 
      args->background)
  {
    fprintf(stderr, "Invalid options/parameters\nUsage: status\n");
    return;
  }
  printf("exit value %d\n", exit_status);
}

/**
 * TODO:
 * Changes the current working directory of the shell. Without extra
 * arguments, the cwd is changed according to the HOME environment
 * variable.
 *
 * @param args The full user command
 */
void 
builtin_cd(struct Arguments *args)
{}

/**
 * TODO:
 * Attempts to execute the user command.
 *
 * @param args The full user command
 */
int 
exec_args(struct Arguments *args)
{}
