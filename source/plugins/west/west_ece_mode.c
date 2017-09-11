// Iterative C program to search an element in linked list
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Link list node */
struct Node {
    int key;
    int ECE_mode;
    struct Node* next;
};

/* Given a reference (pointer to pointer) to the head
  of a list and an int, push a new node on the front
  of the list. */
void push(struct Node** head_ref, int new_key, int ECE_mode)
{
    /* allocate node */
    struct Node* new_node =
            (struct Node*)malloc(sizeof(struct Node));

    /* put in the key  */
    new_node->key = new_key;

    /* put in the ECE mode */
    new_node->ECE_mode = ECE_mode;

    /* link the old list off the new node */
    new_node->next = (*head_ref);

    /* move the head to point to the new node */
    (*head_ref) = new_node;
}

/* Checks whether the value x is present in linked list */
struct Node* search(struct Node* head, int x)
{
    struct Node* current = head;  // Initialize current
    while (current != NULL) {
        if (current->key == x) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
