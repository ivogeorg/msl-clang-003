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
    if(pool_store != NULL){
      return ALLOC_CALLED_AGAIN;
    };

    // allocate the pool store with initial capacity
    // note: holds pointers only, other functions to allocate/deallocate
        pool_store = calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_pt));
        pool_store_capacity = 20;
        return ALLOC_OK;
}

alloc_status mem_free() {
    // ensure that it's called only once for each mem_init
    if(pool_store == NULL){
      return ALLOC_FAIL;
    };

    // make sure all pool managers have been deallocated
    // can free the pool store array
    for(int i=0;i<pool_store_size;i++){
        if(pool_store[i] != NULL){
            mem_pool_close((pool_pt) pool_store[i]);
        };
    }

    // update static variables
    free(pool_store);
    pool_store = NULL;
    pool_store_size = 0;
    pool_store_capacity = 0;
    return ALLOC_OK;
}

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // make sure that the pool store is allocated
    if(pool_store == NULL){
        return NULL;
    };

    // expand the pool store, if necessary
    _mem_resize_pool_store();

    // allocate a new mem pool mgr
    pool_mgr_t *pool_manager;
    pool_manager = calloc(1,sizeof(pool_mgr_t));

    // check success, on error return null
    if(pool_manager == NULL){
        return NULL;
    };

    // allocate a new memory pool
    pool_manager->pool.mem = malloc(size);

    // check success, on error deallocate mgr and return null
    if(pool_manager->pool.mem == NULL){
        free(pool_manager);
        return NULL;
    };


    // allocate a new node heap
    pool_manager->node_heap = calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));
    pool_manager->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;

    // check success, on error deallocate mgr/pool and return null
    if(pool_manager->node_heap == NULL){
        free(pool_manager->pool.mem);
        free(pool_manager);
        return NULL;
    };

    // allocate a new gap index
    pool_manager->gap_ix = calloc(MEM_GAP_IX_INIT_CAPACITY,sizeof(gap_t));
    pool_manager->gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;

    // check success, on error deallocate mgr/pool/heap and return null
    if(pool_manager->gap_ix == NULL){
        free(pool_manager->pool.mem);
        free(pool_manager->node_heap);
        free(pool_manager);
        return NULL;
    };

    // assign all the pointers and update meta data:
    //   intitialize pool
    pool_manager->pool.total_size = size;
    pool_manager->pool.alloc_size = 0;
    pool_manager->pool.policy = policy;
    pool_manager->pool.num_gaps = 1;
    pool_manager->pool.num_allocs = 0;

    //   initialize top node of node heap
    pool_manager->node_heap[0].next = NULL;
    pool_manager->node_heap[0].prev = NULL;
    pool_manager->node_heap[0].allocated = 0;
    pool_manager->node_heap[0].used = 1;
    pool_manager->node_heap[0].alloc_record.size = pool_manager->pool.total_size;
    pool_manager->node_heap[0].alloc_record.mem = pool_manager->pool.mem;
    pool_manager->used_nodes = 1;

    //   initialize top node of gap index
    pool_manager->gap_ix[0].node = &pool_manager->node_heap[0];

    //   initialize pool mgr:

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
    if(pool_mgr->pool.mem == NULL){
        // not allocated
    };

    // check if pool has only one gap
    if(pool_mgr->gap_ix->size == 1){
        // only one gap
    }

    // check if it has zero allocations
    if(pool_mgr->pool.num_allocs == 0){
        // no allocations
    }

    // free memory pool
    free(pool_mgr->pool.mem);

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
    if (pool_mgr->pool.num_gaps == 0) {
        return NULL;
    };

    // expand heap node, if necessary, quit on error
    _mem_resize_node_heap(pool_mgr);

    // check used nodes fewer than total nodes, quit on error
    if (pool_mgr->used_nodes >= pool_mgr->total_nodes) {
        return NULL;
    };

    // get a node for allocation:
    // if FIRST_FIT, then find the first sufficient node in the node heap
    node_pt alloc_node;
    if (pool_mgr->pool.policy == FIRST_FIT) {
        for (int i = 0; i < pool_mgr->total_nodes; i++) {
            if (pool_mgr->node_heap[i].alloc_record.size >= size && pool_mgr->node_heap[i].allocated == 0) {
                alloc_node = &pool_mgr->node_heap[i];
                break;
            };
        }
    };

    // if BEST_FIT, then find the first sufficient node in the gap index
    if(pool_mgr->pool.policy == BEST_FIT){
        for (int i = 0; i < pool_mgr->total_nodes; i++) {
            if (pool_mgr->node_heap[i].alloc_record.size >= size && pool_mgr->node_heap[i].allocated == 0) {
                if (alloc_node != NULL && pool_mgr->node_heap[i].alloc_record.size < alloc_node->alloc_record.size){
                    alloc_node = &pool_mgr->node_heap[i];
                }else{
                    alloc_node = &pool_mgr->node_heap[i];
                };
            };
        };
    };

    // check if node found
    if (alloc_node == NULL) {
        printf("FAILED TO FIND A NODE");
        return NULL;
    };

    // update metadata (num_allocs, alloc_size)
    pool->num_allocs++;
    pool->alloc_size += size;

    // calculate the size of the remaining gap, if any
    size_t gap_remain;
    gap_remain = pool->total_size - pool->alloc_size;

    // remove node from gap index
    _mem_remove_from_gap_ix(pool_mgr, size, alloc_node);

    // convert gap_node to an allocation node of given size
    alloc_node->allocated = 1;
    alloc_node->alloc_record.size = size;

    // adjust node heap:
    //   if remaining gap, need a new node
    node_pt gap_node;

    //   find an unused one in the node heap
    if(gap_remain != 0){
        for(int i=0; i<pool_mgr->total_nodes;i++){
            if(pool_mgr->node_heap[i].used == 0){
                gap_node = &pool_mgr->node_heap[i];
                break;
            }
        }
    }
    //   make sure one was found
    if(gap_node == NULL){
        //gap not found!
    }

    //   initialize it to a gap node
    gap_node->alloc_record.size = gap_remain;
    gap_node->alloc_record.mem = alloc_node->alloc_record.mem + size;
    gap_node->used = 1;
    gap_node->allocated = 0;

    //   update metadata (used_nodes)
    pool_mgr->used_nodes++;

    //   update linked list (new node right after the node for allocation)
    //added to end
    alloc_node->next = gap_node;
    gap_node->next = NULL;
    gap_node->prev = alloc_node;
    pool->num_gaps++;

    //   add to gap index
    _mem_add_to_gap_ix(pool_mgr,gap_remain,gap_node);

    //   check if successful
    if(alloc_node == NULL){
        return NULL;
    };

    // return allocation record by casting the node to (alloc_pt)
    return (alloc_pt) alloc_node;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;

    // get node from alloc by casting the pointer to (node_pt)
    node_pt node = (node_pt) alloc;

    node_pt node_to_delete;
    // find the node in the node heap
    for(int i=0;i < pool_mgr->total_nodes; i++){
       if(pool_mgr->node_heap[i].alloc_record.mem == node->alloc_record.mem){
           // this is node-to-delete
           node_to_delete = &pool_mgr->node_heap[i];
           break;
        };
    };

    // make sure it's found
    if(node_to_delete == NULL){
        return ALLOC_NOT_FREED;
    };

    // convert to gap node
    node_to_delete->allocated = 0;

    // update metadata (num_allocs, alloc_size)
    pool_mgr->pool.num_allocs--;
    pool_mgr->pool.alloc_size = pool_mgr->pool.alloc_size - node_to_delete->alloc_record.size;

    // if the next node in the list is also a gap, merge into node-to-delete
    node_pt merge_node;
    if(node_to_delete->next != NULL && node_to_delete->next->allocated == 0 && node_to_delete->next->used == 1){

        merge_node = node_to_delete->next;

    //   remove the next node from gap index
        _mem_remove_from_gap_ix(pool_mgr,merge_node->alloc_record.size,merge_node);

    //   check success

    //   add the size to the node-to-delete
        node_to_delete->alloc_record.size = node_to_delete->alloc_record.size + node_to_delete->next->alloc_record.size;

        //   update node as unused
        merge_node->used = 0;

    //   update metadata (used nodes)
        pool_mgr->used_nodes--;

    //   update linked list:
        if (merge_node->next) {
            merge_node->next->prev = node_to_delete;
            node_to_delete->next = merge_node->next;
        } else {
            node_to_delete->next = NULL;
        }
        merge_node->next = NULL;
        merge_node->prev = NULL;
    };

    // this merged node-to-delete might need to be added to the gap index
    _mem_add_to_gap_ix(pool_mgr,node_to_delete->alloc_record.size,node_to_delete);

    // but one more thing to check...
    // if the previous node in the list is also a gap, merge into previous!
    node_pt prev_node;
    if (node_to_delete->prev != NULL && node_to_delete->prev->used == 1 && node_to_delete->prev->allocated == 0) {
        prev_node = node_to_delete->prev;

        //   remove the previous node from gap index... wrong remove node-to-delete
        _mem_remove_from_gap_ix(pool_mgr, prev_node->alloc_record.size, prev_node);
        _mem_remove_from_gap_ix(pool_mgr, node_to_delete->alloc_record.size,node_to_delete);

        //   check success

        //   add the size of node-to-delete to the previous
        prev_node->alloc_record.size = node_to_delete->alloc_record.size + node_to_delete->prev->alloc_record.size;

        //   update node-to-delete as unused
        node_to_delete->used = 0;
        node_to_delete->allocated = 0;

        //   update metadata (used_nodes)
        pool_mgr->used_nodes--;

        //   update linked list
        if (node_to_delete->next) {
            prev_node->next = node_to_delete->next;
            node_to_delete->next->prev = prev_node;
        } else {
            prev_node->next = NULL;
        }
        node_to_delete->next = NULL;
        node_to_delete->prev = NULL;

        //   change the node to add to the previous node!
        // add the resulting node to the gap index
        _mem_add_to_gap_ix(pool_mgr,prev_node->alloc_record.size,prev_node);

        // check success
    };

    return ALLOC_OK;
}

