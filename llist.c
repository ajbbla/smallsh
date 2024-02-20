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
 * Creates a new empty Linked List.
 * 
 * @return A pointer to the new List
*/
struct Llist *
init_llist(void)
{
  struct Llist *newLlist = malloc(sizeof(struct Llist));
  newLlist->head = NULL;
  newLlist->tail = NULL;
  newLlist->size = 0;
  return newLlist;
}

/**
 * Adds the given node to the end of the given linked list.
 *
 * @param llist The pointer to the list
 * @param newNode The pointer to the new Node to be added
 */
void 
append_node(struct Llist *llist, struct Node *newNode)
{
  if (llist->tail == NULL)  // llist is empty
  {
    llist->head = newNode;
    llist->tail = newNode;
  } 
  else  // llist is not empty
  {
    llist->tail->next = newNode;
    llist->tail = newNode;
  }
  llist->size++;
}

/**
 * Deletes the first Node found with the given value from the llist and
 * frees its memory.
 *
 * @param llist The pointer to the linked list
 * @param value The value of the Node to be deleted
 */
void
delete_node(struct Llist *llist, int value)
{
  // printf("Trying to delete %d...", value);
  struct Node *current = llist->head;
  struct Node *prev = NULL;

  // Find the unwanted value
  while (current != NULL && current->value != value)
  {
    prev = current;
    current = current->next;
  }

  if (current == NULL)  // value not found
  {
    return;
  }

  if (prev == NULL)  // value found at head
  {
    llist->head = current->next;
  }
  else
  {
    prev->next = current->next;
  }
  
  if (current == llist->tail)  // value found at tail
  {
    llist->tail = prev;
  }

  // printf("Found %d and freeing its memory!\n", current->value);
  free(current);
  llist->size--;
  if (llist->size == 0)  // llist is now empty
  {
    llist->head = NULL;
    llist->tail = NULL;
  }
}

/**
 * Frees the memory of the given linked list by first
 * freeing the memory of each of its Nodes in order
 * before freeing the memory of the llist itself.
 *
 * @param llist The pointer to the llist
 */
void
cleanup_llist(struct Llist *llist)
{
  struct Node *current = llist->head;
  struct Node *tmp;
  while (current != NULL)
  {
    tmp = current->next;
    free(current);
    current = tmp;
  }
  free(llist);
}

/**
 * Iterates over the given llist to print out its members.
 *
 * @param llist The pointer to the the llist
 */
void
print_llist(struct Llist *llist)
{
  struct Node *current = llist->head;
  if (current == NULL)
  {
    printf("Llist: NULL");
  }
  else
  {
    printf("Llist: (size: %d) ", llist->size);
    while (current != NULL)
    {
      printf("%d > ", current->value);
      current = current->next;
    }
  }
  printf("\n");
  fflush(stdout);
}
