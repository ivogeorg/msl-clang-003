/*
 * Created by Ivo Georgiev on 2/9/16.
 
 * Operating Systems Assignment 1
 * Completed by: P. Ross Baldwin
 * Metropolitan State University of Denver
 * Spring 2016
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

//The _node struct creates the doubly linked list of "allocation nodes"
typedef struct _node {
    alloc_t alloc_record;
    //Unsigned integers are always positive
    unsigned used;  //This refers to if there IS DATA in the mem location
    unsigned allocated; //This refers to if this location has been allocated in RAM, not neccessarily filled with data
    struct _node * next; // Doubly-linked list for gap deletion
    struct _node * prev; // Doubly-linked list for gap deletion
} node_t, *node_pt;

//The _gap struct defines a node which is a gap in the allocation table
typedef struct _gap {
    size_t size;
    node_pt node;
} gap_t, *gap_pt;

//The _pool_mgr struct contains a memory pool, and a pointer to the list of allocation nodes for that pool.
//  Also holds some convienience variables
typedef struct _pool_mgr {
    pool_t pool;
    node_pt node_heap;
    unsigned total_nodes;
    unsigned used_nodes;
    gap_pt gap_ix;              //POINTER
    unsigned gap_ix_capacity;
} pool_mgr_t, *pool_mgr_pt;



/***************************/
/*                         */
/* Static global variables */
/*                         */
/***************************/
//pool_store is a pointer, of type pool_mgr_pt
//In the line below, pool_store IS NOT a deref, but a pointer to an array of pointers
//The pointer is set directly to NULL
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expands
//You can apparently use array subscripting on pointers in C

static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;


/*************DEBUGGING****************/
//debug_flag == 1 turns on debugging
//debug_flag == 0 turns off debugging
static unsigned debug_flag = 1;
/*************DEBUGGING****************/


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

/*************DEBUGGING****************/
//custom helper function, debug(char * string)
//  debug function just does a printf
//  used to easily enable or disable verbose debugging
//  only prints debug lines if debug_flag == 1
void debug(const char * string){
    if(debug_flag == 1){
        printf("%s", string);
    }
}
void debug2 (const char* string, int a){
    if(debug_flag == 1){
        printf(string, a);
    }
}
/*************DEBUGGING****************/




/* TESTED IN MY OWN TESTING SUITE - PASSED TESTS */
//mem_init() creates the struct pool_mgr -> Which refers to the Node structs and alloc_t structs
//  Function returns an "alloc_status" type - ALLOC_OK if it worked, or ALLOC_CALLED_AGAIN if called before mem_free()
alloc_status mem_init() {
    debug("FUNCTION CALL: mem_init() has been called\n");
    // ensure that it's called only once until mem_free
    //Make sure the pool has not yet been instantiated
    //If the variable * pool_store IS NULL (not points to NULL or something; the VALUE of the POINTER IS NULL)
    //  , then we have not yet created the pool
    if(pool_store != NULL){
        debug("FAIL: mem_init() has been called before mem_free(), pool_store is not NULL\n");
        return ALLOC_CALLED_AGAIN;
        //If the pool is anything but null, then mem_init() has been called before mem_free()
    }
    else{
        //We know pool_store is NULL now, proceed with creation of the pool
        debug("PASS: pool_store was NULL, attempting to calloc for the pool_store\n");
        //pool_store is assigned a memory chunk here, at THIS POINT, pool_store is a pointer, and not dereferenced.
        //we allocate a chunk of memory the size of a pool_mgr struct, and cast the void * returned to a pool_mgr_pt,
        //  to match the type of pool_store (which is pool_mgr_pt).
        pool_store = (pool_mgr_pt *)calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_t));
        //calloc returns NULL if it fails
        //This allocates MEM_POOL_STORE_INIT_CAPACITY number of pool_mgr_t structs
        //MEM_POOL_STORE_INIT_CAPACITY is == 20, so pool_store goes from pool_store[0]-pool_store[19]
        if(pool_store != NULL){ //If the calloc succeeded, the value of pool_store should NO LONGER BE NULL (it is now some memory address)
            //Operation succeeded->allocate the pool store with initial capacity before returning
            pool_store_capacity = MEM_POOL_STORE_INIT_CAPACITY;  //this capacity is 20
            //Set pool_store_size here?
            //****TODO???********
            debug("PASS: pool_store struct has been successfully allocated with calloc\n");
            return ALLOC_OK; //Operation succeeded on all checks, return ALLOC_OK status
        }
    }
    // note: holds pointers only, other functions to allocate/deallocate
    debug("FAIL: Allocation of memory in mem_init() failed, calloc has failed\n");
    return ALLOC_FAIL; //If the pool_store pointer was NULL, but the calloc failed, return ALLOC_FAIL status
}



