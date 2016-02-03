//
// Created by Ivo Georgiev on 1/31/16.
//

#include <stdlib.h>
#include <assert.h>

#include "mem_alloc.h"
#include "alloc_list.h"

// TODO need a struct for the pool and a private heap for list operations

static char *mem_pool               = NULL;         // static memory pool
static alloc_policy mem_pool_policy = FIRST_FIT;    // allocation policy
static alloc_node *list             = NULL;         // linked list blocks
static int list_length              = 0;            // linked list size

int init_alloc_pool(size_t size, alloc_policy policy) {
    if (mem_pool) return ALLOC_FAILURE; // don't call more than once

    mem_pool_policy = policy;

    mem_pool = (char *) malloc(size);

    return (mem_pool ? ALLOC_SUCCESS : ALLOC_FAILURE);
}

// note: no calls to malloc/calloc/realloc => enforce with macros
alloc_rec *alloc_mem(size_t size) {
    char *mem = alloc_new(list, sizeof(alloc_rec) + size, mem_pool_policy);
    alloc_rec *arec = (alloc_rec *) mem; // allocation record is on top;

    if (mem) {
        // Success: Adjust pointers
        arec->mem = mem + sizeof(alloc_rec); // point to the allocation block
        arec->size = size;
    } // Failure: return NULL

    return arec;
}

// note: no calls to free => enforce with macros
int dealloc_mem(alloc_rec *arec) {
    if (! arec) return ALLOC_FAILURE; // NULL pointer passed

    alloc_del(list, (char *) arec);

    return ALLOC_SUCCESS;

}

int close_pool() {
    assert(! list); // list should be empty

    if (mem_pool) {
        free((void *) mem_pool);
        mem_pool = NULL;

        return ALLOC_SUCCESS;
    } else {
        return ALLOC_FAILURE;   // failure:
                                // close_pool called again, or
                                // init_alloc_pool not called/failed
    }
}
