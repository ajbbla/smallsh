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

#define _POSIX_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

struct Input // used to organize instances of user input
{
  char **args;
  int numArgs;
  char *infile;
  char *outfile;
  int background; // Boolean for background processes
};

struct Node // used for llist implementation
{
  int value;
  struct Node *next;
};

volatile sig_atomic_t fg_mode = 0; // Boolean for foreground-only mode

char *get_pidstr(void);
struct Input *get_userinput();
char **tokenize_input(char *buf);
struct Input *get_input(char **input);
void cleanup_input(struct Input *input);
char *strdup(const char *s);
int builtin_exit(struct Input *input);
void builtin_status(struct Input *input, int exitStatus);
void builtin_cd(struct Input *input);
char *getcwd_a(void);
int fork_child_fg(struct Input *input);
struct Node *fork_child_bg(struct Input *input);
int exec_input(char **args);
int redirect_input(char *filename);
int redirect_output(char *filename);
int reap(struct Node *head);
struct Node *init_node(int value);
void append_node(struct Node *head, struct Node *newNode);
struct Node *delete_node(struct Node *head, int value);
void cleanup_llist(struct Node *head);
void kill_bg(struct Node *bgList);
void print_llist(struct Node *head);
void catch_SIGINT(int signo);
void toggle_fg_mode_on(int signo);
void toggle_fg_mode_off(int signo);

