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

#define     MEM_FILL_FACTOR     0.75;
#define     MEM_EXPAND_FACTOR   2;


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
static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);
static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);



/****************************************/
/*                                      */
/* Definitions of user-facing functions */
/*                                      */
/****************************************/
alloc_status mem_init() {
	// ensure that it's called only once until mem_free
	if (pool_store != NULL)
		return ALLOC_CALLED_AGAIN;

    // allocate the pool store with initial capacity
	pool_store = (pool_mgr_pt *) calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_t));

	if (pool_store == NULL)
		return ALLOC_FAIL;

	pool_store_capacity = MEM_POOL_STORE_INIT_CAPACITY;
    // note: holds pointers only, other functions to allocate/deallocate
	return ALLOC_OK;
}

alloc_status mem_free() {
    // ensure that it's called only once for each mem_init
    // make sure all pool managers have been deallocated
	if (pool_store == NULL)
		return ALLOC_CALLED_AGAIN;

    for (int i = 0; i < pool_store_size; i ++) {
        if (pool_store[i] != NULL)
            mem_pool_close(&pool_store[i]->pool);
    }
    // can free the pool store array
	free(pool_store);

    // update static variables
	pool_store_capacity = 0;
	pool_store_size = 0;
	pool_store = NULL;

	return ALLOC_OK;
}

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // make sure there the pool store is allocated
	if (pool_store == NULL){
		return NULL;
	}

    // expand the pool store, if necessary
	if (_mem_resize_pool_store() == ALLOC_FAIL){
		return NULL;
	}

    // allocate a new mem pool mgr
	pool_mgr_pt new_mgr = calloc(1, sizeof(pool_mgr_t));

    // check success, on error return null
	if (new_mgr == NULL){
		return NULL;
	}

    // allocate a new memory pool
    new_mgr->pool.mem = (char*)malloc(size);

	// check success, on error deallocate mgr and return null
	if (new_mgr->pool.mem == NULL){
		free(new_mgr);
		return NULL;
	}

    // allocate a new node heap
	new_mgr->node_heap = calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));

    // check success, on error deallocate mgr/pool and return null
	if (new_mgr->node_heap == NULL){
		free(new_mgr->pool.mem);
		free(new_mgr);
		return NULL;
	}

    // allocate a new gap index
	new_mgr->gap_ix = calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));

    // check success, on error deallocate mgr/pool/heap and return null
	if (new_mgr->gap_ix == NULL){
		free(new_mgr->node_heap);
		free(new_mgr->pool.mem);
		free(new_mgr);
		return NULL;
	}

    // assign all the pointers and update meta data:
    // initialize top node of node heap
	new_mgr->node_heap[0].alloc_record.mem = new_mgr->pool.mem;
	new_mgr->node_heap[0].alloc_record.size = size;
	new_mgr->node_heap[0].next = NULL;
	new_mgr->node_heap[0].prev = NULL;
	new_mgr->node_heap[0].used = 1;
	new_mgr->node_heap[0].allocated = 0;


    // initialize top node of gap index
	_mem_add_to_gap_ix(new_mgr, size, &(new_mgr->node_heap[0]));

    // initialize pool mgr
	new_mgr->pool.alloc_size = size;
	new_mgr->pool.total_size = size;
	new_mgr->pool.policy = policy;
	new_mgr->total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
	new_mgr->used_nodes = 1;
	new_mgr->gap_ix_capacity = MEM_GAP_IX_INIT_CAPACITY;
	new_mgr->pool.num_gaps = 1;

    // link pool mgr to pool store
    pool_store_capacity ++;
	pool_store_size ++; // increment pool store size for new pool
	pool_store[pool_store_size - 1] = new_mgr;

    // return the address of the mgr, cast to (pool_pt)
	return (pool_pt)new_mgr;
}

