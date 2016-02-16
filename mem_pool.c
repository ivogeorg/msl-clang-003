/*
 * Created by Ivo Georgiev on 2/9/16.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h> // for perror()

#include "mem_pool.h"

/* Constants */
static const float      MEM_FILL_FACTOR                 = 0.75;
static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY    = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR      = MEM_FILL_FACTOR;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR    = MEM_EXPAND_FACTOR;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY     = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR       = MEM_FILL_FACTOR;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR     = MEM_EXPAND_FACTOR;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY        = 40;
static const float      MEM_GAP_IX_FILL_FACTOR          = MEM_FILL_FACTOR;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR        = MEM_EXPAND_FACTOR;

/* Type declarations */
typedef struct _node {
    alloc_t alloc_record;
    unsigned used;
    unsigned allocated;
    struct _node *next, *prev; // doubly-linked list for gap deletion
} node_t, *node_pt;

typedef struct _gap {
    size_t size;
    node_pt node;
} gap_t, *gap_pt;

typedef struct _pool_mgr {
    pool_t pool;
    node_pt node_heap;
    unsigned total_nodes;
    unsigned used_nodes;
    gap_pt gap_ix;
} pool_mgr_t, *pool_mgr_pt;


/* Static global variables */
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expand
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;


/* Forward declarations of static functions */
static alloc_status _mem_resize_pool_store();
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status
        _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                           size_t size,
                           node_pt node);
static alloc_status
        _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                size_t size,
                                node_pt node);
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);


/* Definitions of user-facing functions */
alloc_status mem_init() {
    // TODO implement

    return ALLOC_FAIL;
}

alloc_status mem_free() {
    // TODO implement

    return ALLOC_FAIL;
}

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // TODO implement

    return NULL;
}

alloc_status mem_pool_close(pool_pt pool) {
    // TODO implement

    return ALLOC_FAIL;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // TODO implement

    return NULL;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // TODO implement

    return ALLOC_FAIL;
}

// NOTE: Allocates a dynamic array. Caller responsible for releasing.
void mem_inspect_pool(pool_pt pool, pool_segment_pt segments, unsigned *num_segments) {
    // TODO implement
}


/* Definitions of static functions */
static alloc_status _mem_resize_pool_store() {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // TODO implement

    return ALLOC_FAIL;
}


static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {

    // TODO implement

    return ALLOC_FAIL;
}


