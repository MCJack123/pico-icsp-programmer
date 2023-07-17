cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(icsp C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(icsp
    server.cpp
    icsp.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(icsp pico_stdlib hardware_spi hardware_uart)

pico_enable_stdio_usb(icsp 1)
pico_enable_stdio_uart(icsp 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(icsp)