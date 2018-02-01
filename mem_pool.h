/*
 * Created by Ivo Georgiev on 2/9/16.
 */

#ifndef MEM_POOL_H
#define MEM_POOL_H

/* type declarations */

typedef enum _alloc_policy { FIRST_FIT, BEST_FIT } alloc_policy;

typedef struct _pool {
    char *mem;
    alloc_policy policy;
    size_t total_size;
    size_t alloc_size;
    unsigned num_allocs;
    unsigned num_gaps;
} pool_t, *pool_pt;

typedef struct _pool_segment {
    size_t size;
    unsigned long allocated; // 1-allocation, 0-gap (note: 8 bytes)
} pool_segment_t, *pool_segment_pt;

typedef enum _alloc_status {
    ALLOC_OK,
    ALLOC_FAIL,
    ALLOC_CALLED_AGAIN,
    ALLOC_NOT_FREED
} alloc_status;

/* function declarations */

alloc_status
mem_init();

alloc_status
mem_free();

pool_pt
mem_pool_open(size_t size, alloc_policy policy);

alloc_status
mem_pool_close(pool_pt pool);

void *
mem_new_alloc(pool_pt pool, size_t size);

alloc_status
mem_del_alloc(pool_pt pool, void *alloc);

void
mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments);
#endif //C_MEM_POOL_H
