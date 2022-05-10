/* Header file for the smallsh functions */

#ifndef SMALLSH_H
#define SMALLSH_H

#include <signal.h>

#include "llist.h"

struct Input // used to organize instances of user input
{
  char **args;
  int numArgs;
  char *infile;
  char *outfile;
  int background; // Boolean for background processes
};

// Boolean for foreground-only mode
extern volatile sig_atomic_t fg_mode;

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
void kill_bg(struct Node *bgList);
void catch_SIGINT(int signo);
void toggle_fg_mode_on(int signo);
void toggle_fg_mode_off(int signo);

#endif
