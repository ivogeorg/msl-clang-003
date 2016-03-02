/*
 * Created by Ivo Georgiev on 2/9/16.
 * Modified by Matthew Brake 3/2/16.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h> // for perror()

#include "mem_pool.h"

/*************/
/*           */
/* Constants */
/*           */
/*************/
static const float      MEM_FILL_FACTOR                 = 0.75;
static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY    = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR      = 0.75;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR    = 2;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY     = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR       = 0.75;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR     = 2;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY        = 40;
static const float      MEM_GAP_IX_FILL_FACTOR          = 0.75;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR        = 2;

/*********************/
/*                   */
/* Type declarations */
/*                   */
/*********************/
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
    unsigned gap_ix_capacity;
} pool_mgr_t, *pool_mgr_pt;

/***************************/
/*                         */
/* Static global variables */
/*                         */
/***************************/
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expand
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;

/********************************************/
/*                                          */
/* Forward declarations of static functions */
/*                                          */
/********************************************/
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

/****************************************/
/*                                      */
/* Definitions of user-facing functions */
/*                                      */
/****************************************/
alloc_status mem_init() {
    // ensure that it's called only once until mem_free
    // allocate the pool store with initial capacity
    // note: holds pointers only, other functions to allocate/deallocate
    if (pool_store == NULL) {
        pool_store = (pool_mgr_pt*) calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_pt));
        pool_store_capacity += 20;
        return ALLOC_OK;
    }
    return ALLOC_CALLED_AGAIN;
}

alloc_status mem_free() {
    // ensure that it's called only once for each mem_init
    // make sure all pool managers have been deallocated
    // can free the pool store array
    // update static variables
    if(pool_store != NULL) {
        free(pool_store);
        pool_store = NULL;
        return ALLOC_OK;
    }
    return ALLOC_FAIL;
}

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // make sure that the pool store is allocated
    // expand the pool store, if necessary
    if( (pool_store_size / pool_store_capacity) > MEM_POOL_STORE_FILL_FACTOR){
        pool_store = (pool_mgr_pt*) realloc(pool_store, pool_store_capacity * MEM_POOL_STORE_EXPAND_FACTOR * sizeof(pool_mgr_pt));
        pool_store_capacity *= MEM_POOL_STORE_EXPAND_FACTOR;
    };

    // allocate a new mem pool mgr
    // check success, on error return null
    pool_mgr_t *pool_manager;
    if(!(pool_manager = (pool_mgr_pt) malloc(sizeof(pool_mgr_t)))){
        return NULL;
    };

    // allocate a new memory pool
    // check success, on error deallocate mgr and return null
    pool_t *pool;
    if(!(pool = (pool_t*) malloc(size + sizeof(pool_t)))){
        free(pool_manager);
        return NULL;
    };
    pool->total_size = size;
    pool->alloc_size = 0;
    pool->policy = policy;
    pool->num_gaps = 1;
    pool->num_allocs = 0;
    //pool->mem = sizeof(pool);


    // allocate a new node heap
    // check success, on error deallocate mgr/pool and return null
    node_t *node_heap;
    if(!(node_heap = (node_pt) malloc(sizeof(node_t) * MEM_NODE_HEAP_INIT_CAPACITY))){
        free(pool_manager);
        free(pool);
        return NULL;
    };

    // allocate a new gap index
    // check success, on error deallocate mgr/pool/heap and return null
    gap_t *gap_ix;
    if(!(gap_ix = (gap_pt) malloc(sizeof(gap_t) * MEM_GAP_IX_INIT_CAPACITY))){
        free(pool_manager);
        free(pool);
        free(node_heap);
        return NULL;
    };

    // assign all the pointers and update meta data:
    //   initialize top node of node heap
    node_heap->next = NULL;
    node_heap->prev = NULL;
    node_heap->allocated = 0;
    node_heap->used = 1;
    node_heap->alloc_record.size = pool->total_size;
    node_heap->alloc_record.mem = (char *) pool;

    //   initialize top node of gap index ALLOC_FAIL
    gap_ix->node = node_heap;

    //   initialize pool mgr
    pool_manager->pool = *pool;
    pool_manager->node_heap = node_heap;
    pool_manager->total_nodes = 1;
    pool_manager->used_nodes = 1;
    pool_manager->gap_ix = gap_ix;
    pool_manager->gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;
    pool_manager->gap_ix->node->alloc_record.mem = node_heap->alloc_record.mem;


    //   link pool mgr to pool store
    pool_store[pool_store_size] = pool_manager;
    pool_store_size++;
    // return the address of the mgr, cast to (pool_pt)
    return (pool_pt) pool_manager;
}