/* TESTED IN MY OWN TESTING SUITE - PASSED TESTS */
//mem_free() is called to release the pool_store list created by mem_init()
//This releases ALL of the pools in pool_store
//  If return status is ALLOC_NOT_FREED, or ALLOC_FAIL then this failed.  ALLOC_OK if it succeeded
alloc_status mem_free() {
    debug("FUNCTION CALL: mem_free() has been called\n");
    
    // ensure that it's called only once for each mem_init
    //check and make sure that pool_store is not NULL -> if it is, init() has not been called first
    if(pool_store == NULL){
        //if the pool_store is NULL, then there is nothing to mem_free(); return fail
        debug("FAIL: pool_store is NULL, mem_free must be called only after mem_init()\n");
        return ALLOC_NOT_FREED;
    }
    if(pool_store != NULL){ //If pool_store is not NULL, then it exists
        debug("PASS: pool_store has been found, attempting to close all mem pools\n");
        // make sure all pool managers have been deallocated
        //loop through the pool managers, and call mem_pool_close() on them
        //Need to compare to pool_store_capacity, to ensure all structs are deallocated
        debug2("PASS: pool_store_capacity = %u\n", pool_store_capacity);
        for(int i=0; i < pool_store_capacity; (i++)){  //iterates through pool_store
            alloc_status sts;
            //free each pool, regardless of if it has data in it or not
            //pool_store[i] is a value, and must be cast back to pointer type before being passed to mem_pool_close
            debug2("    Attempting to deallocate pool_store[%d]\n", i);
            sts = mem_pool_close((pool_pt) pool_store[i]);  //close the pool, and capture the status
            assert(sts == ALLOC_OK); //make sure each deallocation happened successfully
            debug2("PASS: pool_store[%d] has been deallocated\n", i);
        }
        // can free the pool store array now
        free(pool_store);  //frees the memory allocated to the pool_store list
    
        // update static variables
        //We have deleted everything in the pool_store, set it back to NULL
        pool_store = NULL; 
        pool_store_capacity = 0;
        pool_store_size = 0;
        
        //If we made it here, everything must have went well
        return ALLOC_OK;
    }
}


