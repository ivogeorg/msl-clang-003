//
// Created by Ivo Georgiev on 1/31/16.
//

#ifndef DENVER_OS_PA_C_ALLOC_LIST_H
#define DENVER_OS_PA_C_ALLOC_LIST_H

#include <stddef.h>

#include "mem_alloc.h"

struct _alloc_node {
    struct _alloc_node *nextNode;
    size_t following_gap_size;      // for quick search
    struct _alloc_rec allocRec;     // on the bottom, adjacent to memory block
};
typedef struct _alloc_node alloc_node;

// alloc_new
// : makes an allocation, and adds to list, if possible, otherwise NULL
// arg : list - the linked list representing all allocations
// arg : size - the total size needed to be allocated
// arg : policy - the policy to use in the search for a gap
// ret : a pointer to the allocation (incl. alloc_rec), NULL on failure
char *alloc_new(alloc_node *list, size_t size, alloc_policy policy);

// alloc_del
// : deletes an allocation, including the node
// arg : list - the linked list representing all allocations
// arg : alloc - a pointer to an allocation
void alloc_del(alloc_node *list, char *alloc);




#endif //DENVER_OS_PA_C_ALLOC_LIST_H
