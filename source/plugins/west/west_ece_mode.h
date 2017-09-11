#ifndef IDAM_PLUGIN_WEST_ECE_MODE_H
#define IDAM_PLUGIN_WEST_ECE_MODE_H

/* Link list node */
struct Node
{
    int key;
    int ECE_mode;
    struct Node* next;
};


void push(struct Node** head_ref, int new_key, int ECE_mode);
struct Node* search(struct Node* head, int x);

#endif // IDAM_PLUGIN_WEST_ECE_MODE_H
