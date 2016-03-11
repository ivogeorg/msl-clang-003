#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "mem_pool.h"
#include "mem_pool.c"

/* forward declarations */
void print_pool(pool_pt pool);

/* main method, runs tests */
int main(){
    
    const unsigned POOL_SIZE = 1000000;
    pool_pt pool = NULL;
    
    
    /*
        TEST: mem_init()
    */
    printf("\nTEST MODULE 1: mem_init()\n");
    printf("\nTest: Call mem_init() -> should pass\n");
    alloc_status status = mem_init();  //this should return alloc_status == ALLOC_OK
    assert(status == ALLOC_OK); //This assertion checks that the mem_init worked, if it did, status is ALLOC_OK
    
    printf("\nTest: Call mem_init() a second time without calling mem_free() -> should fail\n");
    status = mem_init();
    assert(status == ALLOC_CALLED_AGAIN); //checks that the 2nd mem_init failed
    printf("\nTEST MODULE 1: PASSED\n");
    
    /*
        TEST: mem_free()
    */
    //Check and see if mem_free() works
    //If status is ALLOC_NOT_FREED, or ALLOC_FAILED then this failed.  
    //ALLOC_OK if it succeeded
    printf("\nTEST MODULE 2: mem_free()\n");
    printf("\nTest: Call mem_free() to deallocate the pool_store\n");
    status = mem_free();
    assert(status == ALLOC_OK);
    printf("\nTEST MODULE 2: PASSED\n");
    
    
    /*
        TEST: mem_pool_open()
    */
    printf("\nTEST MODULE 3: mem_pool_open()\n");
    //Test failure conditions
    printf("\nTest: Call mem_pool_open() before mem_init() -> should fail\n");
    pool = mem_pool_open(POOL_SIZE, FIRST_FIT);
    assert(!pool);  //This assertion checks that the mem_pool_open worked
    //Test proper function
    printf("\nTest: Call mem_pool_open() -> should pass\n");
    status = mem_init();
    assert(status == ALLOC_OK);
    pool = mem_pool_open(POOL_SIZE, FIRST_FIT);
    //Assertion checks that the mem_pool_open worked
    assert(pool);
    printf("\nTEST MODULE 3: PASSED\n");
    
    
    /*
        TEST: _mem_resize_pool_store()
    */
    printf("\nTEST MODULE 4: _mem_resize_pool_store()\n");
    //Test failure conditions
    //Test proper function
    printf("\nTest: call _mem_resize_pool_store() -> should pass\n");
    status = _mem_resize_pool_store();
    assert(status == ALLOC_OK);
    printf("\nTest: call _mem_resize_pool_store() -> should resize pool and pass\n");
    pool_store_capacity = 20;
    pool_store_size = 18;
    status = _mem_resize_pool_store();
    assert(status == ALLOC_OK);
    mem_free();
    printf("\nTEST MODULE 4: PASSED\n");
    
    //Exit test suite
    return 0; //Exit the program on code 0, ie everything went well.
}



/* function definitions */
void print_pool(pool_pt pool) {
    pool_segment_pt segs = NULL;
    unsigned size = 0;

    assert(pool);

    mem_inspect_pool(pool, &segs, &size);

    assert(segs);
    assert(size);

    for (unsigned u = 0; u < size; u ++)
        printf("%10lu - %s\n", (unsigned long) segs[u].size, (segs[u].allocated) ? "alloc" : "gap");

    free(segs);

    printf("\n");
}
