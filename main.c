#include <stdio.h>
#include <assert.h>

#include "mem_pool.h"

int main(int argc, char *argv[]) {

    printf("Testing C Language Programming Assignment\n");

    printf("%ld\n", __STDC_VERSION__);

    const unsigned POOL_SIZE = 1000000;
    pool_pt pool = NULL;

    alloc_status status = mem_init();
    assert(status == ALLOC_OK);
    pool = mem_pool_open(POOL_SIZE, FIRST_FIT); // TODO add policy to pool
    assert(pool);

    status = mem_pool_close(pool);
    assert(status == ALLOC_OK);
    status = mem_free();
    printf("status = %d\n", status);
    assert(status == ALLOC_OK);

    return 0;
}