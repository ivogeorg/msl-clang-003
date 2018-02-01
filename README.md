Table of Contents
=================

* [C Programming Assignment 3](#c-programming-assignment-3)
  * [Goals](#goals)
  * [Synopsis](#synopsis)
  * [Submission](#submission)
    * [Github](#github)
    * [Canvas](#canvas)
  * [Grading](#grading)
    * [Bonus](#bonus)
    * [Note on points](#note-on-points)
  * [Compiler](#compiler)
  * [Due date](#due-date)
  * [Honor code](#honor-code)
  * [Use of libraries](#use-of-libraries)
  * [Coding style](#coding-style)
  * [References](#references)
  * [Detailed Instructions](#detailed-instructions)
    * [Prerequisites](#prerequisites)
    * [Overall goal](#overall-goal)
    * [API Functions](#api-functions)
    * [Data Structures](#data-structures)
    * [Static Functions](#static-functions)
    * [Static Variables](#static-variables)
  * [TODO](#todo)

# C Programming Assignment 3

_dynamic memory management with the C language_

* * * 

_Note: `cmocka` has to be built installed separately and dynamically linked to the project. You can find directions in [os-playground](https://github.com/ivogeorg/os-playground/blob/master/cmocka-mem-pool.md). The `CMakeLists.txt` has entries for Linux (Ubuntu) and MacOS. On Windows with Cigwin/MinGW, it should be similar but the path has to be adjusted. If someone figures it out, please [create a pull request](https://help.github.com/articles/creating-a-pull-request/) for the addition._

* * *

## Goals

1. Practice development in the C programming language.
2. Learn to manage (allocate/deallocate) dynamic memory (aka memory on the heap/free store).
3. Get a feel for the memory management overhead in an operating system.
4. Get a feel for the design tradeoffs in the design and implementation of an operating system.
5. Get a basic understanding of the operation of `malloc` and the heap.
6. Practice working with a variety of data structures.
7. Practice programming with pointers.
8. Prepare for the development of the [Pintos](http://pintos-os.org/) projects.
9. Develop good coding style.

## Synopsis

C PA 3 asks you to implement a library for a memory pool manager which allocates dynamic memory blocks from a set pre-allocated region, mimicking the functionality of the C Standard Library function `malloc()`. You are given a header file (the user API) `mem_pool.h` and a source file `mem_pool.c` with data structures for a suggested implementation. You are not supposed to change the header file and are free to use the infrastructure in the source file to implement the user-facing functions in the header, or come up with your own.

C PA 3 is an assignment in the test-driven development (TDD) style. The provided `main.c` driver file executes a suite of unit tests against your implementation, and your score depends on how many tests your code passes. The tests use the [cmocka](https://cmocka.org/) unit-test framework.

## Submission

You need to fork this repository and `git clone` it to your development environment. When you are done and your code works, `git commit` all your changes and `git push` to your forked (aka **remote**) repository. Work in the **master** branch.

### Github
Submissions are **one** per team. If you haven't done so, create a git account for your team. The name should look like **ucd-[course]-[team-color]-s18**. For example, **ucd-os-orange-s18** or **ucd-unix-black-s18**. (If you want, you can create an [organization](https://github.com/blog/674-introducing-organizations) but that might be overkill.) Make all team submissions from this account. 

### Canvas
Both team members have to make an assignment submission on [Canvas](https://canvas.instructure.com/courses/1270192) before the deadline. You only need to enter the _clone URL_ of your project repository (e.g. https://github.com/ivogeorg/msl-clang-001.git).

## Grading
Max points: **250**
You get 10 points for each test passed, for a maximum total of 250 for the _full regular submission__. 

### Bonus
Bonus tasks may be scored differently from regular submission.
1. Implement data structure growth. Max points: **100**
2. Pass stress test. Max points: **50**
3. Beat time of reference implementation. Max points: **50**
4. Design a plausible way to perform _bounds checking_ on allocation usage and prevent _overwriting_. Max points: **50**
5. Implement the library with (4). Max points: **250**

### Note on points
Points for regular and bonus tasks are all cumulative for your course grade.

## Compiler

Your program should run on a **C11** compatible compiler.

## Due date

The assignment is due on **Sun, Feb 11, 2018, by 23:59 Mountain time**. The last commit to your C PA 3 repository before the deadline will be graded.

## Honor code

Free Github repositories are public so you can look at each other's code. Please, don't do that. You can discuss any programming topics and the assignments in general but sharing of solutions diminishes the individual learning experience of others. Assignments might be checked for plagiarism without warning and a plagiarism claim may be raised against you.

## Use of libraries

For this assignment, no external libraries should be used, except for the ANSI C Standard Library. The implementation of the data structures should be your own. We will use library implementations of data structures and programming primitives in the Pintos assignments.

## Coding style

Familiarize yourself with and start the following [coding style guide](http://courses.cms.caltech.edu/cs11/material/c/mike/misc/c_style_guide.html). While you are not expected to follow every point of it, you should try to follow it enought to get a feel for what is good style and bad style. C code can quickly become [unreadable](http://www.ioccc.org/) and difficult to maintain.

## References

The [C Reference](http://en.cppreference.com/w/c), which you should get confortable consulting.

The [ISO C Standards](http://www.iso-9899.info/wiki/The_Standard) defines the language. A freely available draft [C11 Standard](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf), if you want to dig deep.

This [C98 Library Reference](https://www-s.acm.illinois.edu/webmonkeys/book/c_guide/) seems to be the standard reference. You should not expect many changes, though it's always good to work off a latest copy of your library reference, which should be available through the vendor/implementor.

The following compendia of frequently asked questions about C are excellent resources for the dusty corners, non-sequiturs, and best practices in C programming:

1. [C Programming FAQs](http://c-faq.com/index.html)
2. [FAQs at CProgramming.com](https://faq.cprogramming.com/cgi-bin/smartfaq.cgi)

A long list of [C/C++ software resources](https://github.com/fffaraz/awesome-cpp).

Two guides for implementation of `malloc()`: [here](http://danluu.com/malloc-tutorial/) and [here](http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf).

## Detailed Instructions

### Prerequisites

1. An installation as in [cmocka-mem-pool](https://github.com/ivogeorg/os-playground/edit/master/cmocka-mem-pool.md).

### Overall goal

The memory pool will work roughly like the dynamic memory management functions `malloc, calloc, realloc, free`. Unlike the `*alloc` functions, 
  * the metadata for allocated blocks will be kept in a separate dynamic memory section;
  * there will be multiple independent memory pools to allocate from;
  * there is less hiding of (some of) the allocation metadata, to help with debugging and testing.

### API Functions

1. `alloc_status mem_init();`

   This function should be called first and called only once until a corresponding `mem_free()`. It initializes the memory pool (manager) store, a data structure which stores records for separate memory pools.

2. `alloc_status mem_free();`

   This function should be called last and called only once for each corresponding `mem_init()`. It frees the pool (manager) store memory.

3. `pool_pt mem_pool_open(size_t size, alloc_policy policy);`

   This function allocates a single memory pool from which separate allocations can be performed. It takes a `size` in bytes, and an allocation policy, either `FIRST_FIT` or `BEST_FIT`.

4. `alloc_status mem_pool_close(pool_pt pool);`

   This function deallocates a single memory pool.

5. `void * mem_new_alloc(pool_pt pool, size_t size);`

   This function performs a single allocation of `size` in bytes from the given memory pool. Allocations from different memory pools are independent. _**Note:** There is no mechanism for bounds-checking on the use of the allocations._

6. `alloc_status mem_del_alloc(pool_pt pool, void * alloc);`

   This function deallocates the given allocation from the given memory pool.

7. `void mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments);`

   This function returns a new dynamically allocated array of the pool `segments` (allocations or gaps) in the order in which they are in the pool. The number of segments is returned in `num_segments`. The caller is responsible for freeing the array.   

### Data Structures

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

2. Allocation record _(library static)_

   The `mem` pointer is returned to the user. The user passes this pointer and the pointer to the structure of the containing memory pool to the deallocation function `mem_del_alloc`.

   **Structure:**
   ```c
   typedef struct _alloc {
      char *mem;
      size_t size;
   } alloc_t, *alloc_pt;
   ```
   
3. Pool manager _(library static)_

   This is a data structure that the `mem_pool` library uses to store the private metadata for a single memory pool. It is hidden from the user.

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
   
4. (Array-packed linked-list) node heap _(library static)_

   This is a _packed_ linked list which holds nodes for all the segments (allocations or gaps) in a pool, in ascending order by memory address. That is, the first node is always going to point to the segment that starts at the beginning of the pool. This data structure is hidden from the user, except that the `num_allocs` and `num_gaps` variables in the user-facing `pool_t` structure are in sync with the node heap.
   
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
   1. This is a linked list allocated as an array of `node__t` structures. If a node has `used` set to 1, it is part of the list; otherwise, it is an unused node which can be used for a new allocation or gap.
   2. The first node is always present and should always point to the top segment of the pool, regardless of the type of segment (allocation or gap).
   2. An active list node (`used == 1`) is either an allocation (`allocated == 1`) or a gap (`allocated == 0`).
   3. The list is doubly-linked to simplify the deallocation of an allocated sector between two gap sectors.
   4. The linked list is initialized with a certain capacity. If necessary, it should be resized. See the corresponding `static` function and constants in the source file.
   
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
   1. The gap entries hold the `size` of the gaps and point to the corresponding nodes in the node heap linked list.
   2. **(bonus)** The array is initialized with a certain capacity. If necessary, it should be resized. See the corresponding `static` function and constants.
   3. Use the `num_gaps` variable in the user-facing `pool_t` structure as the size of the array and keep it updated.
   4. When deleting entries from the array, pull up the entries that follow and update the size. See the corresponding `static` function.
   5. When adding entries to the array, add at the bottom. If necessary, bubble up. See the corresponding `static` function.
   6. There is a separate `static` function for sorting the array.
   7. **(bonus)** There is a separate `static` function for invalidating the array.

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
      unsigned long allocated;
   } pool_segment_t, *pool_segment_pt;
   ```
   
   **Behavior & management:**
   1. An array of such structures is returned by the function `mem_inspect_pool()` for testing, printing, and debugging.
   2. **Note:** The returned array should be freed by the user.

### Static Functions

The following functions are internal to the library and not exposed to the user. Their names are self-explanatory.

1. **(bonus)** `static alloc_status _mem_resize_pool_store();`

   If the pool store's size is within the fill factor of its capacity, expand it by the expand factor using `realloc()`.

2. **(bonus)** `static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);`

   If the node heap's size is within the fill factor of its capacity, expand it by copying over to a larger block of memory. Use `malloc()` and `memcpy()`, packing the nodes in the array _in order_. You will need to rebuild the gap index, as well.

3. **(bonus)** `static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);`

   If the gap index's size is within the fill factor of its capacity, expand it.

4. `static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);`

   Add a new entry to the gap index. The entry is gap `size` and `node` pointer to a node on the node heap of the given `pool_mgr`.

5. `static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr, size_t size, node_pt node);`

   Remove an entry from the gap index. The entry is gap `size` and `node` pointer to a node on the node heap of the given `pool_mgr`.

6. `static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);`

   Sort the gap index in ascending order by size.
   **Note:** The index always has a length equal to the number of gaps currently in the corresponding pool.

7. `static alloc_status _mem_invalidate_gap_ix(pool_mgr_pt pool_mgr);`

   Useful during node heap expansion.

### Static Variables

The following variables are internal to the library and not exposed to the user. Their names are self-explanatory. They are used to hold the _pool store_ array of pointers to `pool_mgr_t` structures and are manipulated by the user-facing functions `mem_init()`, `mem_pool_open()`, `mem_pool_close()`, and `mem_free()`, and the library static function `_mem_resize_pool_store()`.

```c
static pool_mgr_pt *pool_store = NULL;
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;
```

* * *


## TODO

_this section concerns future iterations of the project_

1. ~Add contents for easy navigation.~
2. ~Refactor to return `mem` not `alloc` to allow pool reallocation and stress testing of multiple large pools and pool growth.~
3. Reference implementation of data on-demand structure expansion and **bonus** tests.
4. Generate drawings of the metadata and allocation scenarios, and embed in README.
5. Try cmocka 1.1.1 memory leak detection.
6. Doxygen for headers, which are part of assignment. [Documentation](http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html). [Example](http://fnch.users.sourceforge.net/doxygen_c.html).
7. Currently, the test suite is _not_ testing `static` function implementation in `mem_pool.c`, which results in sloppy code and shortcuts. _What can be done?_
8. Review `TODO`-s in the code.
9. Static linking of the _cmocka_ library. [Latest release is cmocka 1.1.1](https://cmocka.org/). _CMakeLists.txt should work on all platforms after platform-specific library installation._
