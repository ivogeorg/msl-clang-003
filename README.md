## Operating Systems Concepts: Programming Assignment 1

_dynamic memory management with the C language_

* * *

### Goals

1. Practice development in the C programming language.
2. Learn to manage (allocate/deallocate) dynamic memory (aka memory on the heap/free store).
3. Get a feel for the memory management overhead in an operating system.
4. Get a basic understanding of the operation of `malloc` and the heap.
5. Practice working with a variety of data structures.
6. Practice programming with pointers.
7. Prepare for the development of the [Pintos](http://pintos-os.org/) projects.
8. Develop good coding style.

### Synopsis

PA1 asks you to implement a memory pool manager which allocates dynamic memory blocks from a set pre-allocated region, mimicking the functionality of the C Standard Library function `malloc()`. You are given a header file `mem_pool.h` and a source file `mem_pool.c`. You are not supposed to change the header file and are free to use the infrastructure in the source file to implement the user-facing functions in the header, or come up with your own.

PA1 is an assignment in the test-driven development (TDD) style. The provided `main.c` driver file executes a suite of unit tests against your implementation, and your score is equal to the ratio of the number of successfully passed tests and the total number of tests. The tests use the [cmocka](https://cmocka.org/) unit test framework.

### Submission

You need to submit on the course's chosen learning management system the _url_ of your remote Github repository by the assignment deadline. 

Once you fork the repository (this is your **remote** repository on Github, aka **origin**), you will clone it to your development machine (this is your local repository), and start work on it. Commit your changes to your local repository often and push them up to the remote repository occasionally. Make sure you push at least once before the due date. At the due date, your remote repository will be cloned and tested automatically by the grading script. _**Note:** Your code should be in the **master** branch of your remote repository._

### Grading

An autograding script will run the test suite against your files. Your grade will be based on the number of tests passed. (E.g. if your code passes 3 out of 6 test cases, your score will be 50% and the grade will be the corresponding letter grade in the course's grading scale). **Note:** The testing and grading will be done with fresh original copies of all the provided files. In the course of development, you can modify them, if you need to, but your changes will not be used. Only your <tt>mem_pool.c</tt> file will be used.

### Compiler

Your program should run on a **C11** compatible compiler. Use `gcc` on a Linux server for your school. The test will be run there for grading.

### Due date

The assignment is due on **Sun, Mar 13, at 23:59 Mountain time**. The last commit to your PA1 repository before the deadline will be graded.

### Honor code

Free Github repositories are public so you can look at each other's code. Please, don't do that. You can discuss any programming topics and the assignments in general but sharing of solutions diminishes the individual learning experience of many people. Assignments might be randomly checked for plagiarism and a plagiarism claim may be raised against you.

Note that PA1 one is an _individual_ assignment, not a _team_ assignment like the upcoming Pintos assignments.

### Use of libraries

For this assignment, no external libraries should be used, except for the ANSI C Standard Library. The implementation of the data structures should be your own. We will use library implementations of data structures and programming primitives in the Pintos assignments.

### Coding style

Familiarize yourself with and start the following [coding style guide](http://courses.cms.caltech.edu/cs11/material/c/mike/misc/c_style_guide.html). While you are not expected to follow every point of it, you should try to follow it enought to get a feel for what is good style and bad style. C code can quickly become [unreadable](http://www.ioccc.org/) and difficult to maintain.

### References

A minimal [C Reference](https://cs50.harvard.edu/resources/cppreference.com/), which should be sufficient for your needs.

The [C98 Library Reference](https://www-s.acm.illinois.edu/webmonkeys/book/c_guide/) is more complete.

The [C11 Standard](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) is just provided for completeness, and you shouldn't need to read it, except peruse it out of curiosity.

Two guides for implementation of `malloc()`: [here](http://danluu.com/malloc-tutorial/) and [here](http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf).

### Detailed Instructions

The memory pool will work roughly like the dynamic memory management functions `malloc, calloc, realloc, free`. Unlike the `*alloc` functions, 
  * the metadata for allocated blocks will be kept in a separate dynamic memory section;
  * there will be multiple independent memory pool to allocate from;
  * the return value is a pointer to a `struct` which contains the memory pointer, rather than the pointer itself;
  * there is less hiding of (some of) the allocation metadata, to help with debugging and testing.

#### API Functions

1. `alloc_status mem_init();`

   This function should be called first and called only once until a corresponding `mem_free()`. It initializes the memory pool (manager) store, a data structure which stores records for separate memory pools.

2. `alloc_status mem_free();`

   This function should be called last and called only once for each corresponding `mem_init()`. It frees the pool (manager) store memory.

3. `pool_pt mem_pool_open(size_t size, alloc_policy policy);`

   This function allocates a single memory pool from which separate allocations can be performed. It takes a `size` in bytes, and an allocation policy, either `FIRST_FIT` or `BEST_FIT`.

4. `alloc_status mem_pool_close(pool_pt pool);`

   This function deallocates a single memory pool.

5. `alloc_pt mem_new_alloc(pool_pt pool, size_t size);`

   This function performs a single allocation of `size` in bytes from the given memory pool. Allocations from different memory pools are independent. 

6. `alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc);`

   This function deallocates the given allocation from the given memory pool.

7. `void mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments);`

   This function returns a new dynamically allocated array of the pool `segments` (allocations or gaps) in the order in which they are in the pool. The number of segments is returned in `num_segments`. The caller is responsible for freeing the array
   
   **Note:** Fixed bug in signature: `segments` was a single pointer, and has to be double. Fixed and updated in code.


#### Data Structures

1. Memory pool _(user facing)_

   This is the data structure a pointer to which is returned to the user by the call to `mem_pool_open`. The pointer to the allocated memory and the policy are contained in the structure, along with some allocation metadata. The user passes the pointer to the structure to the allocation/deallocation functions `mem_new_alloc` and `mem_del_alloc`. The user is not responsible for deallocating the structure.

   **Structure:**
   ```c
   typedef struct _pool {
      char *mem;
      alloc_policy policy;
      size_t total_size;
      size_t alloc_size;
      unsigned num_allocs;
      unsigned num_gaps;
   } pool_t, *pool_pt;
   ```
   
   **Behavior & management:**
   1. Passed to all functions that open, allocate on, dealocate from, and close a pool.
   2. The metadata contained in the structure is used by the library, so should not be overwritten by the user. It is provided for testing and debugging.

2. Allocation record _(user facing)_

   This is the data structure a pointer to which is returned to the user for each new allocation from a given pool. Again, the pointer to the allocated memory is in this structure along with the allocated size. The user passes the pointer to the structure and the pointer to the structure of the containing memory pool to the deallocation function `mem_del_alloc`.
The user is not responsible for deallocating the structure.

   **Structure:**
   ```c
   typedef struct _alloc {
      size_t size;
      char *mem;
   } alloc_t, *alloc_pt;
   ```
   
   **Behavior & management:**
   1. Passed to the functions that allocate on and dealocate from a given pool.
   2. **Note:** A pointer to an allocation structure (aka allocation record) is the same as the pointer to the allocated memory!

3. Pool manager _(library static)_

   This is a datastructure that the `mem_pool` library uses to store the private metadata for a single memory pool. It is hidden to the user.

   **Structure:**
   ```c
   typedef struct _pool_mgr {
      pool_t pool;
      node_pt node_heap;
      unsigned total_nodes;
      unsigned used_nodes;
      gap_pt gap_ix;
      unsigned gap_ix_capacity;
   } pool_mgr_t, *pool_mgr_pt;
   ```
   **Note:** Notice that the user facing `pool_t` structure is at the top of the internal `pool_mgr_t` structure, meaning that the two structures have the same address, and the same pointer points to both. This allows the pointer to the pool received as an argument to the allocation/deallocation functions to be cast to a pool manager pointer.

   **Behavior & management:**
   1. The pool manager holds pointers to all the required metadata for the memory allocations for a single pool
   2. The functions which make allocations in a given pool have to pass the pool as their first argument.
   3. The `gap_ix_capacity` is the capacity of the gap index and used to test if the index has to be expanded. If the index is expanded, `gap_ix_capacity` is updated as well.
   
4. (Linked-list) node heap _(library static)_

   This is _packed_ linked list which holds nodes for all the segments (allocations or gaps) in a pool, in ascending order by memory address. That is, the first node is always going to point to the segment that starts at the beginning of the pool. This data structure is hidden from the user, except that the `num_allocs` and `num_gaps` variables in the user-facing `pool_t` structure are in sync with the node heap.
   
   **Structure:**
   ```c
   typedef struct _node {
      alloc_t alloc_record;
      unsigned used;
      unsigned allocated;
      struct _node *next, *prev; // doubly-linked list for gap deletion
   } node_t, *node_pt;
   ```
   **Behavior & management:**
   1. This is a linked list allocated as an array of `node__t` structures. If a node has `used` set to 1, it is part of the list; otherwise, it is an unused node which can be used for a new allocation.
   2. The first node is always present and should always point to the top segment of the pool, regardless of the type of segment (allocation or gap).
   2. An active list node (`used == 1`) is either an allocation (`allocated == 1`) or a gap (`allocated == 0`).
   3. The list is doubly-linked to simplify the deallocation of an allocated sector between two gap sectors.
   4. **Note:** Notice that the user-facing allocation record (of type `alloc_t`) is on top of the internal `node_t`, so they have the same address and a pointer to the one points to the other. Of course, the pointer has to be cast to the proper type. For example, the the `alloc_pt` passed by the user as an argument to the `mem_new_alloc` and `mem_del_alloc` has to be cast to `node_pt` before operating with the corresponding linked-list node.
   5. The linked list is initialized with a certain capacity. If necessary, it should be resized with `realloc()`. See the corresponding `static` function and constants in the source file.
   
5. Gap index _(library static)_

   This is a simple array of `gap_t` structures which holds an element for each gap that exists in a given pool and is sorted in an ascending order by size.
   
   **Structure:**
   ```c
   typedef struct _gap {
      size_t size;
      node_pt node;
   } gap_t, *gap_pt;
   ```
   **Behavior & management:**
   1. The gap entries hold the `size` of the gaps and point to the corresponding nodes in the node heap linke list.
   2. The array is initialized with a certain capacity. If necessary, it should be resized with `realloc()`. See the corresponding `static` function and constants in the source file.
   3. Use the `num_gaps` variable in the user-facing `pool_t` structure as the size of the array and keep it updated.
   4. When deleting entries from the array, pull up the entried that follow and update the size. See the corresponding `static` function.
   5. When adding entries to the array, add at the bottom. See the corresponding `static` function.
   6. There is a separate `static` function for sorting the array.

6. Pool (manager) store _(library static)_

   This is an array of pointers to `pool_mgr_t` structures and so holds the metadata for multiple pools. See the corresponding `static` variables and functions.
   
   **Behavior & management:**
   1. The array is initialized with a certain capacity. If necessary, it should be resized with `realloc()`. See the corresponding `static` function and constants in the source file.
   2. Since this array contains pointers, they can be `NULL`. The size of the array, for which a `static` variable is used, should be incremented when a new pool is opened and **never** decremented. The pointer to a new pool should always be added to the end of the array. When a pool is closed, the pointer should be set to `NULL`. 

7. Pool segment _(user facing)_

   This is a simple structure which represents a pool segment, either an allocation or a gap. Used for pool inspection by the user.
   
   **Structure:**
   ```c
   typedef struct _pool_segment {
      size_t size;
      unsigned allocated;
   } pool_segment_t, *pool_segment_pt;
   ```
   
   **Behavior & management:**
   1. An array of such structures is returned by the function `mem_inspect_pool()` for testing, printing, and debugging.
   2. **Note:** The returned array should be freed by the user.

#### Static Functions

The following functions are internal to the library and not exposed to the user. Their names are self-explanatory.

1. `static alloc_status _mem_resize_pool_store();`

   If the pool store's size is within the fill factor of its capacity, expand it by the expand factor using `realloc()`.

2. `static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);`

   If the node heap's size is within the fill factor of its capacity, expand it by the expand factor using `realloc()`.

3. `static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);`

   If the gap index's size is within the fill factor of its capacity, expand it by the expand factor using `realloc()`.

4. `static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);`

   Add a new entry to the gap index. The entry is gap `size` and `node` pointer to a node on the node heap of the given `pool_mgr`.

5. `static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);`

   Remove an entry from the gap index. The entry is gap `size` and `node` pointer to a node on the node heap of the given `pool_mgr`.

6. `static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);`

   Sort the gap index in ascending order by size.
   **Note:** The index always has a length equal to the number of gaps currently in the corresponding pool.

#### Static Variables

The following variables are internal to the library and not exposed to the user. Their names are self-explanatory. They are used to hold the _pool store_ array of pointers to `pool_mgr_t` structures and are manipulated by the user-facing functions `mem_init()`, `mem_pool_open()`, `mem_pool_close()`, and `mem_free()`, and the library static function `_mem_resize_pool_store()`.

```c
static pool_mgr_pt *pool_store = NULL;
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;
```

* * *

### TODO

_this section concerns future editions of the project_

1. Redesign/refactor to return the _memory allocation address (mem)_ to the user from `mem_new_alloc` instead of the allocation record address. The allocation record is embedded in the linked list node, so when the node heap is reallocated, the nodes' (and, thus, the allocation records') addresses shift. The internal infrastructure only requires an adjustment of the linked list pointers and the gap index node pointers, but the allocation record addresses the user has are invalidated. So _mem_ should be returned and not _alloc_.

