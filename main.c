#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "mem_pool.h"

/* forward declarations */
void print_pool(pool_pt pool);

/* main */
int main(int argc, char *argv[]) {

    const unsigned POOL_SIZE = 1000000;
    pool_pt pool = NULL;

    alloc_status status = mem_init();
    assert(status == ALLOC_OK);
    pool = mem_pool_open(POOL_SIZE, FIRST_FIT);
    assert(pool);

    /*
     * Basic allocation scenario:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Allocate 1000. That will be underneath it. The rest is a gap.
     * 4. Deallocate the 100 allocation. Now gaps on both sides of 1000.
     * 4. Deallocate the 1000 allocation. Pool is again one single gap.
     */


    print_pool(pool);

    // + alloc-0
    alloc_pt alloc0 = mem_new_alloc(pool, 100);
    assert(alloc0);

    print_pool(pool);

    // + alloc-1
    alloc_pt alloc1 = mem_new_alloc(pool, 1000);
    assert(alloc1);

    print_pool(pool);

    // - alloc-0
    status = mem_del_alloc(pool, alloc0);
    assert(status == ALLOC_OK);

    print_pool(pool);

    // - alloc-1
    status = mem_del_alloc(pool, alloc1);
    assert(status == ALLOC_OK);

    print_pool(pool);


    /*
     * End of allocation scenario. Clean up.
     */


    status = mem_pool_close(pool);
    assert(status == ALLOC_OK);
    status = mem_free();

    printf("status = %d\n", status);
    assert(status == ALLOC_OK);

    return 0;
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
