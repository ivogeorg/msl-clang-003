cmake_minimum_required(VERSION 3.3)
project(denver_os_pa_c)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

set(SOURCE_FILES
    main.c mem_alloc.c alloc_list.c)

add_executable(denver_os_pa_c ${SOURCE_FILES})