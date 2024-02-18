/**
 * Definitions for built-in shell command functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shell_commands.h"
#include "utilities.h"

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
  if (input->numArgs > 1)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: exit\n");
    fflush(stderr);
    return 1;
  }
  return 0;
}

/**
 * Prints out either the exit status or the terminating signal of the
 * last foreground process ran by the shell. Signals are passed as their
 * negation to differentiate from exit values.
 *
 * @param input The full user command
 */
void 
builtin_status(struct Input *input, int exitStatus)
{
  if (input->numArgs > 1)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: status\n");
    fflush(stderr);
    return;
  }
  if (exitStatus < 0)
  {
    printf("terminated by signal %d\n", abs(exitStatus));
  }
  else
  {
    printf("exit value %d\n", exitStatus);
  }
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
  if (input->numArgs > 2)
  {
    fprintf(stderr, "Invalid number of arguments\nUsage: cd [PATH]\n");
    fflush(stderr);
    return;
  }

  // Change the directory according to the given arguments
  const char *home = getenv("HOME");
  char *cwd = getcwd_a();
  char *path = input->args[1];
  if (path == NULL)
  {
    if (chdir(home))
    {
      perror("chdir()");
    }
  }
  else if (chdir(path))
  {
    perror("chdir()");
  }

  free(cwd);
}
