cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(piconet C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "-Wall")

pico_sdk_init()
add_executable(piconet
    src/piconet.c
    src/econet.c
    src/adlc.c
    src/util.c
    src/buffer_pool.c
    src/lib/b64/cdecode.c
    src/lib/b64/cencode.c
)

pico_generate_pio_header(piconet ${CMAKE_CURRENT_LIST_DIR}/src/pinctl.pio)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(piconet pico_stdlib pico_multicore hardware_pwm hardware_pio)

# create map/bin/hex file etc.
pico_add_extra_outputs(piconet)

pico_enable_stdio_usb(piconet 1)
pico_enable_stdio_uart(piconet 0)
