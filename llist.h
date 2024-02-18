/* Header file for the linked list implementation */

#ifndef LLIST_H
#define LLIST_H

struct Node
{
  int value;
  struct Node *next;
};

struct Node *init_node(int value);
void append_node(struct Node *head, struct Node *newNode);
struct Node *delete_node(struct Node *head, int value);
void cleanup_llist(struct Node *head);
void print_llist(struct Node *head);

#endif
