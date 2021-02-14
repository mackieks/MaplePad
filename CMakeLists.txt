cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(mapple_pio_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(maple_pio)

pico_enable_stdio_usb(maple_pio 0)
pico_enable_stdio_uart(maple_pio 1)
pico_add_extra_outputs(maple_pio)

pico_generate_pio_header(maple_pio ${CMAKE_CURRENT_LIST_DIR}/maple.pio)

target_sources(maple_pio PRIVATE maple.c state_machine.c)

target_link_libraries(maple_pio PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
        hardware_pwm
        )


