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

char * get_pidstr(void);
void get_userinput(char *pidstr);

int
main(void)
{
  char *pidstr = get_pidstr();
  printf("%s\n", pidstr);
  while (1)
  {
    get_userinput(pidstr);
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
void 
get_userinput(char *pidstr)
{
    size_t buf_size = 64;
    char *buf = malloc((buf_size)); 
    int buf_count = 0;
    char c;
    char next;
    printf(": ");
    while ((c = fgetc(stdin)) != '\n')
    {
      if (buf_count == buf_size - 1)
      {
        buf_size *= 2;
        buf = realloc(buf, buf_size);
        printf("Buffer is now %zu!\n", buf_size);
      }
      if (c == '$')
      {
        if ((next = fgetc(stdin)) == '$')
        {
          sprintf(buf + buf_count, "%s", pidstr);
          buf_count += strlen(pidstr);
        }
        else
        {
            buf[buf_count++] = c;
            if (next != '\n') buf[buf_count++] = next;
            else break;
        }
      }
      else
      {
        buf[buf_count++] = c;
      }
    }
    buf[buf_count] = '\0';
    printf("Your input was: '%s'\n", buf);
    /* sort input into a struct of arguments */
    free(buf);
}

