//
// Created by Ivo Georgiev on 3/3/16.
//

#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"

#include "mem_pool.h"
#include "test_suite.h"


/*****             macros              *****/

#define INFO(...)                                     \
                            printf("[      --> ] ");  \
                            printf(__VA_ARGS__);


/*****            constants            *****/

static const unsigned NUM_TEST_ITERATIONS = NUM_ITERATIONS;
static const unsigned POOL_SIZE           = 1000000;


/*****         helper routines         *****/

static void print_pool(pool_pt pool) {
    pool_segment_pt segs = NULL;
    unsigned size = 0;

    assert_non_null(pool);

    mem_inspect_pool(pool, &segs, &size);

    assert_non_null(segs);
    assert_int_not_equal(size, 0);

#ifdef INSPECT_POOL
    for (unsigned u = 0; u < size; u ++)
        printf("%10lu - %s\n", (unsigned long) segs[u].size, (segs[u].allocated) ? "alloc" : "gap");
#endif

    if (segs) free(segs);

#ifdef INSPECT_POOL
    printf("\n");
#endif
}

static void check_pool(pool_pt pool, const pool_segment_pt exp) {
    pool_segment_pt segs = NULL;
    unsigned size = 0;

    assert_non_null(pool);

    mem_inspect_pool(pool, &segs, &size);

    assert_non_null(segs);
    assert_int_not_equal(size, 0);

#ifdef INSPECT_POOL
    for (unsigned u = 0; u < size; u ++)
        printf("%10lu - %s\n", (unsigned long) segs[u].size, (segs[u].allocated) ? "alloc" : "gap");
#endif

    assert_memory_equal(exp, segs, size * sizeof(pool_segment_t));

    if (segs) free(segs);

#ifdef INSPECT_POOL
    printf("\n");
#endif
}

static void check_metadata(pool_pt pool,
                           alloc_policy policy,
                           size_t total_size,
                           size_t alloc_size,
                           unsigned num_allocs,
                           unsigned num_gaps) {
    pool_segment_pt segs = NULL;
    unsigned size = 0;

    assert_non_null(pool);

    mem_inspect_pool(pool, &segs, &size);

    assert_non_null(segs);
    assert_int_not_equal(size, 0);

#ifdef INSPECT_POOL
    for (unsigned u = 0; u < size; u ++)
        printf("%10lu - %s\n", (unsigned long) segs[u].size, (segs[u].allocated) ? "alloc" : "gap");

    printf("%10s = %lu(%lu),\n%10s = %lu(%lu),\n%10s = %u(%u),\n%10s = %u(%u)\n",
           (char *) "total_size", pool->total_size, total_size,
           (char *) "alloc_size", pool->alloc_size, alloc_size,
           (char *) "num_allocs", pool->num_allocs, num_allocs,
           (char *) "num_gaps",   pool->num_gaps,   num_gaps);
#endif

    if (segs) free(segs);

    assert_non_null(pool);
    assert_non_null(pool->mem);
    assert_int_equal(pool->policy, policy);
    assert_in_range(pool->total_size, total_size, total_size);
    assert_in_range(pool->alloc_size, alloc_size, alloc_size);
    assert_true(pool->num_allocs == num_allocs);
    assert_true(pool->num_gaps == num_gaps);

#ifdef INSPECT_POOL
    printf("\n\n");
#endif
}



/*******************************************/
/***                                     ***/
/***          POOL TEST SUITE:           ***/
/***                                     ***/
/*******************************************/

/*******************************************/
/***          1. SMOKE TESTS             ***/
/*******************************************/

static void test_pool_store_smoketest(void **state) {
    (void) state; /* unused */

    alloc_status status;

    for (int i=0; i<NUM_TEST_ITERATIONS; i++) {
        INFO("Initializing pool store\n");
        status = mem_init();
        assert_int_equal(status, ALLOC_OK);

        status = mem_init();
        assert_int_equal(status, ALLOC_CALLED_AGAIN);

        INFO("Closing pool store\n");
        status = mem_free();
        assert_int_equal(status, ALLOC_OK);

        status = mem_free();
        assert_int_equal(status, ALLOC_CALLED_AGAIN);
    }
}

