#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include "input_parsing.h"

int builtin_exit(struct Input *input);
void builtin_status(struct Input *input, int exitStatus);
void builtin_cd(struct Input *input);

#endif