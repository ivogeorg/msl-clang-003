cmake_minimum_required(VERSION 3.3)
project(denver_os_pa_c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

set(SOURCE_FILES
    main.c mem_pool.c test_suite.h test_suite.c)

add_library(libcmocka SHARED IMPORTED)
set_property(TARGET libcmocka PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libcmocka.0.3.1.dylib)

add_executable(denver_os_pa_c ${SOURCE_FILES})

target_link_libraries(denver_os_pa_c libcmocka)