void mem_inspect_pool(pool_pt pool,
                      pool_segment_pt *segments,
                      unsigned *num_segments) {
    // get the mgr from the pool
    pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;

    // allocate the segments array with size == used_nodes
    // check successful
    pool_segment_pt segs;
    segs = calloc(pool_mgr->used_nodes, sizeof(pool_segment_t));
    if(segments == NULL){
        return;
    }

    // loop through the node heap and the segments array
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    for(int i = 0; i<pool_mgr->used_nodes; i++){
        segs[i].allocated = pool_mgr->node_heap[i].allocated;
        segs[i].size = pool_mgr->node_heap[i].alloc_record.size;
    };

    *segments = segs;
    *num_segments = pool_mgr->used_nodes;
    return;
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
                    if((pool_store = (pool_mgr_pt *) realloc(pool_store,
                                                             pool_store_capacity * MEM_POOL_STORE_EXPAND_FACTOR *
                                                             sizeof(pool_mgr_pt)))){
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
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].node = node;
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps].size = size;

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
    int position;
    for(int i=0; i < pool_mgr->pool.num_gaps - 1; i++){
        if(pool_mgr->gap_ix[i].node == node){
            position = i;
        }
    }
    // loop from there to the end of the array:
    //    pull the entries (i.e. copy over) one position up
    //    this effectively deletes the chosen node
    for(int i=position; i<pool_mgr->pool.num_gaps - 1; i++){
        pool_mgr->gap_ix[i] = pool_mgr->gap_ix[i+1];
    }

    // update metadata (num_gaps)
    pool_mgr->pool.num_gaps--;

    // zero out the element at position num_gaps!
    gap_t empty;
    pool_mgr->gap_ix[pool_mgr->pool.num_gaps] = empty;

    return ALLOC_OK;
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


