/*
 * Created by Ivo Georgiev on 2/9/16.
 */

#include "mem_pool.h"

/* Type declarations */
typedef struct _node {
    alloc_t alloc_record;
    unsigned used;
    unsigned allocated;
    struct _node *next;
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
static pool_mgr_pt pool_store;
static unsigned pool_store_size;


/* Forward declarations of static functions */
static alloc_status _mem_new_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_del_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_new_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status _mem_del_gap_ix(pool_mgr_pt pool_mgr);


/* Definitions of user-facing functions */
pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    return NULL;
}

alloc_status mem_pool_close(pool_pt pool) {
    return ALLOC_FAIL;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    return NULL;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    return ALLOC_FAIL;
}


/* Definitions of static functions */
static alloc_status _mem_new_node_heap(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }
static alloc_status _mem_del_node_heap(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }
static alloc_status _mem_new_gap_ix(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }
static alloc_status _mem_del_gap_ix(pool_mgr_pt pool_mgr) { return ALLOC_FAIL; }

