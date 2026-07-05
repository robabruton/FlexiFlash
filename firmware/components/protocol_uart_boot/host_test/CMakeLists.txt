cmake_minimum_required(VERSION 3.16)

project(flexiflash_protocol_uart_boot_host_test C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(protocol_uart_boot_host_test
    test_protocol_uart_boot.c
    ../src/protocol_uart_boot.c
    ../../common/src/common.c
)

target_include_directories(protocol_uart_boot_host_test PRIVATE
    ../include
    ../../common/include
)

enable_testing()
add_test(NAME protocol_uart_boot_host_test COMMAND protocol_uart_boot_host_test)
