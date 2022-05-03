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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

char *get_pidstr(void);
struct Arguments *get_userinput(char *pidstr);
char **tokenize_input(char *buf);
struct Arguments *get_args(char **input);
char *strdup(const char *s);

struct Arguments
{
  char *command;
  char **options;
  char **parameters;
  int background; // Boolean
};

int
main(void)
{
  char *pidstr = get_pidstr();
  printf("PID: %s\n", pidstr);
  while (1)
  {
    struct Arguments *args = get_userinput(pidstr);
    // execute args
    // clean up args
  }
  free(pidstr);
  return EXIT_SUCCESS;
}

/**
 * Converts the current process ID into a string and returns the pointer
 * to it.
 * https://edstem.org/us/courses/21025/discussion/1437434?answer=3250618
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
        printf("Buffer is now %zu!\n", buf_size);
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
    printf("Your input was: '%s'\n", buf);
    char **tokens = tokenize_input(buf);
    /* sort input into a struct of arguments */
    struct Arguments *args = get_args(tokens);
    printf("Here are your arguments:\n");
    printf("command: '%s'\n", args->command);
    int i = 0;
    while (args->options[i] != NULL) 
    {    
      printf("option: '%s'\n", args->options[i]);
      ++i;
    }
    i = 0;
    while (args->parameters[i] != NULL) 
    {    
      printf("parameter: '%s'\n", args->parameters[i]);
      ++i;
    }
    printf("background: %d\n", args->background);
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
      printf("array_size is now %zu!\n", array_size / sizeof(char*));
    }
    tokens[count++] = strtok(NULL, " ");
  }
  printf("Here are your tokens:\n");
  for (int i = 0; i < count - 1; ++i)
  {
    printf("%d: '%s'\n", i, tokens[i]); 
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
  int i = 1; // input pointer
  int j = 0; // options pointer
  int k = 0; // params pointer
  while (input[i] != NULL)
  {
    if (input[i][0] == '-')
    { // add to options
      if (j * sizeof(char*) == params_size - sizeof(char*))
      {
        options_size *= 2;
        args->options = realloc(args->options, options_size);
        printf("options_size is now %zu!\n", options_size / sizeof(char*));
      }
      args->options[j++] = strdup(input[i]);
    }
    else if (strcmp(input[i], "&"))
    { // add to params
      if (k * sizeof(char*) == params_size - sizeof(char*))
      {
        params_size *= 2;
        args->parameters = realloc(args->parameters, params_size);
        printf("params_size is now %zu!\n", params_size / sizeof(char*));
      }
      args->parameters[k++] = strdup(input[i]);
    }
    ++i;
  }
  // terminate options and params
  args->options[j] = NULL;
  args->parameters[k] = NULL;
  // background -> 1 if final element == '&', 0 otherwise
  if (!strcmp(input[i - 1], "&")) args->background = 1;
  else args->background = 0;

  return args;
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