/* TESTED IN MY OWN TESTING SUITE - CURRENTLY PASSING TESTS - NEEDS ADDITIONAL TESTS */
//mem_pool_open creates a pool_t struct (heap), given a size and a fit policy
//This function returns a pool_pt, which is a pointer to the newly created pool
//The return of a NULL pointer means the function failed
pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    debug("FUNCTION CALL: mem_pool_open has been called\n");
    
    //Make sure that the pool store is allocated
    if(pool_store == NULL){
        //If the pool_store pointer is a NULL pointer, then init() has not been successfully called
        //return NULL
        debug("FAIL: mem_pool_open() has failed; pool_store is NULL, make sure init() has been called\n");
        return NULL;  //FAIL
    }
    
    //If we made it here, things are going well
    debug("PASS: An initialized pool_store has been found\n");
        //Implicitly call _mem_resize_pool_store - that method handles logic for 
        //  checking if the pool_store needs to be resized or not.
        // expand the pool store, if necessary -> _mem_resize_pool_store implements this logic
        //we are calling _mem_resize_pool_store from the if statement
        
    if(_mem_resize_pool_store() != ALLOC_OK){
        debug("FAIL: _mem_resize_pool_store() call has returned a status other than ALLOC_OK\n");
        //If the resize failed, then we want to return a NULL pointer to indicate that this
        //  method failed.
        return NULL;  //FAIL
    }
    
    // allocate a new mem pool mgr
    debug("     Attempting to calloc a new pool_mgr_t\n");
    pool_mgr_pt new_pool_mgr_pt = (pool_mgr_pt)calloc(1, sizeof(pool_mgr_t));
    // check success, on error return null
    if(new_pool_mgr_pt == NULL){
        debug("FAIL: calloc of the pool_mgr_t has failed in mem_pool_open()\n");
        //If the new pointer we just tried to calloc is NULL, then something went wrong
        //If the calloc failed, return NULL to indicate failure
        return NULL; //FAIl
    }
    
    debug("PASS: calloc of the pool_mgr_t in mem_pool_open() successful\n");
    // allocate a new memory pool
        //Line below callocs space for the new pool, and saves it to the pool
        //  variable of the our newly created pool_mgr_t
    (*new_pool_mgr_pt).pool = *(pool_pt)calloc(1, sizeof(pool_t));
        //Line below callocs space for the mem array in pool_t
        //This is a char * pointer called mem, and is saved in the pool_t struct
    (*new_pool_mgr_pt).pool.mem = (char *)calloc(1, sizeof(size));
    
    //check success of .pool and .pool.mem callocs
    //  on error deallocate mgr and return null
    if(NULL == (*new_pool_mgr_pt).pool.mem){ //IS THIS RIGHT????.(*mem)????
        //if we got here, something went wrong and .pool and .pool.mem never got allocated
        debug("FAIL: calloc of pool_mgr_t.pool or pool_mgr_t.pool.mem has failed in mem_pool_open()\n");
        //Deallocate the pool_mgr only - the other 2 were not made
        free(new_pool_mgr_pt);       //free the newly created pool_mgr_t
        debug("     pool_mgr_t created in mem_pool_open() has been freed\n");
        return NULL; //FAIL
    }
    
    //If we got here, we need to assign ALLOC_POLICY in the newly created .pool
    debug("PASS: new pool_mgr_t has been created - attempting to assign vars to pool_mgr_t\n");
        //Assign the passed in policy to our new structs pool's alloc_policy
    (*new_pool_mgr_pt).pool.policy = policy;
        //Assign the total_size var in our pool
        //alloc_policy is equal to the policy passed in as a parameter to this method
    (*new_pool_mgr_pt).pool.total_size = size;
        //Assign the alloc_size var in our pool
        //alloc_size starts at zero, as this pool is empty
    (*new_pool_mgr_pt).pool.alloc_size = 0;
        //Assign the num_gaps var in the pool
        //num_gaps starts at 1, as the entire pool at this point is a gap
    (*new_pool_mgr_pt).pool.num_gaps = 1;
    debug("PASS: values have been assigned to vars of pool_mgr_t\n");
    
    //If we got here, things are going well
    // allocate a new node heap and assign it to our pool_mgr_t struct
    debug("     Attempting to calloc space for node_heap in mem_pool_open()\n");
    (*new_pool_mgr_pt).node_heap = (node_pt)calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));
    // check success, on error deallocate mgr/pool and return null
    if(NULL == (*new_pool_mgr_pt).node_heap){  //IS THIS RIGHT???
        //If we got here, something went wrong
        debug("FAIL: calloc of the pool_mgr_t->node_heap has failed\n");
        //  If the node_heap pointer still points to null and not a valid mem space,
        //***TODO*** deallocate mgr/pool and return null
        free((*new_pool_mgr_pt).pool.mem);  //free the pool.mem
        free(new_pool_mgr_pt);              //free the newly created pool_mgr_t
        //  then this is a failure and return NULL
        return NULL;
    }
    debug("PASS: calloc for node_heap successful\n");
    
    // allocate a new gap index
    //If we got here, things are going well
    debug("     attempting to calloc space for gap_ix (gap list)\n");
    (*new_pool_mgr_pt).gap_ix = (gap_pt)calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));
    // check success, on error deallocate mgr/pool/heap and return null
    if(NULL == (*new_pool_mgr_pt).gap_ix){
        //If we got here, something has gone wrong
        debug("FAIL: calloc of gap_ix has failed in mem_pool_open()\n");
        //***TODO*** deallocate mgr/pool/heap and return null
        //free() everything calloc'd up to this point
        free((*new_pool_mgr_pt).node_heap); //free the node_heap
        free((*new_pool_mgr_pt).pool.mem);  //free the pool.mem
        free(new_pool_mgr_pt);              //free the newly created pool_mgr_t
        return NULL;
    }
    debug("PASS: calloc for gap_ix successful\n");
    
    //If we got to here, everything went correctly!
    debug("PASS: All callocs in mem_pool_open() have worked correctly\n");
    
    // assign all the pointers and update meta data:
    //   initialize top node of node heap
    debug("     attempting to calloc the first node of the node_heap\n");
    (*new_pool_mgr_pt).node_heap[0] = *(node_pt)calloc(1, sizeof(node_t));
    //****TODO**** FILL VARIABLES OF NEW NODE STRUCT
        //assign .next and .prev to NULL
    debug("     attempting to set next and prev on the first node\n");
    (*new_pool_mgr_pt).node_heap[0].next = NULL;
    (*new_pool_mgr_pt).node_heap[0].prev = NULL;
    //initialize top node of gap index
    debug("     attempting to calloc the top gap_t of gap_ix\n");
    (*new_pool_mgr_pt).gap_ix[0] = *(gap_pt)calloc(1, sizeof(gap_t));
    //****TODO**** FILL VARIABLES OF NEW GAP STRUCT
    //initialize pool mgr variables
        // unsigned total_nodes;
    (*new_pool_mgr_pt).total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
        // unsigned used_nodes;
    (*new_pool_mgr_pt).used_nodes = 1;
        // unsigned gap_ix_capacity;
    (*new_pool_mgr_pt).gap_ix_capacity = 1;
        //   link pool mgr to pool store
    debug("     attempting to assign the completed pool_mgr_t to the pool_store\n");
    pool_store[pool_store_size] = new_pool_mgr_pt;
    // return the address of the mgr, cast to (pool_pt)
    debug("     attempting to return the pointer to the new pool_mgr_t\n");
    return (pool_pt)new_pool_mgr_pt;
}



