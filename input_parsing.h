#ifndef INPUT_PARSING_H
#define INPUT_PARSING_H

struct Input // used to organize instances of user input
{
  char **args;
  int numArgs;
  char *infile;
  char *outfile;
  int background; // Boolean for background processes
};

struct Input * get_userinput();
char ** tokenize_input(char *buf);
struct Input * get_input(char **tokens);
void cleanup_input(struct Input *input);

#endif