static void test_pool_smoketest(void **state) {
    (void) state; /* unused */

    unsigned pool_size = POOL_SIZE;
    alloc_policy POOL_POLICY = FIRST_FIT;
    pool_pt pool = NULL;

    alloc_status status;

    for (int i=0; i<NUM_TEST_ITERATIONS; i++) {

        POOL_POLICY = (i % 2) ? FIRST_FIT : BEST_FIT;
        pool_size *= (i + 1);

        status = mem_init();
        assert_int_equal(status, ALLOC_OK);

        INFO("Allocating pool of %lu bytes with policy %s\n",
             (long) pool_size, (POOL_POLICY == FIRST_FIT) ? "FIRST_FIT" : "BEST_FIT");
        pool = mem_pool_open(pool_size, POOL_POLICY);
        assert_non_null(pool);
        assert_non_null(pool->mem);
        assert_int_equal(pool->policy, POOL_POLICY);
        assert_int_equal(pool->total_size, pool_size);
        assert_int_equal(pool->alloc_size, 0);
        assert_int_equal(pool->num_allocs, 0);
        assert_int_equal(pool->num_gaps, 1);

        INFO("Closing pool\n");
        status = mem_pool_close(pool);
        assert_int_equal(status, ALLOC_OK);

        status = mem_free();
        assert_int_equal(status, ALLOC_OK);
    }
}

static void test_pool_nonempty(void **state) {
    (void) state; /* unused */

    unsigned pool_size = POOL_SIZE;
    alloc_policy POOL_POLICY = FIRST_FIT;
    pool_pt pool = NULL;

    alloc_status status = mem_init();
    assert_int_equal(status, ALLOC_OK);

    INFO("Allocating pool of %lu bytes with policy %s\n",
         (long) pool_size, (POOL_POLICY == FIRST_FIT) ? "FIRST_FIT" : "BEST_FIT");
    pool = mem_pool_open(pool_size, POOL_POLICY);
    assert_non_null(pool);
    assert_non_null(pool->mem);
    assert_int_equal(pool->policy, POOL_POLICY);
    assert_int_equal(pool->total_size, pool_size);
    assert_int_equal(pool->alloc_size, 0);
    assert_int_equal(pool->num_allocs, 0);
    assert_int_equal(pool->num_gaps, 1);

    INFO("Allocating 100 bytes\n");
    void * alloc = mem_new_alloc(pool, 100);
    assert_non_null(alloc);

    INFO("Trying to close pool without deallocating...\n");
    status = mem_pool_close(pool);
    assert_int_equal(status, ALLOC_NOT_FREED);
    INFO("Failed, as expected\n");

    INFO("Deallocating 100 bytes\n");
    status = mem_del_alloc(pool, alloc);
    assert_int_equal(status, ALLOC_OK);

    INFO("Closing pool\n");
    status = mem_pool_close(pool);
    assert_int_equal(status, ALLOC_OK);

    status = mem_free();
    assert_int_equal(status, ALLOC_OK);
}


/*******************************************/
/***       2. USER-FACING METADATA       ***/
/*******************************************/

static void test_pool_ff_metadata(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Uses scenario 07 w/o pool checking:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 1100.
     * 8. Deallocate the 10000.
     * 9. Deallocate 1100.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 0, 0, 1);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 100, 1, 1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 1100, 2, 1);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 11100, 3, 1);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 10100, 2, 2);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 10000, 1, 2);


    // 7. allocate 1100
    void * alloc3 = mem_new_alloc(pool, 1100);
    assert_non_null(alloc3);

    pool_segment_t exp6[3] =
            {
                    {1100, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            };
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 11100, 2, 1);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[2] =
            {
                    {1100, 1},
                    {pool->total_size-1100, 0}
            };
    check_metadata(pool, FIRST_FIT, POOL_SIZE, 1100, 1, 1);


    // 9. deallocate 1100
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_metadata(pool, FIRST_FIT, POOL_SIZE, 0, 0, 1);
}

