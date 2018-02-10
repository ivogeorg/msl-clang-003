cmake_minimum_required(VERSION 3.3)
project(msl-clang-003)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Werror")

set(SOURCE_FILES
    main.c mem_pool.c test_suite.h test_suite.c)

#[[ TODO
use find_library and/or other config to find the library
on all three major OS after platform-specific installation]]
add_library(libcmocka SHARED IMPORTED)
#set_property(TARGET libcmocka PROPERTY IMPORTED_LOCATION /usr/local/lib/libcmocka.0.4.1.dylib) # MacOS (Yosemite)
set_property(TARGET libcmocka PROPERTY IMPORTED_LOCATION /usr/local/lib/libcmocka.so.0.4.1) # Linux (Ubuntu 16.04.3 LTS)

add_executable(msl-clang-003 ${SOURCE_FILES})

target_link_libraries(msl-clang-003 libcmocka)

