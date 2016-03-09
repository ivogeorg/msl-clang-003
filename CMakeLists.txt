cmake_minimum_required(VERSION 3.3)
project(denver_os_pa_c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Werror")

set(SOURCE_FILES
    main.c mem_pool.c test_suite.h test_suite.c)

add_library(libcmocka SHARED IMPORTED)
set_property(TARGET libcmocka PROPERTY IMPORTED_LOCATION /usr/local/lib/libcmocka.so.0.3.1)

add_executable(denver_os_pa_c ${SOURCE_FILES})

target_link_libraries(denver_os_pa_c libcmocka)