static void test_pool_bf_metadata(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Uses scenario 17 w/o pool checking:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (2, 1, 3), (6, 5), 8
     * 4. Allocate 50.
     * 5. Allocate another 50.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_metadata(pool, BEST_FIT, POOL_SIZE, 0, 0, 1);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[3]), ALLOC_OK); allocs[3]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[5]), ALLOC_OK); allocs[5]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_metadata(pool, BEST_FIT, POOL_SIZE, 400, 4, 4);


    void * alloc0 = mem_new_alloc(pool, 50);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_metadata(pool, BEST_FIT, POOL_SIZE, 450, 5, 4);


    void * alloc1 = mem_new_alloc(pool, 50);
    assert_non_null(alloc1);
    pool_segment_t exp3[9] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 1},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_metadata(pool, BEST_FIT, POOL_SIZE, 500, 6, 3);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);
    assert_int_equal(mem_del_alloc(pool, alloc1), ALLOC_OK);


    check_metadata(pool, BEST_FIT, POOL_SIZE, 0, 0, 1);
}


/*******************************************/
/***       3. FIRST_FIT SCENARIOS        ***/
/*******************************************/

static int pool_ff_setup(void **state) {
    alloc_status status;
    const alloc_policy POOL_POLICY = FIRST_FIT;
    pool_pt pool = NULL;

    status = mem_init();
    assert_int_equal(status, ALLOC_OK);

    INFO("Allocating pool of %lu bytes with policy %s\n",
         (long) POOL_SIZE, (POOL_POLICY == FIRST_FIT) ? "FIRST_FIT" : "BEST_FIT");
    pool = mem_pool_open(POOL_SIZE, POOL_POLICY);
    assert_non_null(pool);

    *state = pool;

    return 0;
}

static int pool_ff_teardown(void **state) {
    pool_pt pool = *state;
    alloc_status status;

    INFO("Closing pool\n");
    status = mem_pool_close(pool);
    assert_int_equal(status, ALLOC_OK);

    status = mem_free();
    assert_int_equal(status, ALLOC_OK);

    return 0;
}

static void test_pool_scenario00(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 00:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Deallocate the 100 allocation. Pool is again one single gap.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };
    check_pool(pool, exp0);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            };
    check_pool(pool, exp1);


    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario01(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 01:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Deallocate the 100 allocation. Pool is again one single gap.
     * 4. Allocate 100. That will be at the top. The rest is a gap.
     * 5. Deallocate the 100 allocation. Pool is again one single gap.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };
    check_pool(pool, exp0);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            };
    check_pool(pool, exp1);


    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);


    alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    check_pool(pool, exp1);


    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}