alloc_status mem_pool_close(pool_pt pool) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
	pool_mgr_pt new_mgr = (pool_mgr_pt)pool;

    // check if this pool is allocated
	if (new_mgr != NULL)
		return ALLOC_OK;

    // check if pool has only one gap

    // check if it has zero allocations

    // free memory pool
	free(new_mgr->pool.mem);

    // free node heap
	free(new_mgr->node_heap);

    // free gap index
	free(new_mgr->gap_ix);


    // find mgr in pool store and set to null
	for (int i = 0; i < pool_store_size; i++) {
		if (pool_store[i] == new_mgr) {
			pool_store[i] = NULL;
		}
	}

    // note: don't decrement pool_store_size, because it only grows
    // free mgr
	free(new_mgr);

	return ALLOC_FAIL;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
	pool_mgr_pt new_mgr = (pool_mgr_pt)pool;

    // check if any gaps, return null if none

    // expand heap node, if necessary, quit on error
	if (_mem_resize_node_heap(new_mgr) == ALLOC_FAIL){
		exit(0);
	}
    // check used nodes fewer than total nodes, quit on error
	if (new_mgr->used_nodes > new_mgr->total_nodes) {
		exit(0);
	}

    // get a node for allocation:
	node_pt node = NULL;

    // if FIRST_FIT, then find the first sufficient node in the node heap
    size_t extraMem = 0;
	if (new_mgr->pool.policy == FIRST_FIT){
		int i=0;
		for(i=0; i < new_mgr->total_nodes; i ++){
			if (new_mgr->node_heap[i].allocated == 0){
				if ( size <= new_mgr->node_heap[i].alloc_record.size){
					node = &(new_mgr->node_heap[i]);
					break;
				}
			}
		}
	}

   // if BEST_FIT, then find the first sufficient node in the gap index
	else if(new_mgr->pool.policy == BEST_FIT){
		for (int i=0; i < new_mgr->gap_ix_capacity; i++){
			if (size < new_mgr->gap_ix[i].size){
				node = new_mgr->gap_ix[i].node;

				if(size == new_mgr->gap_ix[i].size)
					break;
			}
		}
	}

    // check if node found
    if (node == NULL){
    	return NULL;
	}

    // update metadata (num_allocs, alloc_size)
    new_mgr->pool.num_allocs ++;
    new_mgr->pool.alloc_size += size;

    // calculate the size of the remaining gap, if any
    extraMem = node->alloc_record.size - size;

    // remove node from gap index
    if (_mem_remove_from_gap_ix(new_mgr,size,node) == ALLOC_FAIL){
    	return NULL;
    }

    // convert gap_node to an allocation node of given size
	node->used = 1;
	node->allocated = 1;
	node->alloc_record.size = size;
	node->next = NULL;
	node->alloc_record.mem = malloc(size);
	if (node->alloc_record.mem == NULL){
		return NULL;
	}
    // adjust node heap:
    node_pt gap = NULL;
    //   if remaining gap, need a new node
    if (extraMem > 0){
    //   find an unused one in the node heap
    	for (int i=0; i < new_mgr->total_nodes; i ++){
    		//   make sure one was found
    		if (new_mgr->node_heap[i].used == 0){
    			//   initialize it to a gap node
    			gap = &(new_mgr->node_heap[i]);
    			break;
    		}
    	}

    	//   update metadata (used_nodes)
    	new_mgr->used_nodes ++;

    	//   update linked list (new node right after the node for allocation)
    	node->next = gap;
    	gap->prev = node;
	}

    //   add to gap index
    //   check if successful
    if(_mem_add_to_gap_ix(new_mgr, extraMem, gap) == ALLOC_FAIL){
		return NULL;
  	}

    // return allocation record by casting the node to (alloc_pt)
    return (alloc_pt)node;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
	pool_mgr_pt new_mgr = (pool_mgr_pt)pool;

    // get node from alloc by casting the pointer to (node_pt)
	node_pt node = (node_pt)alloc;
	node_pt deleteNode = NULL;
	node_pt addNode = NULL;

    // find the node in the node heap
	for(int i=0; i < new_mgr->total_nodes; i++){
		if(node == &(new_mgr->node_heap[i])){
			deleteNode = &(new_mgr->node_heap[i]);
			break;
		}
	}

    // this is node-to-delete
    // make sure it's found
    if (deleteNode == NULL){
    	return ALLOC_FAIL;
    }

    // convert to gap node
    deleteNode->allocated = 0;

    // update metadata (num_allocs, alloc_size)
    new_mgr->pool.num_allocs --;
    new_mgr->pool.alloc_size -= deleteNode->alloc_record.size;

    // if the next node in the list is also a gap, merge into node-to-delete
    	if(deleteNode->next != NULL && deleteNode->next->allocated == 0){

    //   remove the next node from gap index
    //   check success
    		if (_mem_remove_from_gap_ix(new_mgr, deleteNode->next->alloc_record.size, deleteNode->next) == ALLOC_FAIL){
    			return ALLOC_FAIL;
    		}

    //   add the size to the node-to-delete
    		deleteNode->alloc_record.size += deleteNode->next->alloc_record.size;

    //   update node as unused
    		deleteNode->next->used = 0;

    //   update metadata (used nodes)
    		new_mgr->used_nodes --;

    //   update linked list:
    		node_pt nextNode = deleteNode->next;
                    if (deleteNode->next->next) {
                        deleteNode->next->next->prev = deleteNode;
                        deleteNode->next = deleteNode->next->next;
                    } else {
                        deleteNode->next = NULL;
                    }
                    nextNode->next = NULL;
                    nextNode->prev = NULL;
         	addNode = deleteNode;
         }

    // this merged node-to-delete might need to be added to the gap index
    // but one more thing to check...
    // if the previous node in the list is also a gap, merge into previous!

    	if(deleteNode->prev != NULL && deleteNode->prev->allocated == 0){
    //   remove the previous node from gap index
    //   check success
    //   add the size of node-to-delete to the previous
    		deleteNode->prev->alloc_record.size += deleteNode->alloc_record.size;

    //   update node-to-delete as unused
    		deleteNode->used = 0;

    //   update metadata (used_nodes)
    		new_mgr->used_nodes --;

    //   update linked list
                    if (deleteNode->next) {
                        deleteNode->prev->next = deleteNode->next;
                        deleteNode->next->prev = deleteNode->prev;
                    } else {
                        deleteNode->prev->next = NULL;
                    }
                    deleteNode->next = NULL;
                    deleteNode->prev = NULL;
         	addNode = deleteNode->prev;
       	}

    //   change the node to add to the previous node!
    // add the resulting node to the gap index
    // check success
       	if(addNode != NULL){
       	 	if(_mem_add_to_gap_ix(new_mgr, addNode->alloc_record.size, addNode) == ALLOC_FAIL){
        			return ALLOC_FAIL;
        	}
		}
    return ALLOC_OK;
}

void mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments) {
    // get the mgr from the pool
	pool_mgr_pt new_mgr = (pool_mgr_pt)pool;

    // allocate the segments array with size == used_nodes
	pool_segment_pt seg = calloc(new_mgr->used_nodes, sizeof(pool_segment_t));

    // check successful
	if (seg == NULL){
		return;
	}

    // loop through the node heap and the segments array
	int tempSeg = 0;
	for (int i = 0; i < new_mgr->used_nodes; i++, tempSeg ++){
		seg[tempSeg].size = new_mgr->node_heap[i].alloc_record.size;
		seg[tempSeg].allocated = new_mgr->node_heap[i].allocated;
	}
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    *segments = seg;
    *num_segments = new_mgr->used_nodes;
}



/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
static alloc_status _mem_resize_pool_store() {

    // check if necessary
	if (((float)pool_store_size / (float)pool_store_capacity) >  MEM_POOL_STORE_FILL_FACTOR) {
		pool_mgr_pt* reallocation = (pool_mgr_pt*)realloc(pool_store, pool_store_capacity*MEM_POOL_STORE_EXPAND_FACTOR*sizeof(pool_mgr_pt));

			if (reallocation == NULL){
				return ALLOC_FAIL;
			}
		pool_store = reallocation;
	}


    // don't forget to update capacity variables
	pool_store_capacity *= MEM_POOL_STORE_EXPAND_FACTOR;

    return ALLOC_OK;
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // see above
	if (pool_mgr->used_nodes > pool_mgr->total_nodes*MEM_NODE_HEAP_FILL_FACTOR){
		node_pt node = (node_pt)realloc(pool_mgr->node_heap, pool_mgr->total_nodes*MEM_NODE_HEAP_EXPAND_FACTOR * sizeof(node_t));

		if (node != NULL) {
			pool_mgr->node_heap = node;
			pool_mgr->total_nodes *= MEM_NODE_HEAP_EXPAND_FACTOR;
		}
		else {
			return ALLOC_FAIL;
		}

	}
	return ALLOC_OK;

}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // see above
	if (pool_mgr->total_nodes*MEM_GAP_IX_FILL_FACTOR < pool_mgr->used_nodes){ // is the resize necessary?
		gap_pt reallocation = (gap_pt)realloc(pool_mgr->gap_ix, pool_mgr->gap_ix_capacity*MEM_GAP_IX_EXPAND_FACTOR*sizeof(gap_t));

		if (reallocation != NULL){
			pool_mgr->gap_ix = reallocation;
			pool_mgr->gap_ix_capacity *= MEM_GAP_IX_EXPAND_FACTOR;
		}
		else{
			return ALLOC_FAIL;
		}
	}

	return ALLOC_OK;
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node) {

    // expand the gap index, if necessary (call the function)
    int resize_check = 0;
	if (pool_mgr->gap_ix_capacity != 0)
		resize_check = 1;
	if (pool_mgr->gap_ix_capacity > MEM_GAP_IX_FILL_FACTOR)
		resize_check = 1;
	if (resize_check == 1){
		if(_mem_resize_gap_ix(pool_mgr) == ALLOC_FAIL)
			return ALLOC_FAIL;
	}


    // add the entry at the end
    node->used = 1;
    node->allocated = 0;
    node->alloc_record.size = size;
	pool_mgr->gap_ix[pool_mgr->gap_ix_capacity].node = node;
	pool_mgr->gap_ix[pool_mgr->gap_ix_capacity].size = size;

    // update metadata (num_gaps)
    pool_mgr->pool.num_gaps ++;
    pool_mgr->gap_ix_capacity ++;

    // sort the gap index (call the function)
    // check success
    return _mem_sort_gap_ix(pool_mgr);
}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node) {
    // find the position of the node in the gap index
    int findGap = 0;
    // loop from there to the end of the array:
    int foundit = 0;
    for (int i=0; i < pool_mgr->gap_ix_capacity; i ++){
    	if (pool_mgr->gap_ix[i].node == node){
    		findGap = i;
    		foundit = 1;
    		break;
    	}
    }

    if (foundit == 0){
    	return ALLOC_FAIL;
    }
    for(int i = findGap; i < pool_mgr->gap_ix_capacity-1; i ++){
    	pool_mgr->gap_ix[i] = pool_mgr->gap_ix[i+1];
    }
    //    pull the entries (i.e. copy over) one position up
    //    this effectively deletes the chosen node
    // update metadata (num_gaps)
    pool_mgr->pool.num_gaps --;
    pool_mgr->gap_ix_capacity --;

    // zero out the element at position num_gaps!
    pool_mgr->gap_ix[pool_mgr->gap_ix_capacity-1].node = NULL;
    pool_mgr->gap_ix[pool_mgr->gap_ix_capacity-1].size=0;

    return _mem_sort_gap_ix(pool_mgr);
}

// note: only called by _mem_add_to_gap_ix, which appends a single entry
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {
    // the new entry is at the end, so "bubble it up"
    // loop from num_gaps - 1 until but not including 0:
    //    if the size of the current entry is less than the previous (u - 1)
    //    or if the sizes are the same but the current entry points to a
    //    node with a lower address of pool allocation address (mem)
    //       swap them (by copying) (remember to use a temporary variable)

	for (int i = 0; i < pool_mgr->gap_ix_capacity - 1; i++){
		for (int j = 0; j < pool_mgr->gap_ix_capacity - 2; j++){
			if (pool_mgr->gap_ix[j+1].size < pool_mgr->gap_ix[j].size){
				gap_t temp = pool_mgr->gap_ix[j];
				pool_mgr->gap_ix[j] = pool_mgr->gap_ix[i];
				pool_mgr->gap_ix[i] = temp;
			}
		}
	}
    return ALLOC_OK;
}
