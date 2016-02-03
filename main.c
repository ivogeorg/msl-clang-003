#include <stdio.h>

#include "mem_alloc.h"

int main(int argc, char *argv[]) {

    printf("Testing C Language Programming Assignment\n");

    const int pool_size = 1024;

    if (init_alloc_pool(pool_size, FIRST_FIT))
        printf("Initialized a pool of %d bytes\n", pool_size);
    else
        printf("Failed to initialize pool\n");

    printf("Closing pool...");
    close_pool();
    printf("Done!\n");

    printf("sizeof(size_t) = %ld\n", (long) sizeof(size_t));
    printf("sizeof(alloc_node*) = %ld\n", (long) sizeof(char*));

    return 0;
}