static void test_pool_scenario02(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 02:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Allocate 1000. That will be right underneath the 100. Rest is gap.
     * 4. Deallocate the 1000. The 100 remains. Rest is gap.
     * 5. Deallocate the 100 allocation. Pool is again one single gap.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp1);


    // deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario03(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 03:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Allocate 1000. That will be right underneath the 100. Rest is gap.
     * 4. Deallocate the 100 allocation. Two gaps around the 1000.
     * 4. Deallocate the 1000. The pool is again a single gap.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp3[3] =
            {
                    {100, 0},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // one allocation of 100 w/ two gaps
    check_pool(pool, exp3);


    // deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario04(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 04:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Deallocate the 10000.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario05(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 05:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 2000.
     * 8. Deallocate the 10000.
     * 9. Deallocate 2000.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. allocate 2000
    void * alloc3 = mem_new_alloc(pool, 2000);
    assert_non_null(alloc3);

    pool_segment_t exp6[4] =
            {
                    {1100, 0},
                    {10000, 1},
                    {2000, 1},
                    {pool->total_size-100-1000-10000-2000, 0}
            };
    check_pool(pool, exp6);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[3] =
            {
                    {11100, 0},
                    {2000, 1},
                    {pool->total_size-100-1000-10000-2000, 0}
            };
    check_pool(pool, exp7);


    // 9. deallocate 2000
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario06(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 06:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 500.
     * 8. Deallocate the 10000.
     * 9. Deallocate 500.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. allocate 500
    void * alloc3 = mem_new_alloc(pool, 500);
    assert_non_null(alloc3);

    pool_segment_t exp6[4] =
            {
                    {500, 1},
                    {600, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            };
    check_pool(pool, exp6);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[2] =
            {
                    {500, 1},
                    {pool->total_size-500, 0}
            };
    check_pool(pool, exp7);


    // 9. deallocate 500
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario07(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 07:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 1100.
     * 8. Deallocate the 10000.
     * 9. Deallocate 1100.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. allocate 1100
    void * alloc3 = mem_new_alloc(pool, 1100);
    assert_non_null(alloc3);

    pool_segment_t exp6[3] =
            {
                    {1100, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            };
    check_pool(pool, exp6);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[2] =
            {
                    {1100, 1},
                    {pool->total_size-1100, 0}
            };
    check_pool(pool, exp7);


    // 9. deallocate 1100
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario08(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 08:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Try to allocate 989000. No room.
     * 8. Deallocate the 10000.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. try 989000 (should not succeed)
    void * alloc3 = mem_new_alloc(pool, 989000);
    assert_null(alloc3);

    check_pool(pool, exp5);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario09(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 09:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 988000.
     * 8. Deallocate the 10000.
     * 9. Deallocate the 988000.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. allocate 988000
    void * alloc3 = mem_new_alloc(pool, 988000);
    assert_non_null(alloc3);

    pool_segment_t exp6[4] =
            {
                    {1100, 0},
                    {10000, 1},
                    {988000, 1},
                    {pool->total_size-100-1000-10000-988000, 0}
            };
    check_pool(pool, exp6);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[3] =
            {
                    {11100, 0},
                    {988000, 1},
                    {pool->total_size-100-1000-10000-988000, 0}
            };
    check_pool(pool, exp7);


    // 9. deallocate 988000
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario10(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 09:
     *
     * 1. Pool is a gap.
     * 2. Allocate 100.
     * 3. Allocate 1000.
     * 4. Allocate 10000.
     * 5. Deallocate the 1000.
     * 6. Deallocate the 100.
     * 7. Allocate 988900.
     * 8. Deallocate the 10000.
     * 9. Deallocate the 988900.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };  // empty pool
    check_pool(pool, exp0);


    // 2. allocate 100
    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            }; // one allocation of 100
    check_pool(pool, exp1);


    // 3. allocate 1000
    void * alloc1 = mem_new_alloc(pool, 1000);
    assert_non_null(alloc1);

    pool_segment_t exp2[3] =
            {
                    {100, 1},
                    {1000, 1},
                    {pool->total_size-100-1000, 0}
            }; // two allocations: 100, 1000
    check_pool(pool, exp2);


    // 4. allocate 10000
    void * alloc2 = mem_new_alloc(pool, 10000);
    assert_non_null(alloc2);

    pool_segment_t exp3[4] =
            {
                    {100, 1},
                    {1000, 1},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // three allocations: 100, 1000, 10000
    check_pool(pool, exp3);


    // 5. deallocate 1000
    status = mem_del_alloc(pool, alloc1);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp4[4] =
            {
                    {100, 1},
                    {1000, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // two allocations 100, 10000 w/ two gaps
    check_pool(pool, exp4);


    // 6. deallocate 100
    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp5[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {pool->total_size-100-1000-10000, 0}
            }; // one allocations 10000 w/ two gaps
    check_pool(pool, exp5);


    // 7. allocate 988900
    void * alloc3 = mem_new_alloc(pool, 988900);
    assert_non_null(alloc3);

    pool_segment_t exp6[3] =
            {
                    {1100, 0},
                    {10000, 1},
                    {988900, 1},
            };
    check_pool(pool, exp6);


    // 8. deallocate 10000
    status = mem_del_alloc(pool, alloc2);
    assert_int_equal(status, ALLOC_OK);

    pool_segment_t exp7[2] =
            {
                    {11100, 0},
                    {988900, 1},
            };
    check_pool(pool, exp7);


    // 9. deallocate 988900
    status = mem_del_alloc(pool, alloc3);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

/*******************************************/
/***        4. BEST_FIT SCENARIOS        ***/
/*******************************************/

static int pool_bf_setup(void **state) {
    alloc_status status;
    const alloc_policy POOL_POLICY = BEST_FIT;
    pool_pt pool = NULL;

    status = mem_init();
    assert_int_equal(status, ALLOC_OK);

    INFO("Allocating pool of %lu bytes with policy %s\n",
         (long) POOL_SIZE, (POOL_POLICY == FIRST_FIT) ? "FIRST_FIT" : "BEST_FIT");
    pool = mem_pool_open(POOL_SIZE, POOL_POLICY);
    assert_non_null(pool);

    *state = pool;

    return 0;
}

static int pool_bf_teardown(void **state) {
    pool_pt pool = *state;
    alloc_status status;

    INFO("Closing pool\n");
    status = mem_pool_close(pool);
    assert_int_equal(status, ALLOC_OK);

    status = mem_free();
    assert_int_equal(status, ALLOC_OK);

    return 0;
}

static void dummy_test(void **state) {
    (void) state;
}

static void test_pool_scenario11(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 11:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 100. That will be at the top. The rest is a gap.
     * 3. Deallocate the 100 allocation. Pool is again one single gap.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0}
            };
    check_pool(pool, exp0);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);

    pool_segment_t exp1[2] =
            {
                    {100, 1},
                    {pool->total_size-100, 0}
            };
    check_pool(pool, exp1);


    status = mem_del_alloc(pool, alloc0);
    assert_int_equal(status, ALLOC_OK);

    check_pool(pool, exp0);
}

static void test_pool_scenario12(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 12:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (1, 2), 4, (6, 7, 8)
     * 4. Allocate 100.
     * 5. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[4]), ALLOC_OK); allocs[4]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[7]), ALLOC_OK); allocs[7]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);
    pool_segment_t exp2[8] =
            {
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 1},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario13(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 13:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate 1, 4, (6, 7, 8)
     * 4. Allocate 100.
     * 5. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[4]), ALLOC_OK); allocs[4]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[7]), ALLOC_OK); allocs[7]=0;

    pool_segment_t exp1[9] =
            {
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {100, 1},
                    {100, 1},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario14(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 14:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate 4, 1, (6, 7, 8) (note: 4 before 1)
     * 4. Allocate 100.
     * 5. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[4]), ALLOC_OK); allocs[4]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[7]), ALLOC_OK); allocs[7]=0;

    pool_segment_t exp1[9] =
            {
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 100);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {100, 1},
                    {100, 1},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario15(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 15:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (1, 2), 4, (6, 7, 8)
     * 4. Allocate 50.
     * 5. Allocate another 50.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[4]), ALLOC_OK); allocs[4]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[7]), ALLOC_OK); allocs[7]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 50);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 0},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    void * alloc1 = mem_new_alloc(pool, 50);
    assert_non_null(alloc1);
    pool_segment_t exp3[9] =
            {
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 1},
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp3);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);
    assert_int_equal(mem_del_alloc(pool, alloc1), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario16(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 16:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate 7, 5, 3, 1
     * 4. Allocate 50.
     * 5. Allocate another 50.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[7]), ALLOC_OK); allocs[7]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[5]), ALLOC_OK); allocs[5]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[3]), ALLOC_OK); allocs[3]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;

    pool_segment_t exp1[11] =
            {
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 50);
    assert_non_null(alloc0);
    pool_segment_t exp2[12] =
            {
                    {100, 1},
                    {50, 1},
                    {50, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    void * alloc1 = mem_new_alloc(pool, 50);
    assert_non_null(alloc1);
    pool_segment_t exp3[12] =
            {
                    {100, 1},
                    {50, 1},
                    {50, 1},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp3);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);
    assert_int_equal(mem_del_alloc(pool, alloc1), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario17(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 17:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (2, 1, 3), (6, 5), 8
     * 4. Allocate 50.
     * 5. Allocate another 50.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[3]), ALLOC_OK); allocs[3]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[5]), ALLOC_OK); allocs[5]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 50);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    void * alloc1 = mem_new_alloc(pool, 50);
    assert_non_null(alloc1);
    pool_segment_t exp3[9] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {50, 1},
                    {50, 1},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp3);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);
    assert_int_equal(mem_del_alloc(pool, alloc1), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario18(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 18:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (2, 1, 3), (6, 5), 8
     * 4. Allocate 999000.
     * 5. Try to Allocate 350. Should fail.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[3]), ALLOC_OK); allocs[3]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[5]), ALLOC_OK); allocs[5]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 999000);
    assert_non_null(alloc0);
    pool_segment_t exp2[8] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 1},
            };
    check_pool(pool, exp2);


    void * alloc1 = mem_new_alloc(pool, 350);
    assert_null(alloc1);
    check_pool(pool, exp2);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);


    check_pool(pool, exp0);
}

