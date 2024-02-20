/**
 * Definitions for signal handling functions.
*/

#include <stdlib.h>
#include <unistd.h>

#include "signal_handlers.h"

/**
 * Signal handler for SIGINT that forces the process to self-terminate.
 *
 * @param signo The number of the signal
 */
void 
catch_SIGINT(int signo)
{
  write(STDOUT_FILENO, "Caught SIGINT\n", 14);
  exit(1);
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
