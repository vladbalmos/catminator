cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(catminator
    motor.c
    sensor.c
    catminator.c
)

pico_enable_stdio_usb(catminator 0)
pico_enable_stdio_uart(catminator 1)

# pull in common dependencies
target_link_libraries(catminator
    pico_multicore
    pico_stdlib)