int
main(void)
{
  // Declare parent signal behavior
  struct sigaction ignore_action, SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = toggle_fg_mode_on;
  SIGTSTP_action.sa_flags = 0;
  ignore_action.sa_handler = SIG_IGN;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL); // parent will catch SIGTSTP
  sigaction(SIGINT, &ignore_action, NULL); // parent will ignore SIGINT

  int exitStatus = 0;
  printf("PID: %d\n", getpid());
  fflush(stdout);
  struct Input *input = get_userinput();
  struct Node *bgList = NULL; // llist to keep track of bg processes
  int reapedPid;

  // Parse user input
  while (1)
  {
    if (input->args == NULL || input->args[0][0] == '#')
    { // ignore empty inputs and comments; skip to end of if/else block
    } 
    else if (!strcmp(input->args[0], "exit") && !builtin_exit(input))
    {
      break;
    }
    else if (!strcmp(input->args[0], "status"))
    {
      builtin_status(input, exitStatus);
    }
    else if (!strcmp(input->args[0], "cd"))
    {
      builtin_cd(input);
    }
    else
    { // try to execute non-built-in command
      if (!fg_mode && input->background)
      {
        if (bgList == NULL)
        {
          bgList = fork_child_bg(input);
        }
        else
        {
          append_node(bgList, fork_child_bg(input));
        }
      }
      else
      {
        exitStatus = fork_child_fg(input);
      }
    } 

    // Attempt to reap a bg process
    reapedPid = reap(bgList);
    while (reapedPid)
    { // continue reaping until unsuccessful
      bgList = delete_node(bgList, reapedPid);
      reapedPid = reap(bgList);
    }

    // Proceed to the next prompt for user input
    cleanup_input(input);
    input = get_userinput();
  }

  // Final cleanup
  kill_bg(bgList);
  cleanup_llist(bgList);
  cleanup_input(input);
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
get_userinput()
{
    size_t buf_size = 64;
    char *buf = malloc((buf_size));
    char *pidstr = get_pidstr();
    int count = 0;
    char c;
    char next;

    printf(": ");
    fflush(stdout);
    while (1)
    {
      c = fgetc(stdin);
      if (c == '\n' || c == EOF)
      {
        break;
      }
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
    buf[count] = '\0'; // terminate the buffer

    // Convert the buffer into tokens to populate the Input struct
    char **tokens = tokenize_input(buf);
    struct Input *input = get_input(tokens);
    free(buf);
    free(pidstr);
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

  // Iterate over the buffer to generate tokens
  int count = 0;
  tokens[count++] = strtok(buf, " ");
  while (tokens[count - 1] != NULL)
  {
    if (count * sizeof(char*) == array_size - sizeof(char*))
    { // resize the tokens array if needed
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
  input->numArgs = 0;
  input->infile = NULL;
  input->outfile = NULL;
  input->background = 0;

  if (tokens[0] != NULL) // check for an empty input
  {
    size_t args_size = 4 * sizeof(char*);
    input->args = malloc(args_size);
    int i = 0; // tokens index
    int j = 0; // args index

    // Check each token to populate the Input struct
    while (tokens[i] != NULL)
    {
      if (!strcmp(tokens[i], "<")) 
      { // found a path for input redirection
        input->infile = strdup(tokens[i + 1]);
        i += 2;
      }
      else if (!strcmp(tokens[i], ">")) 
      { // found a path for output redirection
        input->outfile = strdup(tokens[i + 1]);
        i += 2;
      }
      else if (!strcmp(tokens[i], "&") && tokens[i + 1] == NULL)
      { // found a "&" argument at the end
        input->background = 1;
        ++i;
      }
      else
      { // found a regular argument
        if (j * sizeof(char*) == args_size - sizeof(char*))
        { // resize the args array
          args_size *= 2;
          input->args = realloc(input->args, args_size);
        }
        input->args[j++] = strdup(tokens[i++]);
      }
    }
    input->numArgs = j;
    input->args[j] = NULL; // terminate the args array
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
    for (int i = 0; i < input->numArgs; ++i) // free all args
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
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = catch_SIGINT;
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
 * Creates a new Node with the given value.
 *
 * @param value The integer value for the new Node
 * @return A pointer to the new Node
 */
struct Node *
init_node(int value)
{
  struct Node *newNode = malloc(sizeof(struct Node));
  newNode->value = value;
  newNode->next = NULL;
  return newNode;
}

/**
 * Adds the given node to the end of the given linked list.
 *
 * @param head The pointer to the head of the list
 * @param newNode The pointer to the new Node to be added
 */
void 
append_node(struct Node *head, struct Node *newNode)
{
    struct Node *current = head;
    while (current->next != NULL) // traverse to the end of the list
    {
      current = current->next;
    }
    current->next = newNode;
}

/**
 * Deletes the first Node found with the given value from the list and
 * frees its memory.
 *
 * @param head The first Node in the list
 * @param value The value of the Node to be deleted
 * @return A pointer to the head of the list
 */
struct Node *
delete_node(struct Node *head, int value)
{
  struct Node *current = head;
  struct Node *prev = NULL;

  // Find the unwanted value
  while (current->value != value)
  {
    prev = current;
    current = current->next;
  }

  if (prev == NULL)
  { // unwanted value is at the head
    struct Node *newHead = current->next;
    free(current);
    return newHead;
  }

  // Remove the unwanted Node
  prev->next = current->next;
  free(current);
  return head;
}

/**
 * Frees the memory of the given linked list in order.
 *
 * @param head The first Node in the list
 */
void
cleanup_llist(struct Node *head)
{
  struct Node *current = head;
  struct Node *tmp;
  while (current != NULL)
  {
    tmp = current->next;
    free(current);
    current = tmp;
  }
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

/**
 * Iterates over the given llist to print out its members.
 *
 * @param head The pointer to the head of the llist
 */
void
print_llist(struct Node *head)
{
  if (head == NULL)
  {
    printf("List: NULL");
  }
  else
  {
    struct Node *current = head;
    int i = 0;
    printf("List: ");
    while (current != NULL)
    {
      printf("[%d] %d > ", i++, current->value);
      current = current->next;
    }
  }
  printf("\n");
  fflush(stdout);
}

/**
 * Signal handler for SIGINT that forces the process to self-terminate.
 *
 * @param signo The number of the signal
 */
void 
catch_SIGINT(int signo)
{
  exit(0);
}

/**
 * Signal handler for SIGTSTP to turn on foreground-only mode.
 *
 * @param signo The number of the signal
 */
void
toggle_fg_mode_on(int signo)
{
  fg_mode = 1; // toggle fg_mode on

  // Switch the SIGTSTP handler to toggle_fg_mode_off
  struct sigaction SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = toggle_fg_mode_off;
  SIGTSTP_action.sa_flags = 0;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  char *message = "\nEntering foreground-only mode (& is now ignored)\n";
  write(STDOUT_FILENO, message, 50);
}

/**
 * Signal handler for SIGTSTP to turn off foreground-only mode.
 *
 * @param signo The number of the signal
 */
void
toggle_fg_mode_off(int signo)
{
  fg_mode = 0; // toggle fg_mode off

  // Switch the SIGTSTP handler to toggle_fg_mode_on
  struct sigaction SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = toggle_fg_mode_on;
  SIGTSTP_action.sa_flags = 0;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  char *message = "\nExiting foreground-only mode\n";
  write(STDOUT_FILENO, message, 30);
}
