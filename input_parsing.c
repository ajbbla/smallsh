/**
 * Definitions for user input functions
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input_parsing.h"
#include "utilities.h"

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
