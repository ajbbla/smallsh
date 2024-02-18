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

#include <stdlib.h>
#include <string.h>

#include "llist.h"
#include "signal_handlers.h"
#include "input_parsing.h"
#include "process_control.h"
#include "shell_commands.h"
#include "utilities.h"

// Boolean for foreground-only mode
volatile sig_atomic_t fg_mode = 0;

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