alloc_status mem_pool_close(pool_pt pool) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if this pool is allocated
    // check if pool has only one gap
    // check if it has zero allocations
    // free memory pool
    // free node heap
    // free gap index
    // find mgr in pool store and set to null
    // note: don't decrement pool_store_size, because it only grows
    // free mgr

    return ALLOC_OK;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // get mgr from pool by casting the pointer to (pool_mgr_pt)
    // check if any gaps, return null if none
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
    // allocate the segments array with size == used_nodes
    // check successful
    // loop through the node heap and the segments array
    //    for each node, write the size and allocated in the segment
    // "return" the values:
    /*
                    *segments = segs;
                    *num_segments = pool_mgr->used_nodes;
     */
}



/***********************************/
/*                                 */
/* Definitions of static functions */
/*                                 */
/***********************************/
//This method is calle from mem_pool_open()
static alloc_status _mem_resize_pool_store() {
    debug("FUNCTION CALL: _mem_resize_pool_store() has been called\n");
    
    //Check if the pool_store needs to be resized.
    //Check if necessary
    // if (((float) pool_store_size / pool_store_capacity){
    //     > MEM_POOL_STORE_FILL_FACTOR) {...};
    // } 
    //Don't forget to update capacity variables
    debug("FUNCTION NOT YET IMPLEMENTED - SENDING ALLOC_OK\n");
    //return ALLOC_FAIL;
    return ALLOC_OK;
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


