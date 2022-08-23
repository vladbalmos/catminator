cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(catminator VERSION 0.1 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if(DEFINED ENV{DEBUG_MODE})
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DDEBUG_MODE")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG_MODE")
endif()

add_executable(catminator
    utils.c
    motor.c
    sensor.c
    catminator.c
    battery.c
)

pico_enable_stdio_usb(catminator 0)
pico_enable_stdio_uart(catminator 1)

# pull in common dependencies
target_link_libraries(catminator
    pico_multicore
    pico_stdlib
    hardware_adc
    hardware_clocks)

pico_add_extra_outputs(catminator)
