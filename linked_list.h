#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode {
    void *data;
    size_t size;
    char type;
    struct ListNode *next;
} ListNode;

void list_append(ListNode **head, void *data, size_t data_size, char type);

void list_delete(ListNode **head,  ListNode* node);