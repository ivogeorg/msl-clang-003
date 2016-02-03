//
// Created by Ivo Georgiev on 1/31/16.
//

#ifndef DENVER_OS_PA_C_MEM_ALLOC_H
#define DENVER_OS_PA_C_MEM_ALLOC_H

struct _alloc_rec {
    char *mem;
    size_t size;
};
typedef struct _alloc_rec alloc_rec;

typedef enum { FIRST_FIT, BEST_FIT } alloc_policy;
typedef enum { ALLOC_FAILURE = 0, ALLOC_SUCCESS = 1 } alloc_status;

// alloc_mem
// : initializes a pool of memory to allocate from
// arg: size - the total size of the pool
// arg: policy - the allocation policy for the pool
// ret:  0 - success
//      -1 - failure
int init_alloc_pool(size_t size, alloc_policy policy);

// alloc_mem
// : allocates a block of memory
// arg: size - the number of bytes to allocate
// ret: pointer to a dynamic allocation record (freed by dealloc_mem)
//      NULL on failure
alloc_rec *alloc_mem(size_t size);

// dealloc_mem
// : deallocates a block of memory
// arg: alloc - pointer to the allocation record for the block (freed by the function)
// ret:  0 - success
//      -1 - failure
int dealloc_mem(alloc_rec *arec);

// close_pool
// : closes the memory pool
// ret:  0 - success
//      -1 - failure
int close_pool();

#endif //DENVER_OS_PA_C_MEM_ALLOC_H
