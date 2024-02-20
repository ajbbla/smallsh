# smallsh

**smallsh** is a shell program that implements a subset of features of well-known 
shells, such as Bash.

## Installation
### Unix Systems
After cloning the repo or downloading and extracting the source files, navigate to the project folder in the terminal and use the Makefile to compile the project by using the following command:

`make`

To remove the object files and the executable, navigate to the project folder in the terminal and use the following command:

`make clean`

### Windows Systems
For Windows users, it is recommended to use WSL (Windows Subsystem for Linux) to provide a Unix-like environment. Once WSL is set up with your preferred Linux distribution, follow the Unix installation instructions above.

## Execution
Once the program has been compiled, it can be run in the terminal via the following command:

`./smallsh`

To exit the program, type `exit` and press `Enter/Return`.

## Features
### Provides a prompt for running commands:
![smallsh-1](https://github.com/allenjbb/smallsh/assets/105831767/403fcd34-299e-4794-b477-441ab4ebab48)

### Ignores blank lines and comments (lines beginning with the `#` character):
![smallsh-2](https://github.com/allenjbb/smallsh/assets/105831767/2a59b395-0275-4edc-8933-3a89f7375144)

### Provides expansion for the variable `$$`, replacing it with the PID:
![smallsh-3](https://github.com/allenjbb/smallsh/assets/105831767/978db7f4-9292-4130-9cb9-be5bbb94196e)

### Executes `status`, `cd`, and `exit` via code built into the shell and other commands via new processes forked by the exec family of functions:
![smallsh-4](https://github.com/allenjbb/smallsh/assets/105831767/88c9dda8-feeb-405b-8e56-576210be0d0a)

### Supports input and output redirection using `<` and `>` respectively:
![smallsh-5](https://github.com/allenjbb/smallsh/assets/105831767/b44862d6-b13e-4c16-9bc7-33cf0c2e8490)

### Supports running commands as background processes with `&` or foreground processes without:
![smallsh-6](https://github.com/allenjbb/smallsh/assets/105831767/1f6575c0-7867-45cb-a4e4-880ab69eaf7d)

### Uses custom handlers for 2 signals, SIGINT and SIGTSTP, to terminate foreground child processes or toggle foreground-only mode by pressing `Ctrl-C` or `Ctrl-Z` respectively:
![smallsh-7](https://github.com/allenjbb/smallsh/assets/105831767/41a1f1ed-9ecf-425e-81a6-534d37c9e3b3)

## Feedback
Any and all feedback is greatly appreciated. If you have a suggestion to improve this project, feel free to leave your thoughts in the [Discussions Tab](https://github.com/allenjbb/smallsh/discussions)!

## License
Distributed under the GNU General Public License v3.0. See LICENSE for more information.
