# smallsh

smallsh is a shell program that implements a subset of features of well-known shells, such as bash.

* Provides a prompt for running commands
* Handles blank lines and comments, which are lines beginning with the # character
* Provides expansion for the variable $$
* Executes 3 commands exit, cd, and status via code built into the shell
* Executes other commands by creating new processes using a function from the exec family of functions
* Supports input and output redirection
* Supports running commands in foreground and background processes
* Uses custom handlers for 2 signals-- SIGINT and SIGTSTP
