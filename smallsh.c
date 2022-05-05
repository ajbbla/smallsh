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
#include <err.h>
#include <errno.h>

char *get_pidstr(void);
struct Input *get_userinput(char *pidstr);
char **tokenize_input(char *buf);
struct Input *get_input(char **input);
void cleanup_input(struct Input *input);
char *strdup(const char *s);
int builtin_exit(struct Input *input);
void builtin_status(struct Input *input, int exit_status);
void builtin_cd(struct Input *input);
char * getcwd_a(void);
int exec_input(struct Input *input);

struct Input
{
  // TODO: refactor program to match the following
  char **args;
  int num_args;
  char *infile;
  char *outfile;
  int background; // Boolean
};

int
main(void)
{
  int exit_status = 0;
  char *pidstr = get_pidstr();
  printf("PID: %s\n", pidstr);
  struct Input *input = get_userinput(pidstr);
  while (1)
  {
    if (input->args == NULL || input->args[0][0] == '#')
    { // ignore empty inputs and comments
    } // (skip to the end of the if/else block)
    else if (!strcmp(input->args[0], "exit") && !builtin_exit(input))
    {
      break;
    }
    else if (!strcmp(input->args[0], "status"))
    {
      builtin_status(input, exit_status);
    }
    else if (!strcmp(input->args[0], "cd"))
    {
      builtin_cd(input);
    }
    else
    {
      exit_status = exec_input(input);
    }
    cleanup_input(input);
    input = get_userinput(pidstr);
  }
  cleanup_input(input);
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
struct Input * 
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
    struct Input *input = get_input(tokens);
    free(buf);
    free(tokens);
    return input;
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
 * Initializes and returns a pointer to an Input struct from the given
 * user input.
 *
 * @param tokens The tokenized user input as an array of strings
 * @return The pointer to the initialized Input struct
 */
struct Input *
get_input(char **tokens)
{
  struct Input *input = malloc(sizeof(struct Input));
  input->args = NULL;
  input->num_args = 0;
  input->infile = NULL;
  input->outfile = NULL;
  input->background = 0;
  if (tokens[0] != NULL) // check for an empty input
  {
    size_t args_size = 4 * sizeof(char*);
    input->args = malloc(args_size);
    int i = 0; // tokens index
    int j = 0; // args index
    while (tokens[i] != NULL)
    {
      if (strcmp(tokens[i], "&")) // ignore non-terminal "&" tokens
      { // add to args
        if (!strcmp(tokens[i], "<")) // check for input redirection
        {
          input->infile = strdup(tokens[i + 1]);
          printf("infile: '%s'\n", input->infile);
          i += 2;
        }
        else if (!strcmp(tokens[i], ">")) // check for output redirection
        {
          input->outfile = strdup(tokens[i + 1]);
          printf("outfile: '%s'\n", input->outfile);
          i += 2;
        }
        else
        {
          if (j * sizeof(char*) == args_size - sizeof(char*))
          {
            args_size *= 2;
            input->args = realloc(input->args, args_size);
          }
          input->args[j++] = strdup(tokens[i++]);
          printf("args[%d]: '%s'\n", j-1, input->args[j-1]);
        }
      }
      else if (tokens[i + 1] == NULL)
      {
        input->background = 1;
        printf("background: yes\n");
        ++i;
      }
    }
    input->num_args = j;
    input->args[j] = NULL;
  }
  return input;
}

/**
 * Frees the memory used by the given Input struct and its members.
 *
 * @param input The pointer to the Input struct
 */
void 
cleanup_input(struct Input *input)
{
  if (input->args != NULL)
  {
    for (int i = 0; i < input->num_args; ++i) // free all args
    {
      free(input->args[i]);
    }
    free(input->args); // free the args array
  }
  if (input->infile != NULL)
  {
    free(input->infile); // free the infile string
  }
  if (input->outfile != NULL)
  {
    free(input->outfile); // free the outfile string
  }
  free(input); // free the struct itself
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
 * @param input The full user command
 * @return 0 or 1 to signal success or failure
 */
int 
builtin_exit(struct Input *input)
{
  if (input->num_args > 1)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: exit\n");
    return 1;
  }
  return 0;
}

/**
 * TODO: print terminating signal if applicable
 * Prints out either the exit status or the terminating signal of the
 * last foreground process ran by the shell.
 *
 * @param input The full user command
 */
void 
builtin_status(struct Input *input, int exit_status)
{
  if (input->num_args > 1)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: status\n");
    return;
  }
  printf("exit value %d\n", exit_status);
  fflush(stdout);
}

/**
 * Changes the current working directory of the shell. Without extra
 * arguments, the cwd is changed according to the HOME environment
 * variable.
 *
 * @param input The full user command
 */
void 
builtin_cd(struct Input *input)
{
  if (input->num_args > 2)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: cd [PATH]\n");
    return;
  }
  const char *home = getenv("HOME");
  char *cwd = getcwd_a();
  char *path = input->args[1];
  if (path == NULL)
  {
    if (chdir(home))
    {
      fprintf(stderr, "Error changing to home directory\n");
    }
  }
  else if (chdir(path))
  {
    fprintf(stderr, "Invalid path: '%s'\n", path);
  }
  printf("cwd: '%s'\n", cwd);
  printf("changed to: '%s'\n", path);
  free(cwd);
}

/**
 * (from archive.c skeleton code)
 * Allocates a string containing the CWD
 *
 * @return allocated string
 */
char *
getcwd_a(void)
{
  char *pwd = NULL;
  for (size_t sz = 128;; sz *= 2)
  {
    pwd = realloc(pwd, sz);
    if (getcwd(pwd, sz)) break;
    if (errno != ERANGE) err(errno, "getcwd()");
  }
  return pwd;
}

/**
 * TODO:
 * Attempts to execute the user command.
 *
 * @param input The full user command
 */
int 
exec_input(struct Input *input)
{}
