#ifndef PROCESS_CONTROL_H
#define PROCESS_CONTROL_H

#include "llist.h"
#include "input_parsing.h"

int fork_child_fg(struct Input *input);
struct Node * fork_child_bg(struct Input *input);
int redirect_input(char *filename);
int redirect_output(char *filename);
int exec_input(char **args);
int reap(struct Node *bgList);
void kill_bg(struct Node *bgList);

#endif