static void test_pool_scenario19(void **state) {
    alloc_status status;
    pool_pt pool = *state;

    /*
     * Scenario 19:
     *
     * 1. Pool starts out as a single gap.
     * 2. Allocate 10 x 100.
     * 3. Deallocate (2, 1, 3), (6, 5), 8
     * 4. Allocate 150.
     * 6. Clean up.
     */

    pool_segment_t exp0[1] =
            {
                    {pool->total_size, 0},
            };
    check_pool(pool, exp0);


    const unsigned NUM_ALLOCS = 10;

    void * *allocs = (void * *) calloc(NUM_ALLOCS, sizeof(void *));
    assert_non_null(allocs);

    for (int i=0; i<NUM_ALLOCS; ++i) {
        allocs[i] = mem_new_alloc(pool, 100);
        assert_non_null(allocs[i]);
    }
    assert_int_equal(mem_del_alloc(pool, allocs[2]), ALLOC_OK); allocs[2]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[1]), ALLOC_OK); allocs[1]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[3]), ALLOC_OK); allocs[3]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[6]), ALLOC_OK); allocs[6]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[5]), ALLOC_OK); allocs[5]=0;
    assert_int_equal(mem_del_alloc(pool, allocs[8]), ALLOC_OK); allocs[8]=0;

    pool_segment_t exp1[8] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {200, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp1);


    void * alloc0 = mem_new_alloc(pool, 150);
    assert_non_null(alloc0);
    pool_segment_t exp2[9] =
            {
                    {100, 1},
                    {300, 0},
                    {100, 1},
                    {150, 1},
                    {50, 0},
                    {100, 1},
                    {100, 0},
                    {100, 1},
                    {pool->total_size - 1000, 0},
            };
    check_pool(pool, exp2);


    // clean up
    for (int i=0; i<NUM_ALLOCS; ++i) {
        if (allocs[i])
            assert_int_equal(mem_del_alloc(pool, allocs[i]), ALLOC_OK);
    }
    free(allocs);
    assert_int_equal(mem_del_alloc(pool, alloc0), ALLOC_OK);


    check_pool(pool, exp0);
}

