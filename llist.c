/**
 * A linked list implementation including the functionality to:
 * - initialize a Node
 * - append a Node to a llist
 * - delete a Node from a llist
 * - free a llist's memory and that of its Nodes
 * - print a llist's values in head-to-tail order
 */

#include <stdlib.h>
#include <stdio.h>

#include "llist.h"

/**
 * Creates a new Node with the given value.
 *
 * @param value The integer value for the new Node
 * @return A pointer to the new Node
 */
struct Node *
init_node(int value)
{
  struct Node *newNode = malloc(sizeof(struct Node));
  newNode->value = value;
  newNode->next = NULL;
  return newNode;
}

/**
 * Adds the given node to the end of the given linked list.
 *
 * @param head The pointer to the head of the list
 * @param newNode The pointer to the new Node to be added
 */
void 
append_node(struct Node *head, struct Node *newNode)
{
    struct Node *current = head;
    while (current->next != NULL) // traverse to the end of the list
    {
      current = current->next;
    }
    current->next = newNode;
}

/**
 * Deletes the first Node found with the given value from the list and
 * frees its memory.
 *
 * @param head The first Node in the list
 * @param value The value of the Node to be deleted
 * @return A pointer to the head of the list
 */
struct Node *
delete_node(struct Node *head, int value)
{
  struct Node *current = head;
  struct Node *prev = NULL;

  // Find the unwanted value
  while (current->value != value)
  {
    prev = current;
    current = current->next;
  }

  if (prev == NULL)
  { // unwanted value is at the head
    struct Node *newHead = current->next;
    free(current);
    return newHead;
  }

  // Remove the unwanted Node
  prev->next = current->next;
  free(current);
  return head;
}

/**
 * Frees the memory of the given linked list in order.
 *
 * @param head The first Node in the list
 */
void
cleanup_llist(struct Node *head)
{
  struct Node *current = head;
  struct Node *tmp;
  while (current != NULL)
  {
    tmp = current->next;
    free(current);
    current = tmp;
  }
}

/**
 * Iterates over the given llist to print out its members.
 *
 * @param head The pointer to the head of the llist
 */
void
print_llist(struct Node *head)
{
  if (head == NULL)
  {
    printf("List: NULL");
  }
  else
  {
    struct Node *current = head;
    int i = 0;
    printf("List: ");
    while (current != NULL)
    {
      printf("[%d] %d > ", i++, current->value);
      current = current->next;
    }
  }
  printf("\n");
  fflush(stdout);
}
