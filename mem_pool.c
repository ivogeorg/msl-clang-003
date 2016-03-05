/*
* ORIGINAL WAS Created by Ivo Georgiev on 2/9/16.
* TESTED VERSION Created by Tegan Straley on 2/29/2016
*         for Operating Systems CSCI 3453, SPRING 2016
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
     if (pool_store == NULL) {

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
alloc_status mem_free(){
     // ensure that it's called only once for each mem_init
     if (pool_store == NULL) {
          return ALLOC_FAIL;
     }

     // make sure all pool managers have been deallocated
     int pool_manager_allocated = 0;
	 int i;
     for (i = 0; i < pool_store_capacity; i++) {
          if (pool_store[i] != NULL && !pool_manager_allocated) {
               // Found an allocated pool manager
               pool_manager_allocated = 1;
          }
     }

     if (pool_manager_allocated) {
          return ALLOC_FAIL;
     }

     // can free the pool store array
     free(pool_store);

     // update static variables
     pool_store = NULL;
     pool_store_size = 0;
     pool_store_capacity = 0;

     return ALLOC_OK;
 
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
          pool_store = realloc(pool_store, (pool_store_capacity + MEM_POOL_STORE_INIT_CAPACITY) * sizeof(pool_mgr_pt));
          if (pool_store == NULL) {
               return NULL;
          }
          pool_store_capacity += MEM_POOL_STORE_INIT_CAPACITY;
     }

     // allocate a new mem pool mgr
     // check success, on error return null
     mem_pool_mgr = malloc(sizeof(pool_mgr_t));
     if (mem_pool_mgr == NULL) {
          printf("ERROR: mem_pool_open can't allocate new mem pool manager\n");
          return NULL;
     }

     // allocate a new memory pool
     // check success, on error deallocate mgr and return null
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
     mem_pool_mgr->node_heap = malloc(sizeof(node_t) * MEM_NODE_HEAP_INIT_CAPACITY);
     if (mem_pool_mgr->node_heap == NULL) {
          free(mem_pool_mgr->pool.mem);
          free(mem_pool_mgr);
          return NULL;
     }
     mem_pool_mgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;

     node_pt prevNodePt = NULL;
     node_pt nodePt = mem_pool_mgr->node_heap;
	 int i;
     for (i = 0; i < MEM_NODE_HEAP_INIT_CAPACITY; i++){
          if (prevNodePt != NULL){
               prevNodePt->next = nodePt;
               nodePt->prev = prevNodePt; 
          }
          prevNodePt = nodePt;
          nodePt++;
     }

     // allocate a new gap index
     // check success, on error deallocate mgr/pool/heap and return null
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
     mem_pool_mgr->node_heap->alloc_record.size = mem_pool_mgr->pool.total_size;
     mem_pool_mgr->node_heap->used = 1;
     mem_pool_mgr->node_heap->allocated = 0;

     //   initialize top node of gap index
     mem_pool_mgr->gap_ix->node = mem_pool_mgr->node_heap;
     mem_pool_mgr->gap_ix->size = size;

     //   initialize pool mgr
     mem_pool_mgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
     mem_pool_mgr->used_nodes = 1;
     
     //   link pool mgr to pool store (find the first open slot)
     int ps_idx = 0;
    // for (ps_idx = 0; ps_idx < pool_store_capacity && (*(pool_store + ps_idx) != NULL);
    //      ps_idx++) {
          //printf("loop: ps_idx = %d\n", ps_idx);
    // }
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
     if (mem_pool_mgr == NULL) {
          printf("ERROR: pool to close was empty...\n");
          return ALLOC_FAIL;
     }
     // check if pool has only one gap
     gap_pt gap = mem_pool_mgr->gap_ix;
     int num_gaps = 0;
	int i;
     for (i = 0; i < mem_pool_mgr->gap_ix_capacity; i++){
          if (gap->node != NULL){
               num_gaps++;
          }
          gap++;
     }
     if (num_gaps > 1) {
          // Something's off if there's memory in use (more than one gap)
          //(the pool only has 1 gap when it's ready to close)
          return ALLOC_FAIL;
     }

     // check if it has zero allocations
     if (mem_pool_mgr->pool.num_allocs != 0){
          printf("ERROR : more than 0 allocations when trying to close : %d \n", mem_pool_mgr->pool.num_allocs);
          return ALLOC_FAIL;
     }

     // free memory pool
     free(pool->mem);
     // free node heap
     free(mem_pool_mgr->node_heap);
     // free gap index
     free(mem_pool_mgr->gap_ix);
     // find mgr in pool store and set to null
     // note: don't decrement pool_store_size, because it only grows
     for (i = 0; i < pool_store_capacity; i++) {
          if (pool_store[i] == mem_pool_mgr) {
               // Found the pool manager
               pool_store[i] = NULL;
          }
     }

     // free mgr
     free(mem_pool_mgr);

     return ALLOC_OK;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
     // get mgr from pool by casting the pointer to (pool_mgr_pt)
     pool_mgr_pt mem_pool_mgr = (pool_mgr_pt) pool;

     // check if any gaps, return null if none
     gap_pt gap = mem_pool_mgr->gap_ix;
     gap_pt good_gap = NULL;
	int i;
     for (i = 0; i < mem_pool_mgr->gap_ix_capacity && good_gap == NULL; i++){
          if (gap->size >= size){
               good_gap = gap;
          }
          gap++;
     }//end of for
     if (good_gap == NULL){
          return NULL;
     }

     // expand node heap, if necessary, quit on error
     if (_mem_resize_node_heap(mem_pool_mgr) == ALLOC_FAIL){
          return NULL;
     }

     // check used nodes fewer than total nodes, quit on error
     if (mem_pool_mgr->used_nodes >= mem_pool_mgr->total_nodes){
          printf("ERROR: mem_new_alloc used_nodes larger than total_nodes...\n");
     }

     // get a node for allocation:
     // if FIRST_FIT, then find the first sufficient node in the node heap
     // if BEST_FIT, then find the first sufficient node in the gap index
     node_pt node_to_use = NULL;
     if (pool->policy == FIRST_FIT){
		 node_pt node;
          for (node = mem_pool_mgr->node_heap; node != NULL && node_to_use == NULL; node = node->next){
               if (node->alloc_record.size >= size && node->allocated == 0){
                    node_to_use = node;
               }
          }//end of for
     }
     else{
          gap_pt gap = mem_pool_mgr->gap_ix;

          for (i = 0; i < mem_pool_mgr->gap_ix_capacity && node_to_use == NULL; i++){
               if (gap->size >= size){
                    node_to_use = gap->node;
               }
          }//end of for
     }//end of else

     // check if node found
     if (node_to_use == NULL){
          printf("ERROR: node_to_use wasn't found...\n");
          return NULL;
     }

     // update metadata (num_allocs, alloc_size)
     pool->num_allocs += 1;
     pool->alloc_size += size;

     // calculate the size of the remaining gap, if any
     size_t remaining_gap = node_to_use->alloc_record.size - size;

     // remove node from gap index
     good_gap->node = NULL;
    
     // convert gap_node to an allocation node of given size
     node_to_use->allocated = 1;
     node_to_use->alloc_record.size = size;

     // adjust node heap:
     //   if remaining gap, need a new node
     node_pt unused_node = NULL;

     if (remaining_gap > 0){
		 node_pt node;
          //   find an unused one in the node heap
          for (node = mem_pool_mgr->node_heap; node != NULL && unused_node == NULL; node = node->next){
               if (node->allocated == 0){
                    unused_node = node;
               }
          }//end of for

          //   make sure one was found
          if (unused_node == NULL){
               printf("ERROR: unused_node is still NULL...\n");
               return NULL;
          }

          //   initialize it to a gap node
          good_gap->node = unused_node;
          unused_node->allocated = 0;
          unused_node->used = 1;
          unused_node->alloc_record.size = remaining_gap;
          unused_node->alloc_record.mem = node_to_use->alloc_record.mem + size;

          //   update metadata (used_nodes)
          mem_pool_mgr->used_nodes += 1; 


     }//end of if

     // return allocation record by casting the node to (alloc_pt)
     return (alloc_pt) node_to_use;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
     // get mgr from pool by casting the pointer to (pool_mgr_pt)
     pool_mgr_pt mem_pool_mgr = (pool_mgr_pt)pool;

     // get node from alloc by casting the pointer to (node_pt)
     node_pt node_to_delete = (node_pt) alloc;

     // find the node in the node heap
     int found = 0;
	 node_pt node;
     for (node = mem_pool_mgr->node_heap; node != NULL && !found; node = node->next){
          if (node == node_to_delete){
               found = 1;
          }
     }//end of for

     // this is node-to-delete
     // make sure it's found
     if (node_to_delete == NULL){
          printf("ERROR: node_to_delete was not found in pool...\n");
          return ALLOC_FAIL;
     }

     // convert to gap node
     node_to_delete->allocated = 0;

     // update metadata (num_allocs, alloc_size)
   //  mem_pool_mgr->pool.num_allocs -= 1;
   //  mem_pool_mgr->pool.alloc_size = (mem_pool_mgr->pool.alloc_size - alloc->size);
     pool->num_allocs -= 1;
     mem_pool_mgr->pool.alloc_size = mem_pool_mgr->pool.alloc_size - alloc->size;

     // if the next node in the list is also a gap, merge into node-to-delete
     node_pt next_node = node_to_delete->next;
     if (next_node != NULL && next_node->allocated == 0 && next_node->used == 1){
          //   remove the next node from gap index
          int found_gap = 0;
          gap_pt gap = mem_pool_mgr->gap_ix;
          gap_pt matched_gap = NULL;
		  int i;
          for (i = 0; i < mem_pool_mgr->gap_ix_capacity && !found_gap; i++){
               if (gap->node == next_node){
                    found_gap = 1;
                    gap->node = NULL;
               }
               gap++;
          }
          //   check success NO

          //   add the size to the node-to-delete
          node_to_delete->alloc_record.size += next_node->alloc_record.size;

          //   update node as unused
          node_to_delete->used = 0;

          //   update metadata (used nodes)
          mem_pool_mgr->used_nodes -= 1;

          //   update linked list:
          if (next_node->next) {
               next_node->next->prev = node_to_delete;
               node_to_delete->next = next_node->next;
          }
          else {
               node_to_delete->next = NULL;
          }
          next_node->next = NULL;
          next_node->prev = NULL;
     }//end of if

     // this merged node-to-delete might need to be added to the gap index
     // but one more thing to check...
     // if the previous node in the list is also a gap, merge into previous! 
     node_pt prev_node = node_to_delete->prev;
     if (prev_node != NULL && prev_node->allocated == 0){
          //   remove the next node from gap index
          int found_gap = 0;
		  int i;
          gap_pt gap = mem_pool_mgr->gap_ix;
          gap_pt matched_gap = NULL;
          for (i = 0; i < mem_pool_mgr->gap_ix_capacity && !found_gap; i++){
               if (gap->node == prev_node){
                    found_gap = 1;
                    gap->node = NULL;
               }
               gap++;
          }
          //   check success NO

          //   add the size of node-to-delete to the previous
          prev_node->alloc_record.size += node_to_delete->alloc_record.size;

          //   update node as unused
          node_to_delete->used = 0;

          //   update metadata (used nodes)
          mem_pool_mgr->used_nodes -= 1;

          //   update linked list:
          if (node_to_delete->next) {
               prev_node->next = node_to_delete->next;
               node_to_delete->next->prev = prev_node;
          }
          else {
               prev_node->next = NULL;
          }
          node_to_delete->next = NULL;
          node_to_delete->prev = NULL;

          //   change the node to add to the previous node!
          node_to_delete = prev_node;
     }//end of if
	 int i;
     // add the resulting node to the gap index
     gap_pt gap = mem_pool_mgr->gap_ix;
     gap_pt available_gap = NULL;
     for (i = 0; i < mem_pool_mgr->gap_ix_capacity && available_gap == NULL; i++){
          if (gap->node == NULL){
               available_gap = gap;
          }//end of if
          gap++;
     }//end of for
     available_gap->node = node_to_delete;
     available_gap->size = node_to_delete->alloc_record.size;

     // check success


     return ALLOC_OK;
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
	 int i;
     for (i = 0; i < pool_mgr->used_nodes; i++){
          seg->size = checking_nodes->alloc_record.size;
          seg->allocated = checking_nodes->allocated;
          ++seg;
          checking_nodes = checking_nodes->next;
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
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
     // check if necessary
     if (((float)pool_mgr->used_nodes / pool_mgr->total_nodes) > MEM_NODE_HEAP_FILL_FACTOR) {

          int new_node_count = pool_mgr->total_nodes * MEM_NODE_HEAP_EXPAND_FACTOR;
          int new_heap_size = new_node_count * sizeof(node_t);
          node_pt new_node_heap = (node_pt)malloc(new_heap_size);
          if (new_node_heap != NULL){

               // update the total # nodes
               pool_mgr->total_nodes += new_node_count;

               // Since we're adding a chunk of nodes, link them onto the old list
               node_pt last_node = pool_mgr->node_heap;
               for (node_pt node = pool_mgr->node_heap; node->next != NULL; node = node->next) {
                    last_node = node;
               }
               last_node->next = new_node_heap;

               // Initialize the new nodes and link them together (as we did at the beginning)
               node_pt prev_node = NULL;
               node_pt this_node = new_node_heap;
               for (int i = 0; i < new_node_count; i++){
                    this_node->used = 1;
                    this_node->allocated = 0;
                    this_node->alloc_record.size = 0;
                    this_node->alloc_record.mem = 0;
                    if (prev_node != NULL){
                         prev_node->next = this_node;
                         this_node->prev = prev_node;
                    }
                    prev_node = this_node;
                    this_node++;
               }

               // just for fun, let's print out the entire node heap again
               //_print_node_heap(pool_mgr);

          }
          else {
               return ALLOC_FAIL;
          }
     }

     return ALLOC_OK;
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
	// see above
	// check if necessary
	/*
	if (((float) pool_store_size / pool_store_capacity)
	> MEM_POOL_STORE_FILL_FACTOR) {...}
	*/
	// don't forget to update capacity variables

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