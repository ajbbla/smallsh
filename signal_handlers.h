#ifndef SIGNAL_HANDLERS_H
#define SIGNAL_HANDLERS_H

#include <signal.h>

// Boolean for foreground-only mode
extern volatile sig_atomic_t fg_mode;

void catch_SIGINT(int signo);
void toggle_fg_mode_on(int signo);
void toggle_fg_mode_off(int signo);

#endif