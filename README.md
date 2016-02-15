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

You don't need to submit anything. Once you fork the repository (this is your **remote** repository on Github, aka **origin**), you will clone it to your development machine (this is your local repository), and start work on it. Commit your changes to your local repository often and push them up to the remote repository occasionally. Make sure you push at least once before the due date. At the due date, your remote repository will be cloned and tested automatically by the grading script. _**Note:** Your code should be in the **master** branch of your remote repository._

### Grading

An autograding script will run the test suite against your files. Your grade will be based on the number of tests passed. (E.g. if your code passes 3 out of 6 test cases, your score will be 50% and the grade will be the corresponding letter grade in the course's grading scale). **Note:** The testing and grading will be done with fresh original copies of all the provided files. In the course of development, you can modify them, if you need to, but your changes will not be used. Only your <tt>mem_pool.c</tt> file will be used.

### Compiler

Your program should run on a **C11** compatible compiler. The tests will be run on _Apple LLVM version 7.0.0 (clang-700.0.72)_.

### Due date

The assignment is due on **Sun, Feb 28, at 23:59 Mountain time**. The last commit to your PA1 repository before the deadline will be graded.

### Honor code

Free Github repositories are public so you can look at each other's code. Please, don't do that. You can discuss any programming topics and the assignments in general but sharing of solutions diminishes the individual learning experience of many people. Assignments might be randomly checked for plagiarism and a plagiarism claim may be raised against you.

Note that PA1 one is an _individual_ assignment, not a _team_ assignment like the upcoming Pintos assignments.

### Use of libraries

For this assignment, no external libraries should be used, except for the ANSI C Standard Library. The implementation of the data structures should be your own. We will use library implementations of data structures and programming primitives in the Pintos assignments.

### Coding style

Familiarize yourself with and start following [coding style guide](http://courses.cms.caltech.edu/cs11/material/c/mike/misc/c_style_guide.html). While you are not expected to follow every point of it, you should try to follow it enought to get a feel for what is good style and bad style. C code can quickly become [unreadable](http://www.ioccc.org/) and difficult to maintain.

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
   } pool_mgr_t, *pool_mgr_pt;
   ```
   **Note:** Notice that the user facing `pool_t` structure is at the top of the internal `pool_mgr_t` structure, meaning that the two structures have the same address, and the same pointer points to both. This allows the pointer to the pool received as an argument to the allocation/deallocation functions to be cast to a pool manager pointer.
   **Behavior & management:**
   1. The pool manager holds pointers to all the required metadata for the memory allocations for a single pool
   2. The functions which make allocations in a given pool have to pass the pool as their first argument.
   
4. (Linked-list) node heap _(library static)_

5. Gap index _(library static)_

6. Pool (manager) store _(library static)_

#### Static Functions