alloc_status mem_pool_close(pool_pt pool) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;
    // check if this pool is allocated
    if(!pool_mgr->pool.alloc_size){
    };
    // check if pool has only one gap
    if(pool_mgr->gap_ix->size == 1){
    }
    // check if it has zero allocations
    if(pool_mgr->pool.num_allocs == 0){
    }
    // free memory pool
    free(&pool_mgr->pool);
    // free node heap
    free(pool_mgr->node_heap);
    // free gap index
    free(pool_mgr->gap_ix);
    // find mgr in pool store and set to null
    for(int i = 0;i<pool_store_capacity;i++){
      if(pool_store[i] == pool_mgr){
          pool_store[i] = NULL;
      };
    };
    // note: don't decrement pool_store_size, because it only grows
    // free mgr
    free(pool_mgr);

    return ALLOC_OK;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;
    // check if any gaps, return null if none
    if(pool_mgr->gap_ix->size == 0){
        return NULL;
    };
    // expand heap node, if necessary, quit on error
    // check used nodes fewer than total nodes, quit on error
    // get a node for allocation:
    // if FIRST_FIT, then find the first sufficient node in the node heap
    // if BEST_FIT, then find the first sufficient node in the gap index
    // check if node found
    // update metadata (num_allocs, alloc_size)
    // calculate the size of the remaining gap, if any
    // remove node from gap index
    // convert gap_node to an allocation node of given size
    // adjust node heap:
    //   if remaining gap, need a new node
    //   find an unused one in the node heap
    //   make sure one was found
    //   initialize it to a gap node
    //   update metadata (used_nodes)
    //   update linked list (new node right after the node for allocation)
    //   add to gap index
    //   check if successful
    // return allocation record by casting the node to (alloc_pt)

    return NULL;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // get node from alloc by casting the pointer to (node_pt)
    // find the node in the node heap
    // this is node-to-delete
    // make sure it's found
    // convert to gap node
    // update metadata (num_allocs, alloc_size)
    // if the next node in the list is also a gap, merge into node-to-delete
    //   remove the next node from gap index
    //   check success
    //   add the size to the node-to-delete
    //   update node as unused
    //   update metadata (used nodes)
    //   update linked list:
    /*
                    if (next->next) {
                        next->next->prev = node_to_del;
                        node_to_del->next = next->next;
                    } else {
                        node_to_del->next = NULL;
                    }
                    next->next = NULL;
                    next->prev = NULL;
     */

    // this merged node-to-delete might need to be added to the gap index
    // but one more thing to check...
    // if the previous node in the list is also a gap, merge into previous!
    //   remove the previous node from gap index
    //   check success
    //   add the size of node-to-delete to the previous
    //   update node-to-delete as unused
    //   update metadata (used_nodes)
    //   update linked list
    /*
                    if (node_to_del->next) {
                        prev->next = node_to_del->next;
                        node_to_del->next->prev = prev;
                    } else {
                        prev->next = NULL;
                    }
                    node_to_del->next = NULL;
                    node_to_del->prev = NULL;
     */
    //   change the node to add to the previous node!
    // add the resulting node to the gap index
    // check success

    return ALLOC_FAIL;
}

void mem_inspect_pool(pool_pt pool,
                      pool_segment_pt *segments,
                      unsigned *num_segments) {
    // get the mgr from the pool
    pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;

    // allocate the segments array with size == used_nodes
    // check successful
    pool_segment_pt segs;
    if(!(segs = (pool_segment_pt) calloc(pool_mgr->used_nodes, sizeof(pool_segment_t)))){
        printf("Failed allocation of segments array \n");
    };

    // loop through the node heap and the segments array
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    node_pt current_node = pool_mgr->node_heap;
    for(int i = 0; i<pool_mgr->total_nodes; i++){
        segs[i].allocated = current_node->allocated;
        segs[i].size = current_node->alloc_record.size;
    };
    *segments = segs;
    *num_segments = pool_mgr->used_nodes;
}

