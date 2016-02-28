/*
* Created by Ivo Georgiev on 2/9/16.
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
#define MEM_FILL_FACTOR   0.75;
#define MEM_EXPAND_FACTOR 2;

//static const float      MEM_FILL_FACTOR                 = 0.75;
//static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR = MEM_FILL_FACTOR;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR = MEM_EXPAND_FACTOR;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR = MEM_FILL_FACTOR;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR = MEM_EXPAND_FACTOR;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY = 40;
static const float      MEM_GAP_IX_FILL_FACTOR = MEM_FILL_FACTOR;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR = MEM_EXPAND_FACTOR;



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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_status mem_init() {
     // ensure that it's called only once until mem_free
     // allocate the pool store with initial capacity
     // note: holds pointers only, other functions to allocate/deallocate

     // Here is where we allocate the memory pool
     //printf("mem_init: pool_store = %d\n", pool_store);
     if (pool_store == NULL) {

          // This is the first call to mem_init() since the pool_store is NULL
          // so we need to allocate the pool_store (array of pointers)
          // rather than creating a static block)
          pool_store = malloc(MEM_POOL_STORE_INIT_CAPACITY * sizeof(pool_mgr_pt));
          pool_store_size = 0;
          pool_store_capacity = MEM_POOL_STORE_INIT_CAPACITY;

          return ALLOC_OK;
     }
     else {
          printf("mem_init: pool_store is NULL, returning ALLOC_FAIL\n");
          return ALLOC_FAIL;
     }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_status mem_free() {
     // ensure that it's called only once for each mem_init
     // make sure all pool managers have been deallocated
     // can free the pool store array
     // update static variables

     return ALLOC_FAIL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
pool_pt mem_pool_open(size_t size, alloc_policy policy) {

     pool_mgr_pt mem_pool_mgr = NULL;

     // make sure that the pool store is allocated
     if (pool_store == NULL) {
          printf("ERROR: mem_pool_open pool store is not allocated.\n");
          return NULL;
     }

     // expand the pool store, if necessary
     // We shouldn't run into this, but if we used up all of our pool stores,
     // then we'd use realloc and get a larger # of pool_mgr_pt in our pool_store
     // that's what we originally allocated in mem_init())
     if (pool_store_size == pool_store_capacity) {
          printf("mem_pool_open: increase the size of the pool_store\n");
          pool_store = realloc(pool_store, (pool_store_capacity + MEM_POOL_STORE_INIT_CAPACITY) * sizeof(pool_mgr_pt));
          if (pool_store == NULL) {
               return NULL;
          }
          pool_store_capacity += MEM_POOL_STORE_INIT_CAPACITY;
     }

     // allocate a new mem pool mgr
     // check success, on error return null
     printf("mem_pool_open: allocating new mem pool manager\n");
     mem_pool_mgr = malloc(sizeof(pool_mgr_t));
     if (mem_pool_mgr == NULL) {
          printf("ERROR: mem_pool_open can't allocate new mem pool manager\n");
          return NULL;
     }

     // allocate a new memory pool
     // check success, on error deallocate mgr and return null
     printf("mem_pool_open: allocating new memory pool\n");
     mem_pool_mgr->pool.policy = policy;
     mem_pool_mgr->pool.mem = malloc(size);
     mem_pool_mgr->pool.total_size = size;
     mem_pool_mgr->pool.alloc_size = 0;
     mem_pool_mgr->pool.num_allocs = 0;
     mem_pool_mgr->pool.num_gaps = 1;
     if (mem_pool_mgr->pool.mem == NULL) {
          free(mem_pool_mgr);
          return NULL;
     }

     // allocate a new node heap
     // check success, on error deallocate mgr/pool and return null
     printf("mem_pool_open: allocating new node heap\n");
     mem_pool_mgr->node_heap = malloc(sizeof(node_t) * MEM_NODE_HEAP_INIT_CAPACITY);
     if (mem_pool_mgr->node_heap == NULL) {
          free(mem_pool_mgr->pool.mem);
          free(mem_pool_mgr);
          return NULL;
     }
     mem_pool_mgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;

     node_pt prevNodePt = NULL;
     node_pt nodePt = mem_pool_mgr->node_heap;
     for (int i = 0; i < MEM_NODE_HEAP_INIT_CAPACITY; i++){
          if (prevNodePt != NULL){
               prevNodePt->next = nodePt;
               nodePt->prev = prevNodePt; 
          }
          prevNodePt = nodePt;
          nodePt++;
     }

     // allocate a new gap index
     // check success, on error deallocate mgr/pool/heap and return null
     printf("mem_pool_open: allocating new gap index\n");
     mem_pool_mgr->gap_ix = malloc(sizeof(gap_t) * MEM_GAP_IX_INIT_CAPACITY);
     if (mem_pool_mgr->gap_ix == NULL) {
          free(mem_pool_mgr->pool.mem);
          free(mem_pool_mgr->node_heap);
          free(mem_pool_mgr);
          return NULL;
     }
     mem_pool_mgr->gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;

     // assign all the pointers and update meta data:
     //   initialize top node of node heap
     mem_pool_mgr->node_heap->alloc_record.mem = mem_pool_mgr->pool.mem;
     mem_pool_mgr->node_heap->alloc_record.size = mem_pool_mgr->pool.alloc_size;
     mem_pool_mgr->node_heap->used = 0;
     mem_pool_mgr->node_heap->allocated = 0;

     //   initialize top node of gap index
     mem_pool_mgr->gap_ix->node = mem_pool_mgr->node_heap;
     mem_pool_mgr->gap_ix->size = size;

     //   initialize pool mgr
     mem_pool_mgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
     mem_pool_mgr->used_nodes = 1;
     
     //   link pool mgr to pool store (find the first open slot)
     int ps_idx = 0;
     for (ps_idx = 0;
          ps_idx < pool_store_capacity && (*(pool_store + ps_idx) != NULL);
          ps_idx++) {
          //printf("loop: ps_idx = %d\n", ps_idx);
     }
     printf("mem_pool_open: available pool store index = %d\n", ps_idx);
     pool_store[ps_idx] = mem_pool_mgr;
     pool_store_size++;

     // return the address of the mgr, cast to (pool_pt)
     return (pool_pt)mem_pool_mgr;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_status mem_pool_close(pool_pt pool) {
     // get mgr from pool by casting the pointer to (pool_mgr_pt)
     pool_mgr_pt mem_pool_mgr = (pool_mgr_pt) pool;
     // check if this pool is allocated
     //if (mem_pool_mgr->node_heap == NULL) {
     printf("mem_pool_close doesn't have valid pool to close...");
          return ALLOC_FAIL;
    
     // check if pool has only one gap
     if (mem_pool_mgr->gap_ix == NULL) {
          printf("mem_pool_close doesn't have valid pool to close...");
               return ALLOC_FAIL;
     }
     // check if it has zero allocations
     // free memory pool
     free(mem_pool_mgr->pool.mem);
     // free node heap
     free(mem_pool_mgr->node_heap);
     // free gap index
     free(mem_pool_mgr->gap_ix);
     // find mgr in pool store and set to null
     // note: don't decrement pool_store_size, because it only grows
     // free mgr
     free(mem_pool_mgr);

     return ALLOC_FAIL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
     // get mgr from pool by casting the pointer to (pool_mgr_pt)
     pool_mgr_pt mem_pool_mgr = (pool_mgr_pt)pool;
     gap_pt gap = mem_pool_mgr->gap_ix;
     gap_pt good_gap = NULL;

     printf("IN mem_new_alloc\n");

     printf("gap ix capa %d", mem_pool_mgr->gap_ix_capacity);

     // check if any gaps, return null if none
     for (int i = 0; i > mem_pool_mgr->gap_ix_capacity; i++){
          if (gap->size >= size){
               printf("We have a gap...");
               good_gap = gap;
               break;
          }
          else{
               printf("Bad gap");
          }
     }

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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments) {
     // get the mgr from the pool
     pool_mgr_pt pool_mgr = (pool_mgr_pt) pool;

     // allocate the segments array with size == used_nodes
     pool_segment_pt segs = malloc(sizeof(pool_segment_t) * pool_mgr->used_nodes);

     // check successful
     if (segs == NULL){
          printf("ERROR: allocating segments array in mem_inspect_pool.");
          return;
     }

     pool_segment_pt seg = segs;
     node_pt checking_nodes = pool_mgr->node_heap;

     for (int i = 0; i < pool_mgr->used_nodes; i++){
          seg->size = checking_nodes->alloc_record.size;
          seg->allocated = checking_nodes->allocated;
          ++seg;
     }//end of for

     *segments = segs;
     *num_segments = pool_mgr->used_nodes;

     return;
}//end of mem_inspect_pool



/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
static alloc_status _mem_resize_pool_store() {
     // check if necessary
     /*
     if (((float) pool_store_size / pool_store_capacity)
     > MEM_POOL_STORE_FILL_FACTOR) {...}
     */
     // don't forget to update capacity variables

     return ALLOC_FAIL;
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
     // see above

     return ALLOC_FAIL;
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
     // see above

     return ALLOC_FAIL;
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
     size_t size,
     node_pt node) {

     // expand the gap index, if necessary (call the function)
     // add the entry at the end
     // update metadata (num_gaps)
     // sort the gap index (call the function)
     // check success

     return ALLOC_FAIL;
}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
     size_t size,
     node_pt node) {
     // find the position of the node in the gap index
     // loop from there to the end of the array:
     //    pull the entries (i.e. copy over) one position up
     //    this effectively deletes the chosen node
     // update metadata (num_gaps)
     // zero out the element at position num_gaps!

     return ALLOC_FAIL;
}

// note: only called by _mem_add_to_gap_ix, which appends a single entry
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {
     // the new entry is at the end, so "bubble it up"
     // loop from num_gaps - 1 until but not including 0:
     //    if the size of the current entry is less than the previous (u - 1)
     //       swap them (by copying) (remember to use a temporary variable)

     return ALLOC_FAIL;
}
