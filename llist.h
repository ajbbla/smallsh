/* Header file for the linked list implementation */

#ifndef LLIST_H
#define LLIST_H

struct Node
{
  int value;
  struct Node *next;
};

struct Llist
{
  struct Node *head;
  struct Node *tail;
  int size;
};

struct Node *init_node(int value);
struct Llist *init_llist(void);
void append_node(struct Llist *llist, struct Node *newNode);
void delete_node(struct Llist *llist, int value);
void cleanup_llist(struct Llist *llist);
void print_llist(struct Llist *llist);

#endif