/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
static alloc_status _mem_resize_pool_store() {
    // check if necessary
    // don't forget to update capacity variables
    if (((float) pool_store_size / pool_store_capacity)
                    > MEM_POOL_STORE_FILL_FACTOR) {
                    if(pool_store = (pool_mgr_pt*) realloc(pool_store,
                                                        pool_store_capacity * MEM_POOL_STORE_EXPAND_FACTOR * sizeof(pool_mgr_pt))){
                        pool_store_capacity *= MEM_POOL_STORE_EXPAND_FACTOR;
                        return ALLOC_OK;
                    };
                };
    return ALLOC_FAIL;
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // see above
    if (((float) pool_mgr->node_heap->used / pool_mgr->node_heap->alloc_record.size) > MEM_NODE_HEAP_FILL_FACTOR){
        if((pool_mgr->node_heap = (node_pt) realloc(pool_mgr->node_heap,
                                                MEM_NODE_HEAP_EXPAND_FACTOR * MEM_NODE_HEAP_INIT_CAPACITY * sizeof(pool_mgr->node_heap)))){
            pool_mgr->node_heap->alloc_record.size *= MEM_NODE_HEAP_EXPAND_FACTOR;
            return ALLOC_OK;
        };
    };
    return ALLOC_FAIL;
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // see above
    if(((float)pool_mgr->gap_ix->size / pool_mgr->gap_ix_capacity) > MEM_GAP_IX_FILL_FACTOR){

        if((pool_mgr->gap_ix = (gap_pt) realloc(pool_mgr->gap_ix,
                                                MEM_GAP_IX_EXPAND_FACTOR * MEM_GAP_IX_INIT_CAPACITY * sizeof(pool_mgr->gap_ix)))){
            pool_mgr->gap_ix_capacity *= MEM_GAP_IX_EXPAND_FACTOR;
            return ALLOC_OK;
        };
    };
    return ALLOC_FAIL;
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // expand the gap index, if necessary (call the function)
    _mem_resize_gap_ix(pool_mgr);
    // add the entry at the end
    pool_mgr->gap_ix[pool_mgr->gap_ix->size - 1].node = node;
    pool_mgr->gap_ix[pool_mgr->gap_ix->size - 1].size = size;
    // update metadata (num_gaps)
    pool_mgr->pool.num_gaps++;
    // sort the gap index (call the function)
    // check success
    if(_mem_sort_gap_ix(pool_mgr) == ALLOC_OK){
        return ALLOC_OK;
    };

    return ALLOC_FAIL;
}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // find the position of the node in the gap index
    // loop from there to the end of the array:
    //    pull the entries (i.e. copy over) one position up
    //    this effectively deletes the chosen node
    int position;
    for(int i=0; i < pool_mgr->pool.num_gaps; i++){
        if(pool_mgr->gap_ix[i].node == node){
            position = 1;
        }
    }
    while(pool_mgr->gap_ix[position + 1].node != NULL){
        pool_mgr->gap_ix[position].node = pool_mgr->gap_ix[position + 1].node;
        position++;
    };

    // update metadata (num_gaps)
    pool_mgr->pool.num_gaps--;

    // zero out the element at position num_gaps!
    gap_t empty;
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps] = empty;

    return ALLOC_FAIL;
}

// note: only called by _mem_add_to_gap_ix, which appends a single entry
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {
    // the new entry is at the end, so "bubble it up"
    // loop from num_gaps - 1 until but not including 0:
    //    if the size of the current entry is less than the previous (u - 1)
    //       swap them (by copying) (remember to use a temporary variable)
    for(int i= pool_mgr->pool.num_gaps - 1;i > 0; i--){
      if(pool_mgr->gap_ix[i].node->alloc_record.size > pool_mgr->gap_ix[i - 1].node->alloc_record.size){
        //swap
          struct _gap temp_gap;
          temp_gap = pool_mgr->gap_ix[i - 1];
          pool_mgr->gap_ix[i - 1] = pool_mgr->gap_ix[i];
          pool_mgr->gap_ix[i] = temp_gap;
      };
    };
    return ALLOC_OK;
}