/*******************************************/
/***        5. STRESS TESTING            ***/
/*******************************************/

void test_pool_stresstest0(void **state) {
    (void) state; /* unused */

    const unsigned num_pools = 200;
    const unsigned num_allocations = 1000;
    const unsigned min_alloc_size = 10;
    const unsigned pool_size =
            (num_allocations / 2) *
            (2 * min_alloc_size + (num_allocations - 1) * min_alloc_size);
    assert_int_equal(pool_size, 5005000);


    pool_pt pools[num_pools];
    void *allocations[num_pools][num_allocations];

    /*
     * Testing dynamic reallocation of pool structures:
     *
     * 1. 200 pools of 5005000 each (many pools)
     * 2. In each pool 1000 allocations of different sizes (many allocations)
     * 3. In each pool 500 deallocations (many gaps)
     */

    // initialize store
    assert_int_equal(mem_init(), ALLOC_OK);

    // allocate pools
    for (unsigned pix=0; pix < num_pools; ++pix) {
        // open pool
        pools[pix] =
                mem_pool_open(pool_size, (pix % 2) ? FIRST_FIT : BEST_FIT);
        assert_non_null(pools[pix]);
        // allocate pool
        unsigned allocated = 0;
        for (unsigned aix=0; aix < num_allocations; ++aix) {
            allocations[pix][aix] =
                    mem_new_alloc(pools[pix], (aix + 1) * min_alloc_size);
            allocated += (aix + 1) * min_alloc_size;
            if (!allocations[pix][aix]) {
                INFO("ASSERT WILL FAIL at pix = %u, aix = %u, allocated = %u\n", pix, aix, allocated);
            }
            assert_non_null(allocations[pix][aix]);
        }

        // delete every other allocation
        for (unsigned aix=0; aix < num_allocations; ++aix) {
            if (aix % 2) {
                assert_int_equal(
                        mem_del_alloc(pools[pix], allocations[pix][aix]),
                        ALLOC_OK);
                allocations[pix][aix] = NULL;
            }
        }
    }

    // delete pools
    for (unsigned pix=0; pix < num_pools; ++pix) {
        // delete pool's allocations
        for (unsigned aix=0; aix < num_allocations; ++aix) {
            if (allocations[pix][aix]) {
                // delete allocation
                assert_int_equal(
                        mem_del_alloc(pools[pix], allocations[pix][aix]),
                        ALLOC_OK);
            }
        }
        // close pool
        assert_int_equal(mem_pool_close(pools[pix]), ALLOC_OK);
    }

    // free store
    assert_int_equal(mem_free(), ALLOC_OK);
}


