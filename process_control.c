/**
 * Definitions for process control functions.
*/

#define _POSIX_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "llist.h"
#include "process_control.h"
#include "signal_handlers.h"

/**
 * Forks a child to try and execute the user input as a foreground process.
 *
 * @param input The full user command
 */
int 
fork_child_fg(struct Input *input)
{
  // Block SIGTSTP until fg process finishes
  sigset_t block_set;
  sigaddset(&block_set, SIGTSTP);
  sigprocmask(SIG_BLOCK, &block_set, NULL); // block SIGTSTP

  // Fork a child
  pid_t childPid = fork();
  if (childPid == -1)
  { // Fork error
    fprintf(stderr, "fork(): Fork failed\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  else if (childPid == 0)
  { // Child Process
    // Unblock SIGINT
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    // Setup SIGINT action
    struct sigaction SIGINT_action;
    SIGINT_action.sa_handler = catch_SIGINT; // custom handler
    sigemptyset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL); // child will catch SIGINT

    // Redirect I/O if needed and try to execute the command
    if (!redirect_input(input->infile) && !redirect_output(input->outfile))
    {
      exec_input(input->args);
    }

    // Error during execution
    cleanup_input(input);
    exit(EXIT_FAILURE);
  }
  else
  { // Parent Process
    // Wait for the child to finish
    int childStatus;
    int exitStatus;
    childPid = waitpid(childPid, &childStatus, 0);

    // Report its status
    if (WIFEXITED(childStatus))
    {
      exitStatus = WEXITSTATUS(childStatus);
    }
    else
    {
      exitStatus = -WTERMSIG(childStatus);
      printf("terminated by signal %d\n", -exitStatus);
      fflush(stdout);
    }
    sigprocmask(SIG_UNBLOCK, &block_set, NULL); // unblock SIGTSTP
    return exitStatus;
  }
}

/**
 * Forks a child to try and execute the user input as a background
 * process.
 *
 * @param input The full user command
 * @return A pointer to the new child Node 
 */
struct Node * 
fork_child_bg(struct Input *input)
{
  // Fork a child
  pid_t childPid = fork();
  if (childPid == -1)
  { // Fork error
    fprintf(stderr, "fork(): Fork failed\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  else if (childPid == 0)
  { // Child Process
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL); // child will ignore SIGTSTP

    // Define the path for I/O redirection
    char *in = "/dev/null";
    char *out = "/dev/null";
    if (input->infile != NULL)
    {
      in = input->infile;
    }
    if (input->outfile != NULL)
    {
      out = input->outfile;
    }

    // Redirect I/O and try to execute the input
    if (!redirect_input(in) && !redirect_output(out))
    {
      exec_input(input->args);
    }

    // Error while executing
    cleanup_input(input);
    exit(EXIT_FAILURE);
  }
  else
  { // Parent Process
    printf("background PID is %d\n", childPid);
    fflush(stdout);
    struct Node *newChild = init_node(childPid);
    return newChild;
  }
}


/**
 * (adapted from "Processes and I/O" Exploration)
 * Redirects stdin to the file with the given filename.
 *
 * @param infile The name of the input file
 * @return -1 for failure, 0 for success
 */
int
redirect_input(char *filename)
{
  if (filename != NULL)
  {
    int in = open(filename, O_RDONLY);
    fcntl(in, F_SETFD, FD_CLOEXEC);
    if (in == -1)
    {
      perror("open()");
      fflush(stderr);
      return -1;
    }
    if (dup2(in, 0) == -1)
    {
      perror("dup2()");
      fflush(stderr);
      return -1;
    }
  }
  return 0;
}

/**
 * (adapted from "Processes and I/O" Exploration)
 * Redirects stdout to the file with the given filename.
 *
 * @param outfile The name of the output file
 * @return -1 for failure, 0 for success
 */
int
redirect_output(char *filename)
{
  if (filename != NULL)
  {
    int out = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    fcntl(out, F_SETFD, FD_CLOEXEC);
    if (out == -1)
    {
      perror("open()");
      fflush(stderr);
      return -1;
    }
    if (dup2(out, 1) == -1)
    {
      perror("dup2()");
      fflush(stderr);
      return -1;
    }
  }
  return 0;
}

/**
 * Attempts to execute the given argument(s), printing an error and
 * returning if not.
 *
 * @param args The array of arguments
 * @return -1 to signify failure
 */
int
exec_input(char **args)
{
    // Attempt to exec the command
    execvp(args[0], args);

    // Error during execution
    fprintf(stderr, "execvp(): Bad argument(s) '%s", args[0]);
    for (int i = 1; args[i] != NULL; ++i)
    {
      fprintf(stderr, " %s", args[i]);
    }
    fprintf(stderr, "'\n");
    fflush(stderr);
    return -1;
}

/**
 * Attempts to reap any will child processes, returning the PID of the
 * reaped process if successful or 0 otherwise.
 *
 * @param bgList The pointer to the list of current background processes
 * @return The PID of the reaped process or 0
 */
int
reap(struct Node *bgList)
{
  if (bgList == NULL) // check if there are any processes to reap
  {
    return 0;
  }

  // Attempt to reap a process and report its exit status
  int reapedPid;
  int childStatus;
  int exitStatus;
  reapedPid = waitpid(-1, &childStatus, WNOHANG);
  if (reapedPid)
  {
    printf("background PID %d is done: ", reapedPid); 
    if (WIFEXITED(childStatus))
    {
      exitStatus = WEXITSTATUS(childStatus);
      printf("exit value %d\n", exitStatus);
    }
    else
    {
      exitStatus = WTERMSIG(childStatus);
      printf("terminated by signal %d\n", exitStatus);
    }
    fflush(stdout);
  }
  return reapedPid; // 0 = no reaped process
}

/**
 * Iterates over the list of background processes to kill them.
 *
 * @param bgList The pointer to the list of background processes
 */
void 
kill_bg(struct Node *bgList)
{
  struct Node *current = bgList;
  while (current != NULL)
  {
    if (kill(current->value, SIGTERM))
    {
      perror("kill()");
    }
    current = current->next;
  }
}
