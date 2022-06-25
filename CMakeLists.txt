cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
project(piconet C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(piconet
    piconet.c
)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(piconet pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(piconet)

pico_enable_stdio_usb(piconet 1)
pico_enable_stdio_uart(piconet 0)