/*******************************************/
/***         6. DRIVER ROUTINE           ***/
/*******************************************/

int run_test_suite() {
    const struct CMUnitTest tests[] = {
            // General tests
            cmocka_unit_test(test_pool_store_smoketest),
            cmocka_unit_test(test_pool_smoketest),

            cmocka_unit_test(test_pool_nonempty),

            cmocka_unit_test_setup_teardown(test_pool_ff_metadata, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_bf_metadata, pool_bf_setup, pool_bf_teardown),

            // First-fit tests
            cmocka_unit_test_setup_teardown(test_pool_scenario00, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario01, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario02, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario03, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario04, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario05, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario06, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario07, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario08, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario09, pool_ff_setup, pool_ff_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario10, pool_ff_setup, pool_ff_teardown),

            // Best-fit tests
            cmocka_unit_test_setup_teardown(test_pool_scenario11, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario12, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario13, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario14, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario15, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario16, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario17, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario18, pool_bf_setup, pool_bf_teardown),
            cmocka_unit_test_setup_teardown(test_pool_scenario19, pool_bf_setup, pool_bf_teardown),

            // Stress tests
            cmocka_unit_test(test_pool_stresstest0),
    };

    return cmocka_run_group_tests_name("pool_test_suite", tests, NULL, NULL);
}

// TODO
// cmocka memory leak testing
// flush stdout before stderr?