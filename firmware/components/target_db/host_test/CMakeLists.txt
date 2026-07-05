cmake_minimum_required(VERSION 3.16)

project(flexiflash_target_db_host_test C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(target_db_host_test
    test_target_db.c
    ../src/target_db.c
    ../../common/src/common.c
)

target_include_directories(target_db_host_test PRIVATE
    ../include
    ../../common/include
)

enable_testing()
add_test(NAME target_db_host_test COMMAND target_db_host_test)
